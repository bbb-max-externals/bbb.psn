#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>
#include <Ws2tcpip.h>
#endif

#include "c74_min.h"
#include "psn_lib.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <list>
#include <memory>
#include <string>

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

std::string socket_error_text(const std::string &prefix) {
#ifdef _WIN32
	return prefix + " (WSA error " + std::to_string(WSAGetLastError()) + ")";
#else
	return prefix + " (errno " + std::to_string(errno) + ": " + std::strerror(errno) + ")";
#endif
}

bool address_is_any(const std::string &address) {
	return address.empty() || address == "any" || address == "0.0.0.0";
}

bool parse_ipv4_address(const std::string &address, in_addr &result) {
	return inet_pton(AF_INET, address.c_str(), &result) == 1;
}

bool address_is_multicast(const in_addr &address) {
	const uint32_t host_order_address{ntohl(address.s_addr)};
	return 0xE0000000 <= host_order_address && host_order_address <= 0xEFFFFFFF;
}

class udp_sender {
public:
	udp_sender() = default;
	udp_sender(const udp_sender &) = delete;
	udp_sender(udp_sender &&) = delete;
	udp_sender &operator=(const udp_sender &) = delete;
	udp_sender &operator=(udp_sender &&) = delete;

	~udp_sender() {
		close();
	}

	bool open(std::string &error) {
		if(socket_ != invalid_socket_handle) {
			return true;
		}

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

		return true;
	}

	bool send(const std::string &destination, const std::string &local_address, uint16_t port, const std::string &packet, std::string &error) {
		if(!open(error)) {
			return false;
		}

		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_port = htons(port);
		if(!parse_ipv4_address(destination, address.sin_addr)) {
			error = "invalid IPv4 destination: " + destination;
			return false;
		}

		if(address_is_multicast(address.sin_addr) && !set_multicast_interface(local_address, error)) {
			return false;
		}

		const int result = sendto(socket_, packet.data(), (int)packet.size(), 0, (sockaddr *)&address, sizeof(address));
		if(result < 0) {
			error = socket_error_text("sendto failed");
			return false;
		}
		return true;
	}

	bool set_multicast_interface(const std::string &local_address, std::string &error) {
		if(address_is_any(local_address)) {
			return true;
		}

		in_addr interface_address{};
		if(!parse_ipv4_address(local_address, interface_address)) {
			error = "invalid IPv4 localaddr: " + local_address;
			return false;
		}

		if(setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_IF, (const char *)&interface_address, sizeof(interface_address)) < 0) {
			error = socket_error_text("IP_MULTICAST_IF failed");
			return false;
		}

		return true;
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

uint64_t now_microseconds() {
	using clock = std::chrono::system_clock;
	const auto now = clock::now().time_since_epoch();
	return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

bool has_count(const c74::min::atoms &args, size_t count) {
	return count <= args.size();
}

int atom_to_int(const c74::min::atoms &args, size_t index) {
	return (int)args[index];
}

bool valid_tracker_id(int id) {
	return 0 <= id && id <= 65535;
}

double atom_to_double(const c74::min::atoms &args, size_t index) {
	return (double)args[index];
}

std::string atom_to_string(const c74::min::atoms &args, size_t index) {
	return std::string(args[index]);
}

} // namespace

class bbb_psn_sender : public c74::min::object<bbb_psn_sender> {
public:
	MIN_DESCRIPTION{"Encode and send PosiStageNet tracking data over UDP"};
	MIN_TAGS{"posistagenet, psn, tracking, udp, multicast"};
	MIN_AUTHOR{"2bit"};

	c74::min::inlet<> input{this, "(messages) tracker, pos, ori, speed, accel, target, status, name, clear, send, info, bang"};
	c74::min::outlet<> info_output{this, "(anything) status and errors", ""};

	c74::min::attribute<c74::min::symbol> destination{this, "destination", psn::DEFAULT_UDP_MULTICAST_ADDR,
		c74::min::description{"IPv4 destination address."}
	};

	c74::min::attribute<int> port{this, "port", (int)psn::DEFAULT_UDP_PORT,
		c74::min::description{"UDP destination port."},
		c74::min::range{1, 65535}
	};

