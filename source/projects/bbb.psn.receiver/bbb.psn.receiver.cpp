#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>
#include <Ws2tcpip.h>
#endif

#include "c74_min.h"
#include "psn_lib.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

#ifdef _WIN32
using socket_handle = SOCKET;
constexpr socket_handle invalid_socket_handle = INVALID_SOCKET;
#else
using socket_handle = int;
constexpr socket_handle invalid_socket_handle = -1;
#endif

void close_socket(socket_handle socket) {
	if(socket == invalid_socket_handle) {
		return;
	}

#ifdef _WIN32
	closesocket(socket);
#else
	close(socket);
#endif
}

bool set_reuse_address(socket_handle socket) {
	int value{1};
	return 0 <= setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&value, sizeof(value));
}

bool set_receive_timeout(socket_handle socket, int timeout_milliseconds) {
#ifdef _WIN32
	DWORD timeout{(DWORD)timeout_milliseconds};
	return 0 <= setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#else
	timeval timeout{};
	timeout.tv_sec = timeout_milliseconds / 1000;
	timeout.tv_usec = (timeout_milliseconds % 1000) * 1000;
	return 0 <= setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#endif
}

bool parse_ipv4_address(const std::string &address, in_addr &result) {
	return inet_pton(AF_INET, address.c_str(), &result) == 1;
}

bool address_is_any(const std::string &address) {
	return address.empty() || address == "any" || address == "0.0.0.0";
}

std::string socket_error_text(const std::string &prefix) {
#ifdef _WIN32
	return prefix + " (WSA error " + std::to_string(WSAGetLastError()) + ")";
#else
	return prefix + " (errno " + std::to_string(errno) + ": " + std::strerror(errno) + ")";
#endif
}

std::string normalized_symbol_text(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
		return (char)std::tolower(character);
	});
	return value;
}

bool multicast_is_disabled(const std::string &multicast_address) {
	const std::string value{normalized_symbol_text(multicast_address)};
	return value.empty() || value == "none" || value == "off" || value == "false" || value == "0" || value == "unicast";
}

bool join_multicast_group(socket_handle socket, const std::string &multicast_address, const std::string &local_address) {
	if(multicast_is_disabled(multicast_address)) {
		return true;
	}

	ip_mreq request{};
	if(!parse_ipv4_address(multicast_address, request.imr_multiaddr)) {
		return false;
	}

	if(address_is_any(local_address)) {
		request.imr_interface.s_addr = htonl(INADDR_ANY);
	} else if(!parse_ipv4_address(local_address, request.imr_interface)) {
		return false;
	}

	return 0 <= setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&request, sizeof(request));
}

class udp_receiver {
public:
	udp_receiver() = default;
	udp_receiver(const udp_receiver &) = delete;
	udp_receiver(udp_receiver &&) = delete;
	udp_receiver &operator=(const udp_receiver &) = delete;
	udp_receiver &operator=(udp_receiver &&) = delete;

	~udp_receiver() {
		close();
	}

	bool open(uint16_t port, const std::string &multicast_address, const std::string &local_address, std::string &error) {
		close();

#ifdef _WIN32
		if(0 == wsa_reference_count_++) {
			WSADATA data{};
			if(WSAStartup(MAKEWORD(2, 2), &data) != 0) {
				wsa_reference_count_--;
				error = socket_error_text("WSAStartup failed");
				return false;
			}
		}
#endif

		socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(socket_ == invalid_socket_handle) {
			error = socket_error_text("socket creation failed");
			cleanup_wsa_if_needed();
			return false;
		}

		set_reuse_address(socket_);
		set_receive_timeout(socket_, 100);

		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_ANY);
		address.sin_port = htons(port);

		if(bind(socket_, (sockaddr *)&address, sizeof(address)) < 0) {
			error = socket_error_text("bind failed");
			close();
			return false;
		}

		if(!join_multicast_group(socket_, multicast_address, local_address)) {
			error = socket_error_text("multicast join failed for localaddr " + local_address);
			close();
			return false;
		}

		return true;
	}

	long receive(char *buffer, size_t buffer_size) {
		if(socket_ == invalid_socket_handle) {
			return -1;
		}

#ifdef _WIN32
		return recv(socket_, buffer, (int)buffer_size, 0);
#else
		return (long)recv(socket_, buffer, buffer_size, 0);
#endif
	}

	void close() {
		if(socket_ != invalid_socket_handle) {
			close_socket(socket_);
			socket_ = invalid_socket_handle;
		}
		cleanup_wsa_if_needed();
	}

