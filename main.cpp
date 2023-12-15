#include "Server.hpp"

int main() {
	try {
		io::io_context io;
		Server server{ io, 13 };
		server.async_accept();
		io.run();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}