	c74::min::attribute<c74::min::symbol> localaddr{this, "localaddr", "any",
		c74::min::description{"Local IPv4 address to use as the multicast output interface. Use any/0.0.0.0 for OS routing."}
	};

	c74::min::attribute<c74::min::symbol> system{this, "system", "Max",
		c74::min::description{"PSN system/server name used in info packets."}
	};

	c74::min::message<> tracker_message{this, "tracker", "Set tracker position and optional orientation: tracker id x y z [yaw pitch roll]",
		MIN_FUNCTION {
			if(!has_count(args, 4)) {
				error("tracker requires: id x y z [yaw pitch roll]");
				return {};
			}
			const int tracker_id{atom_to_int(args, 0)};
			if(!require_tracker_id(tracker_id)) {
				return {};
			}
			auto &tracker = tracker_for(tracker_id);
			tracker.set_pos(psn::float3((float)atom_to_double(args, 1), (float)atom_to_double(args, 2), (float)atom_to_double(args, 3)));
			if(has_count(args, 7)) {
				tracker.set_ori(psn::float3((float)atom_to_double(args, 4), (float)atom_to_double(args, 5), (float)atom_to_double(args, 6)));
			}
			return {};
		}
	};

	c74::min::message<> pos_message{this, "pos", "Set tracker position: pos id x y z",
		MIN_FUNCTION {
			if(!has_count(args, 4)) {
				error("pos requires: id x y z");
				return {};
			}
			const int tracker_id{atom_to_int(args, 0)};
			if(!require_tracker_id(tracker_id)) {
				return {};
			}
			tracker_for(tracker_id).set_pos(psn::float3((float)atom_to_double(args, 1), (float)atom_to_double(args, 2), (float)atom_to_double(args, 3)));
			return {};
		}
	};

	c74::min::message<> orientation_message{this, "ori", "Set tracker orientation: ori id yaw pitch roll",
		MIN_FUNCTION {
			if(!has_count(args, 4)) {
				error("ori requires: id yaw pitch roll");
				return {};
			}
			const int tracker_id{atom_to_int(args, 0)};
			if(!require_tracker_id(tracker_id)) {
				return {};
			}
			tracker_for(tracker_id).set_ori(psn::float3((float)atom_to_double(args, 1), (float)atom_to_double(args, 2), (float)atom_to_double(args, 3)));
			return {};
		}
	};

	c74::min::message<> speed_message{this, "speed", "Set tracker speed: speed id x y z",
		MIN_FUNCTION {
			if(!has_count(args, 4)) {
				error("speed requires: id x y z");
				return {};
			}
			const int tracker_id{atom_to_int(args, 0)};
			if(!require_tracker_id(tracker_id)) {
				return {};
			}
			tracker_for(tracker_id).set_speed(psn::float3((float)atom_to_double(args, 1), (float)atom_to_double(args, 2), (float)atom_to_double(args, 3)));
			return {};
		}
	};

	c74::min::message<> acceleration_message{this, "accel", "Set tracker acceleration: accel id x y z",
		MIN_FUNCTION {
			if(!has_count(args, 4)) {
				error("accel requires: id x y z");
				return {};
			}
			const int tracker_id{atom_to_int(args, 0)};
			if(!require_tracker_id(tracker_id)) {
				return {};
			}
			tracker_for(tracker_id).set_accel(psn::float3((float)atom_to_double(args, 1), (float)atom_to_double(args, 2), (float)atom_to_double(args, 3)));
			return {};
		}
	};

	c74::min::message<> target_message{this, "target", "Set tracker target position: target id x y z",
		MIN_FUNCTION {
			if(!has_count(args, 4)) {
				error("target requires: id x y z");
				return {};
			}
			const int tracker_id{atom_to_int(args, 0)};
			if(!require_tracker_id(tracker_id)) {
				return {};
			}
			tracker_for(tracker_id).set_target_pos(psn::float3((float)atom_to_double(args, 1), (float)atom_to_double(args, 2), (float)atom_to_double(args, 3)));
			return {};
		}
	};