private:
	void cleanup_wsa_if_needed() {
#ifdef _WIN32
		if(0 < wsa_reference_count_) {
			if(0 == --wsa_reference_count_) {
				WSACleanup();
			}
		}
#endif
	}

	socket_handle socket_{invalid_socket_handle};
#ifdef _WIN32
	inline static std::atomic<int> wsa_reference_count_{0};
#endif
};

struct output_event {
	c74::min::atoms values;
};

} // namespace

class shared_psn_receiver {
public:
	struct tagged_callback {
		void *owner;
		std::function<void(const std::vector<output_event> &, const std::vector<output_event> &)> function;
	};

	shared_psn_receiver(uint16_t port, std::string multicast_address, std::string local_address)
	: port_{port}
	, multicast_address_{std::move(multicast_address)}
	, local_address_{std::move(local_address)}
	{}

	shared_psn_receiver(const shared_psn_receiver &) = delete;
	shared_psn_receiver(shared_psn_receiver &&) = delete;
	shared_psn_receiver &operator=(const shared_psn_receiver &) = delete;
	shared_psn_receiver &operator=(shared_psn_receiver &&) = delete;

	~shared_psn_receiver() {
		stop();
	}

	bool start(std::string &error) {
		if(running_.load()) {
			return true;
		}

		if(!receiver_.open(port_, multicast_address_, local_address_, error)) {
			return false;
		}

		running_.store(true);
		worker_ = std::thread([this]() {
			receive_loop();
		});
		return true;
	}

	void stop() {
		if(!running_.exchange(false)) {
			return;
		}

		receiver_.close();
		if(worker_.joinable()) {
			worker_.join();
		}
	}

	void add_callback(void *owner, std::function<void(const std::vector<output_event> &, const std::vector<output_event> &)> callback) {
		std::lock_guard<std::mutex> lock(callback_mutex_);
		callbacks_.push_back({owner, std::move(callback)});
	}

	void remove_callback(void *owner) {
		std::lock_guard<std::mutex> lock(callback_mutex_);
		callbacks_.erase(
			std::remove_if(callbacks_.begin(), callbacks_.end(), [owner](const tagged_callback &callback) {
				return callback.owner == owner;
			}),
			callbacks_.end()
		);
	}

	size_t callback_count() const {
		std::lock_guard<std::mutex> lock(callback_mutex_);
		return callbacks_.size();
	}

private:
	void receive_loop() {
		std::array<char, psn::MAX_UDP_PACKET_SIZE> buffer{};

		broadcast({}, {output_event{{"status", 1}}});

		while(running_.load()) {
			const long byte_count{receiver_.receive(buffer.data(), buffer.size())};
			if(byte_count <= 0) {
				continue;
			}

			if(!decoder_.decode(buffer.data(), (size_t)byte_count)) {
				broadcast({}, {output_event{{"error", "decode failed"}}});
				continue;
			}

			std::vector<output_event> tracker_events;
			std::vector<output_event> info_events;
			collect_info_events(info_events);
			collect_tracker_events(tracker_events);
			if(!tracker_events.empty() || !info_events.empty()) {
				broadcast(tracker_events, info_events);
			}
		}
	}

	void collect_info_events(std::vector<output_event> &info_events) {
		const auto &info = decoder_.get_info();
		if((!has_info_frame_ || last_info_frame_id_ != info.header.frame_id) && !info.system_name.empty()) {
			has_info_frame_ = true;
			last_info_frame_id_ = info.header.frame_id;
			info_events.push_back({{"server", info.system_name}});
			for(const auto &entry : info.tracker_names) {
				tracker_names_[entry.first] = entry.second;
				info_events.push_back({{"name", entry.first, entry.second}});
			}
		}
	}

	void collect_tracker_events(std::vector<output_event> &tracker_events) {
		const auto &data = decoder_.get_data();
		if(!has_data_frame_ || last_data_frame_id_ != data.header.frame_id) {
			has_data_frame_ = true;
			last_data_frame_id_ = data.header.frame_id;
			for(const auto &entry : data.trackers) {
				tracker_events.push_back(make_tracker_event(entry.second));
			}
		}
	}

	output_event make_tracker_event(const psn::tracker &tracker) const {
		const auto position = tracker.get_pos();
		const auto orientation = tracker.get_ori();
		const auto tracker_id = (int)tracker.get_id();
		std::string tracker_name;
		const auto name_iterator = tracker_names_.find(tracker_id);
		if(name_iterator != tracker_names_.end()) {
			tracker_name = name_iterator->second;
		}

		return {{
			"tracker",
			tracker_id,
			tracker_name,
			(double)position.x,
			(double)position.y,
			(double)position.z,
			(double)orientation.x,
			(double)orientation.y,
			(double)orientation.z,
			(double)tracker.get_status(),
			(double)tracker.get_timestamp()
		}};
	}

