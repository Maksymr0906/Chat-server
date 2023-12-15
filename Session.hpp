#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <string>
#include <queue>
#include <memory>

namespace io = boost::asio;
using tcp = io::ip::tcp;
using system_error = boost::system::error_code;

using message_handler = std::function<void(std::string)>;
using error_handler = std::function<void()>;

class Session : public std::enable_shared_from_this<Session> {
private:
	tcp::socket socket;
	io::streambuf buffer;
	std::queue<std::string> outgoing;
	message_handler on_message;
	error_handler on_error;
public:
	Session(tcp::socket &&socket) 
		:socket{std::move(socket)} {}

	void start(message_handler&& on_message, error_handler&& on_error) {
		this->on_message = std::move(on_message);
		this->on_error = std::move(on_error);
		async_read();
	}

	void post(const std::string& message) {
		bool idle = outgoing.empty();

		outgoing.push(message);
		
		if (idle) {
			async_write();
		}
	}

	void async_read() {
		io::async_read_until(socket, buffer, '\n', boost::bind(&Session::read_handler, shared_from_this(), _1, _2));
	}

	void read_handler(system_error error, std::size_t bytes_transferred) {
		if (!error) {
			std::stringstream message;
			message << socket.remote_endpoint(error) << ": "
				<< std::istream(&buffer).rdbuf();
			buffer.consume(bytes_transferred);
			on_message(message.str());
			async_read();
		}
		else {
			socket.close(error);
			on_error();
		}
	}

	void async_write() {
		io::async_write(socket, io::buffer(outgoing.front()), boost::bind(&Session::write_handler, shared_from_this(), _1, _2));
	}

	void write_handler(system_error error, std::size_t bytes_transferred) {
		if (!error) {
			outgoing.pop();

			if (!outgoing.empty()) {
				async_write();
			}
		}
		else {
			std::cerr << "Writing error: " << error.message() << std::endl;
			socket.close();
			on_error();
		}
	}

	std::string get_address() const {
		tcp::endpoint ep = socket.remote_endpoint();
		std::string remote_address = ep.address().to_string() + ":" + std::to_string(ep.port());
		return remote_address;
	}
};