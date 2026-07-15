#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>
# include <map>
# include <poll.h>
# include <csignal>
# include "Client.hpp"
# include "Channel.hpp"

class Server {
public:
	Server(int port, const std::string &password);
	~Server();

	void	run();
	static void	handleSignal(int sig);

	// accessors used by command handlers
	const std::string&			password() const;
	std::map<int, Client*>&		clients();
	std::map<std::string, Channel*>&	channels();
	Client*		findClientByNick(const std::string &nick);
	Channel*	findChannel(const std::string &name);
	Channel*	getOrCreateChannel(const std::string &name);
	void		removeChannel(const std::string &name);
	void		disconnectClient(int fd, const std::string &reason);
	const std::string&	serverName() const;

private:
	int							_port;
	std::string					_password;
	std::string					_serverName;
	int							_listenFd;
	std::vector<struct pollfd>	_pfds;
	std::map<int, Client*>		_clients;
	std::map<std::string, Channel*>	_channels;

	static volatile sig_atomic_t	_stop;

	void	setupListenSocket();
	void	acceptNewClient();
	void	handleClientRead(int fd);
	void	handleClientWrite(int fd);
	void	processBuffer(Client *c);
	void	setNonBlocking(int fd);

	Server(const Server&);
	Server& operator=(const Server&);
};

#endif