	void broadcast(const std::vector<output_event> &tracker_events, const std::vector<output_event> &info_events) {
		std::lock_guard<std::mutex> lock(callback_mutex_);
		for(const auto &callback : callbacks_) {
			callback.function(tracker_events, info_events);
		}
	}

	uint16_t port_;
	std::string multicast_address_;
	std::string local_address_;
	udp_receiver receiver_;
	psn::psn_decoder decoder_;
	std::atomic<bool> running_{false};
	std::thread worker_;
	mutable std::mutex callback_mutex_;
	std::vector<tagged_callback> callbacks_;
	std::map<int, std::string> tracker_names_;
	uint8_t last_info_frame_id_{0};
	uint8_t last_data_frame_id_{0};
	bool has_info_frame_{false};
	bool has_data_frame_{false};
};

struct receiver_key {
	uint16_t port;
	std::string multicast_address;
	std::string local_address;

	bool operator<(const receiver_key &rhs) const {
		if(port != rhs.port) {
			return port < rhs.port;
		}
		if(multicast_address != rhs.multicast_address) {
			return multicast_address < rhs.multicast_address;
		}
		return local_address < rhs.local_address;
	}
};

std::string normalize_address_key(const std::string &address, const std::string &any_value) {
	const std::string value{normalized_symbol_text(address)};
	if(value.empty() || value == "any" || value == "0.0.0.0") {
		return any_value;
	}
	return value;
}

std::string normalize_multicast_key(const std::string &multicast_address) {
	if(multicast_is_disabled(multicast_address)) {
		return "none";
	}
	return normalize_address_key(multicast_address, "none");
}

std::string normalize_local_address_key(const std::string &local_address) {
	return normalize_address_key(local_address, "any");
}

class receiver_registry {
public:
	static receiver_registry &shared() {
		static receiver_registry instance;
		return instance;
	}

	std::shared_ptr<shared_psn_receiver> get(uint16_t port, const std::string &multicast_address, const std::string &local_address, std::string &error) {
		std::lock_guard<std::mutex> lock(mutex_);
		receiver_key key{port, normalize_multicast_key(multicast_address), normalize_local_address_key(local_address)};
		const auto iterator = receivers_.find(key);
		if(iterator != receivers_.end()) {
			return iterator->second;
		}

		auto receiver = std::make_shared<shared_psn_receiver>(port, key.multicast_address, key.local_address);
		if(!receiver->start(error)) {
			return nullptr;
		}
		receivers_.insert(std::make_pair(key, receiver));
		return receiver;
	}

	void release(uint16_t port, const std::string &multicast_address, const std::string &local_address) {
		std::shared_ptr<shared_psn_receiver> receiver_to_destroy;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			receiver_key key{port, normalize_multicast_key(multicast_address), normalize_local_address_key(local_address)};
			const auto iterator = receivers_.find(key);
			if(iterator != receivers_.end() && iterator->second->callback_count() == 0) {
				receiver_to_destroy = iterator->second;
				receivers_.erase(iterator);
			}
		}
	}

private:
	receiver_registry() = default;
	std::mutex mutex_;
	std::map<receiver_key, std::shared_ptr<shared_psn_receiver>> receivers_;
};

class bbb_psn_receiver : public c74::min::object<bbb_psn_receiver> {
public:
	MIN_DESCRIPTION{"Receive and decode PosiStageNet tracking data over UDP"};
	MIN_TAGS{"posistagenet, psn, tracking, udp, multicast"};
	MIN_AUTHOR{"2bit"};

	c74::min::inlet<> input{this, "(messages) start, stop, restart, bang"};
	c74::min::outlet<> tracker_output{this, "(anything) tracker data", ""};
	c74::min::outlet<> info_output{this, "(anything) server, names, status and errors", ""};

	c74::min::attribute<int> port{this, "port", (int)psn::DEFAULT_UDP_PORT,
		c74::min::description{"UDP port to listen on."},
		c74::min::range{1, 65535}
	};

	c74::min::attribute<c74::min::symbol> multicast{this, "multicast", psn::DEFAULT_UDP_MULTICAST_ADDR,
		c74::min::description{"IPv4 multicast group to join. Use none/off/false/0/unicast to disable multicast join for unicast receiving."}
	};

