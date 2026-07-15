#include "Client.hpp"

Client::Client(int fd, const std::string &host)
	: _fd(fd), _host(host), _nick(""), _user(""), _realname(""),
	  _readBuf(""), _writeBuf(""),
	  _passOk(false), _nickSet(false), _userSet(false),
	  _registered(false), _delete(false) {}

Client::~Client() {}

int					Client::fd() const { return _fd; }
const std::string&	Client::host() const { return _host; }
const std::string&	Client::nick() const { return _nick; }
const std::string&	Client::user() const { return _user; }
const std::string&	Client::realname() const { return _realname; }
const std::string&	Client::readBuf() const { return _readBuf; }
std::string&		Client::readBuf() { return _readBuf; }
const std::string&	Client::writeBuf() const { return _writeBuf; }
std::string&		Client::writeBuf() { return _writeBuf; }

bool	Client::passOk() const { return _passOk; }
bool	Client::nickSet() const { return _nickSet; }
bool	Client::userSet() const { return _userSet; }
bool	Client::registered() const { return _registered; }
bool	Client::markedForDeletion() const { return _delete; }

void	Client::setPassOk() { _passOk = true; }

void	Client::setNick(const std::string &n) {
	_nick = n;
	_nickSet = true;
}

void	Client::setUser(const std::string &u, const std::string &real) {
	_user = u;
	_realname = real;
	_userSet = true;
}

void	Client::setRegistered() { _registered = true; }
void	Client::markForDeletion() { _delete = true; }

std::string	Client::prefix() const {
	std::string p = _nick.empty() ? std::string("*") : _nick;
	p += "!";
	p += _user.empty() ? std::string("*") : _user;
	p += "@";
	p += _host;
	return p;
}

void	Client::appendWrite(const std::string &data) {
	_writeBuf += data;
}

std::set<std::string>&	Client::channels() { return _channels; }
