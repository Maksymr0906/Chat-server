#pragma once

#include "Session.hpp"

#include <optional>
#include <unordered_set>
#include <algorithm>

class Server {
private:
	io::io_context& io;
	tcp::acceptor acceptor;
	std::optional<tcp::socket> socket;
	std::unordered_set<std::shared_ptr<Session>> clients;

	void accept_handler(const system_error error) {
		if (!error) {
			auto client = std::make_shared<Session>(std::move(*socket));
			std::string client_name = client->get_address();
			client->post("Welcome to chat " + client_name + "!\n\r");
			post("We have a new user: " + client_name + "\n\r");
			clients.insert(client);

			client->start(
				boost::bind(&Server::post, this, _1),
				[&, client_name = client_name, weak = std::weak_ptr(client)]() {
					if (auto shared = weak.lock();
						shared && clients.erase(shared)) {
							post("Goodbye " + client_name + "!\n\r");
					}
				}
			);

			async_accept();
		}
		else {
			std::cerr << "Error accept connection: " << error.message() << std::endl;
		}
	}
public:
	Server(io::io_context& io, uint16_t port)
		:io{ io }, acceptor{ io, tcp::endpoint(tcp::v4(), port) } {}

	void async_accept() {
		socket.emplace(io);
		acceptor.async_accept(*socket, boost::bind(&Server::accept_handler, this, _1));
	}

	void post(const std::string& message) {
		std::for_each(clients.begin(), clients.end(), [&](std::shared_ptr<Session> client) {client->post(message); });
	}
};