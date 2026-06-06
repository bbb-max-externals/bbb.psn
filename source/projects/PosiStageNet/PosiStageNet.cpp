#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>
#include <Ws2tcpip.h>
#endif

#include "c74_min.h"
#include "psn_lib.hpp"

#include <array>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <map>
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

std::string socket_error_text(const std::string &prefix) {
#ifdef _WIN32
	return prefix + " (WSA error " + std::to_string(WSAGetLastError()) + ")";
#else
	return prefix + " (errno " + std::to_string(errno) + ": " + std::strerror(errno) + ")";
#endif
}

bool join_multicast_group(socket_handle socket, const std::string &multicast_address) {
	if(multicast_address.empty()) {
		return true;
	}

	ip_mreq request{};
	if(!parse_ipv4_address(multicast_address, request.imr_multiaddr)) {
		return false;
	}
	request.imr_interface.s_addr = htonl(INADDR_ANY);
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

	bool open(uint16_t port, const std::string &multicast_address, std::string &error) {
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

		if(!join_multicast_group(socket_, multicast_address)) {
			error = socket_error_text("multicast join failed");
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

class PosiStageNet : public c74::min::object<PosiStageNet> {
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
		c74::min::description{"IPv4 multicast group to join. Use an empty symbol to disable multicast."}
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
			push_info({"status", running_.load() ? 1 : 0});
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

	~PosiStageNet() {
		stop(false);
	}

private:
	void start() {
		if(running_.load()) {
			push_info({"status", 1});
			output_queue.set();
			return;
		}

		if(worker_.joinable()) {
			worker_.join();
		}

		const int requested_port{(int)port};
		if(requested_port < 1 || 65535 < requested_port) {
			push_info({"error", "port must be between 1 and 65535"});
			output_queue.set();
			return;
		}

		const c74::min::atoms multicast_atoms{multicast.get_atoms()};
		const std::string requested_multicast{multicast_atoms.empty() ? "" : std::string(multicast_atoms[0])};

		running_.store(true);
		worker_ = std::thread([this, requested_port, requested_multicast]() {
			receive_loop((uint16_t)requested_port, requested_multicast);
		});
	}

	void stop(bool report_status = true) {
		const bool was_running{running_.exchange(false)};

		if(worker_.joinable()) {
			worker_.join();
		}

		if(report_status && was_running) {
			push_info({"status", 0});
			output_queue.set();
		}
	}

	void receive_loop(uint16_t requested_port, const std::string requested_multicast) {
		udp_receiver receiver;
		std::string error;
		if(!receiver.open(requested_port, requested_multicast, error)) {
			running_.store(false);
			push_info({"error", error});
			push_info({"status", 0});
			output_queue.set();
			return;
		}

		psn::psn_decoder decoder;
		std::array<char, psn::MAX_UDP_PACKET_SIZE> buffer{};
		uint8_t last_info_frame_id{0};
		uint8_t last_data_frame_id{0};
		bool has_info_frame{false};
		bool has_data_frame{false};

		push_info({"status", 1});
		output_queue.set();

		while(running_.load()) {
			const long byte_count{receiver.receive(buffer.data(), buffer.size())};
			if(byte_count <= 0) {
				continue;
			}

			if(!decoder.decode(buffer.data(), (size_t)byte_count)) {
				push_info({"error", "decode failed"});
				output_queue.set();
				continue;
			}

			const auto &info = decoder.get_info();
			if((!has_info_frame || last_info_frame_id != info.header.frame_id) && !info.system_name.empty()) {
				has_info_frame = true;
				last_info_frame_id = info.header.frame_id;
				push_info({"server", info.system_name});
				for(const auto &entry : info.tracker_names) {
					tracker_names_[entry.first] = entry.second;
					push_info({"name", entry.first, entry.second});
				}
				output_queue.set();
			}

			const auto &data = decoder.get_data();
			if(!has_data_frame || last_data_frame_id != data.header.frame_id) {
				has_data_frame = true;
				last_data_frame_id = data.header.frame_id;
				for(const auto &entry : data.trackers) {
					push_tracker(entry.second);
				}
				output_queue.set();
			}
		}
	}

	void push_tracker(const psn::tracker &tracker) {
		const auto position = tracker.get_pos();
		const auto orientation = tracker.get_ori();
		const auto tracker_id = (int)tracker.get_id();
		std::string tracker_name;
		const auto name_iterator = tracker_names_.find(tracker_id);
		if(name_iterator != tracker_names_.end()) {
			tracker_name = name_iterator->second;
		}

		push_tracker_atoms({
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
		});
	}

	void push_tracker_atoms(c74::min::atoms values) {
		std::lock_guard<std::mutex> lock(output_mutex_);
		tracker_events_.push_back({std::move(values)});
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

	std::atomic<bool> running_{false};
	std::thread worker_;
	std::mutex output_mutex_;
	std::vector<output_event> tracker_events_;
	std::vector<output_event> info_events_;
	std::map<int, std::string> tracker_names_;
};

MIN_EXTERNAL(PosiStageNet);
