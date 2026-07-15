#include <iostream>
#include <cstdlib>
#include <string>
#include <csignal>
#include "Server.hpp"
#include "Utils.hpp"

static void	usage(const char *bin) {
	std::cerr << "Usage: " << bin << " <port> <password>" << std::endl;
}

int	main(int ac, char **av) {
	if (ac != 3) {
		usage(av[0]);
		return 1;
	}
	long port = Utils::atoiSafe(av[1]);
	if (port < 1 || port > 65535) {
		std::cerr << "Error: port must be between 1 and 65535" << std::endl;
		return 1;
	}
	std::string password(av[2]);
	if (password.empty()) {
		std::cerr << "Error: password cannot be empty" << std::endl;
		return 1;
	}

	// Ignore SIGPIPE so send() on a closed socket returns EPIPE instead of killing the process
	std::signal(SIGPIPE, SIG_IGN);

	try {
		Server server(static_cast<int>(port), password);
		server.run();
	} catch (const std::exception &e) {
		std::cerr << "Fatal: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
