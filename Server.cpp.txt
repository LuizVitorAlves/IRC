#include "Server.hpp"
#include "Command.hpp"
#include "Utils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <sstream>

volatile sig_atomic_t Server::_stop = 0;

Server::Server(int port, const std::string &password)
	: _port(port), _password(password), _serverName("ircserv.42"), _listenFd(-1) {
	std::signal(SIGINT, Server::handleSignal);
	std::signal(SIGTERM, Server::handleSignal);
	setupListenSocket();
}

Server::~Server() {
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		close(it->first);
		delete it->second;
	}
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		delete it->second;
	if (_listenFd >= 0) close(_listenFd);
}

void	Server::handleSignal(int) { _stop = 1; }

const std::string&	Server::password() const { return _password; }
const std::string&	Server::serverName() const { return _serverName; }
std::map<int, Client*>&	Server::clients() { return _clients; }
std::map<std::string, Channel*>&	Server::channels() { return _channels; }

Client*	Server::findClientByNick(const std::string &nick) {
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (Utils::iequals(it->second->nick(), nick)) return it->second;
	}
	return NULL;
}

Channel*	Server::findChannel(const std::string &name) {
	std::map<std::string, Channel*>::iterator it = _channels.find(Utils::toUpper(name));
	if (it == _channels.end()) return NULL;
	return it->second;
}

Channel*	Server::getOrCreateChannel(const std::string &name) {
	std::string key = Utils::toUpper(name);
	std::map<std::string, Channel*>::iterator it = _channels.find(key);
	if (it != _channels.end()) return it->second;
	Channel *c = new Channel(name);
	_channels[key] = c;
	return c;
}

void	Server::removeChannel(const std::string &name) {
	std::string key = Utils::toUpper(name);
	std::map<std::string, Channel*>::iterator it = _channels.find(key);
	if (it != _channels.end()) {
		delete it->second;
		_channels.erase(it);
	}
}

void	Server::setNonBlocking(int fd) {
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl O_NONBLOCK failed");
}

void	Server::setupListenSocket() {
	_listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenFd < 0) throw std::runtime_error("socket() failed");

	int yes = 1;
	if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
		throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");

	setNonBlocking(_listenFd);

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(static_cast<uint16_t>(_port));

	if (bind(_listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("bind() failed");

	if (listen(_listenFd, SOMAXCONN) < 0)
		throw std::runtime_error("listen() failed");

	struct pollfd p;
	p.fd = _listenFd;
	p.events = POLLIN;
	p.revents = 0;
	_pfds.push_back(p);

	std::cout << "ircserv listening on port " << _port << std::endl;
}

void	Server::acceptNewClient() {
	struct sockaddr_in cli;
	socklen_t len = sizeof(cli);
	int cfd = accept(_listenFd, (struct sockaddr*)&cli, &len);
	if (cfd < 0) return;

	setNonBlocking(cfd);

	char hostbuf[INET_ADDRSTRLEN];
	if (!inet_ntop(AF_INET, &cli.sin_addr, hostbuf, sizeof(hostbuf)))
		std::strcpy(hostbuf, "unknown");

	Client *c = new Client(cfd, std::string(hostbuf));
	_clients[cfd] = c;

	struct pollfd p;
	p.fd = cfd;
	p.events = POLLIN;
	p.revents = 0;
	_pfds.push_back(p);

	std::cout << "[+] client fd=" << cfd << " from " << hostbuf << std::endl;
}

void	Server::handleClientRead(int fd) {
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client *c = it->second;

	char buf[4096];
	ssize_t n = recv(fd, buf, sizeof(buf), 0);
	if (n <= 0) {
		if (n == 0) {
			c->markForDeletion();
		} else {
			// non-blocking: EAGAIN/EWOULDBLOCK => try later
			if (errno != EAGAIN && errno != EWOULDBLOCK)
				c->markForDeletion();
		}
		return;
	}
	c->readBuf().append(buf, static_cast<size_t>(n));
	processBuffer(c);
}

void	Server::processBuffer(Client *c) {
	static CommandDispatcher dispatcher(*this);
	std::string &rb = c->readBuf();
	size_t pos;
	while ((pos = rb.find('\n')) != std::string::npos) {
		std::string line = rb.substr(0, pos);
		rb.erase(0, pos + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty()) continue;
		dispatcher.handleLine(c, line);
		if (c->markedForDeletion()) return;
	}
}

void	Server::handleClientWrite(int fd) {
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client *c = it->second;
	if (c->writeBuf().empty()) return;

	ssize_t n = send(fd, c->writeBuf().c_str(), c->writeBuf().size(), 0);
	if (n <= 0) {
		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
		c->markForDeletion();
		return;
	}
	c->writeBuf().erase(0, static_cast<size_t>(n));
}

void	Server::disconnectClient(int fd, const std::string &reason) {
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end()) return;
	Client *c = it->second;

	// remove from all channels
	std::set<std::string> chans = c->channels();
	std::string quitMsg = ":" + c->prefix() + " QUIT :" + reason + "\r\n";
	for (std::set<std::string>::iterator ch = chans.begin(); ch != chans.end(); ++ch) {
		Channel *chan = findChannel(*ch);
		if (chan) {
			chan->removeMember(c);
			chan->broadcast(quitMsg, c);
			if (chan->isEmpty()) removeChannel(chan->name());
		}
	}
	std::cout << "[-] client fd=" << fd << " (" << c->nick() << ") disconnected: "
			  << reason << std::endl;

	close(fd);
	delete c;
	_clients.erase(it);
	for (std::vector<struct pollfd>::iterator p = _pfds.begin(); p != _pfds.end(); ++p) {
		if (p->fd == fd) { _pfds.erase(p); break; }
	}
}

void	Server::run() {
	while (!_stop) {
		// arm POLLOUT only when there is data to write
		for (size_t i = 0; i < _pfds.size(); ++i) {
			int fd = _pfds[i].fd;
			_pfds[i].events = POLLIN;
			std::map<int, Client*>::iterator it = _clients.find(fd);
			if (it != _clients.end() && !it->second->writeBuf().empty())
				_pfds[i].events |= POLLOUT;
		}

		int rc = poll(&_pfds[0], _pfds.size(), 1000);
		if (rc < 0) {
			if (errno == EINTR) continue;
			throw std::runtime_error("poll() failed");
		}
		if (rc == 0) continue;

		// snapshot to avoid iterator invalidation on disconnect
		std::vector<struct pollfd> snap = _pfds;
		for (size_t i = 0; i < snap.size(); ++i) {
			short re = snap[i].revents;
			int fd = snap[i].fd;
			if (re == 0) continue;

			if (fd == _listenFd) {
				if (re & POLLIN) acceptNewClient();
				continue;
			}
			if (re & (POLLERR | POLLHUP | POLLNVAL)) {
				disconnectClient(fd, "Connection closed");
				continue;
			}
			if (re & POLLIN) handleClientRead(fd);
			if (_clients.find(fd) == _clients.end()) continue;
			if (re & POLLOUT) handleClientWrite(fd);
			if (_clients.find(fd) != _clients.end() &&
				_clients[fd]->markedForDeletion() &&
				_clients[fd]->writeBuf().empty())
				disconnectClient(fd, "Client quit");
		}
	}
	std::cout << "\nShutting down..." << std::endl;
}