	c74::min::message<> status_message{this, "status", "Set tracker status: status id value",
		MIN_FUNCTION {
			if(!has_count(args, 2)) {
				error("status requires: id value");
				return {};
			}
			const int tracker_id{atom_to_int(args, 0)};
			if(!require_tracker_id(tracker_id)) {
				return {};
			}
			tracker_for(tracker_id).set_status((float)atom_to_double(args, 1));
			return {};
		}
	};

	c74::min::message<> name_message{this, "name", "Set tracker name: name id symbol",
		MIN_FUNCTION {
			if(!has_count(args, 2)) {
				error("name requires: id symbol");
				return {};
			}
			const int tracker_id{atom_to_int(args, 0)};
			if(!require_tracker_id(tracker_id)) {
				return {};
			}
			tracker_for(tracker_id).set_name(atom_to_string(args, 1));
			return {};
		}
	};

	c74::min::message<> clear_message{this, "clear", "Clear one tracker by id, or all trackers with no args.",
		MIN_FUNCTION {
			if(args.empty()) {
				trackers_.clear();
			} else {
				const int tracker_id{atom_to_int(args, 0)};
				if(!require_tracker_id(tracker_id)) {
					return {};
				}
				trackers_.erase((uint16_t)tracker_id);
			}
			return {};
		}
	};

	c74::min::message<> send_message{this, "send", "Send one PSN data frame.",
		MIN_FUNCTION {
			send_data();
			return {};
		}
	};

	c74::min::message<> bang_message{this, "bang", "Send one PSN data frame.",
		MIN_FUNCTION {
			send_data();
			return {};
		}
	};

	c74::min::message<> info_message{this, "info", "Send one PSN info frame.",
		MIN_FUNCTION {
			send_info();
			return {};
		}
	};

private:
	bool require_tracker_id(int requested_id) {
		if(!valid_tracker_id(requested_id)) {
			error("tracker id must be between 0 and 65535");
			return false;
		}
		return true;
	}

	psn::tracker &tracker_for(int requested_id) {
		const uint16_t id = (uint16_t)requested_id;
		auto iterator = trackers_.find(id);
		if(iterator == trackers_.end()) {
			iterator = trackers_.emplace(id, psn::tracker(id)).first;
		}
		return iterator->second;
	}

	void send_info() {
		ensure_encoder();
		send_packets(encoder_->encode_info(trackers_, now_microseconds()), "info");
	}

	void send_data() {
		ensure_encoder();
		const uint64_t timestamp = now_microseconds();
		for(auto &entry : trackers_) {
			entry.second.set_timestamp(timestamp);
		}
		send_packets(encoder_->encode_data(trackers_, timestamp), "data");
	}

	void send_packets(const std::list<std::string> &packets, const char *packet_type) {
		const int requested_port{(int)port};
		if(requested_port < 1 || 65535 < requested_port) {
			error("port must be between 1 and 65535");
			return;
		}

		const std::string requested_destination = attribute_symbol_to_string(destination);
		const std::string requested_local_address = attribute_symbol_to_string(localaddr);
		std::string send_error;
		int sent_count{0};
		for(const auto &packet : packets) {
			if(!sender_.send(requested_destination, requested_local_address, (uint16_t)requested_port, packet, send_error)) {
				error(send_error);
				return;
			}
			sent_count++;
		}
		info_output.send({"sent", packet_type, sent_count});
	}

	void ensure_encoder() {
		const std::string requested_system = attribute_symbol_to_string(system);
		if(!encoder_ || requested_system != encoder_system_) {
			encoder_system_ = requested_system;
			encoder_ = std::make_unique<psn::psn_encoder>(encoder_system_);
		}
	}

	std::string attribute_symbol_to_string(c74::min::attribute<c74::min::symbol> &attribute) const {
		const c74::min::atoms values{attribute.get_atoms()};
		return values.empty() ? "" : std::string(values[0]);
	}

	void error(const std::string &message) {
		info_output.send({"error", message});
	}

	psn::tracker_map trackers_;
	udp_sender sender_;
	std::unique_ptr<psn::psn_encoder> encoder_;
	std::string encoder_system_;
};

MIN_EXTERNAL(bbb_psn_sender);