	c74::min::attribute<c74::min::symbol> localaddr{this, "localaddr", "any",
		c74::min::description{"Local IPv4 address to use when joining a multicast group. Use any/0.0.0.0 for OS default."}
	};

	c74::min::attribute<bool> autostart{this, "autostart", true,
		c74::min::description{"Start listening when the object is created."},
		c74::min::style{c74::min::style::onoff}
	};

	c74::min::message<> start_message{this, "start", "Open UDP socket and start receiving PSN packets.",
		MIN_FUNCTION {
			start();
			return {};
		}
	};

	c74::min::message<> stop_message{this, "stop", "Stop receiving and close the socket.",
		MIN_FUNCTION {
			stop();
			return {};
		}
	};

	c74::min::message<> restart_message{this, "restart", "Restart receiving with the current attributes.",
		MIN_FUNCTION {
			stop();
			start();
			return {};
		}
	};

	c74::min::message<> bang_message{this, "bang", "Report receiver status.",
		MIN_FUNCTION {
			push_info({"status", receiver_ ? 1 : 0});
			if(receiver_) {
				push_info({"subscribers", (int)receiver_->callback_count()});
			}
			output_queue.set();
			return {};
		}
	};

	c74::min::message<> loadbang_message{this, "loadbang",
		MIN_FUNCTION {
			if((bool)autostart) {
				start();
			}
			return {};
		}
	};

	c74::min::queue<> output_queue{this,
		MIN_FUNCTION {
			flush_output();
			return {};
		}
	};

	~bbb_psn_receiver() {
		stop(false);
	}

private:
	void start() {
		if(receiver_) {
			push_info({"status", 1});
			push_info({"subscribers", (int)receiver_->callback_count()});
			output_queue.set();
			return;
		}

		const int requested_port{(int)port};
		if(requested_port < 1 || 65535 < requested_port) {
			push_info({"error", "port must be between 1 and 65535"});
			output_queue.set();
			return;
		}

		active_port_ = (uint16_t)requested_port;
		active_multicast_ = attribute_symbol_to_string(multicast, "");
		active_local_address_ = attribute_symbol_to_string(localaddr, "any");

		std::string error;
		auto receiver = receiver_registry::shared().get(active_port_, active_multicast_, active_local_address_, error);
		if(!receiver) {
			push_info({"error", error});
			push_info({"status", 0});
			output_queue.set();
			return;
		}

		receiver_ = receiver;
		receiver_->add_callback(this, [this](const std::vector<output_event> &tracker_events, const std::vector<output_event> &info_events) {
			enqueue_events(tracker_events, info_events);
		});
		push_info({"status", 1});
		push_info({"subscribers", (int)receiver_->callback_count()});
		output_queue.set();
	}

	void stop(bool report_status = true) {
		if(!receiver_) {
			return;
		}

		receiver_->remove_callback(this);
		receiver_.reset();
		receiver_registry::shared().release(active_port_, active_multicast_, active_local_address_);

		if(report_status) {
			push_info({"status", 0});
			output_queue.set();
		}
	}

	std::string attribute_symbol_to_string(c74::min::attribute<c74::min::symbol> &attribute, const std::string &fallback) const {
		const c74::min::atoms values{attribute.get_atoms()};
		return values.empty() ? fallback : std::string(values[0]);
	}

	void enqueue_events(const std::vector<output_event> &tracker_events, const std::vector<output_event> &info_events) {
		{
			std::lock_guard<std::mutex> lock(output_mutex_);
			tracker_events_.insert(tracker_events_.end(), tracker_events.begin(), tracker_events.end());
			info_events_.insert(info_events_.end(), info_events.begin(), info_events.end());
		}
		output_queue.set();
	}

	void push_info(c74::min::atoms values) {
		std::lock_guard<std::mutex> lock(output_mutex_);
		info_events_.push_back({std::move(values)});
	}

	void flush_output() {
		std::vector<output_event> tracker_events;
		std::vector<output_event> info_events;
		{
			std::lock_guard<std::mutex> lock(output_mutex_);
			tracker_events.swap(tracker_events_);
			info_events.swap(info_events_);
		}

		for(const auto &event : info_events) {
			info_output.send(event.values);
		}
		for(const auto &event : tracker_events) {
			tracker_output.send(event.values);
		}
	}

	std::shared_ptr<shared_psn_receiver> receiver_;
	std::mutex output_mutex_;
	std::vector<output_event> tracker_events_;
	std::vector<output_event> info_events_;
	uint16_t active_port_{0};
	std::string active_multicast_;
	std::string active_local_address_;
};

MIN_EXTERNAL(bbb_psn_receiver);
