#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(const std::string &name)
	: _name(name), _topic(""), _key(""), _userLimit(0),
	  _inviteOnly(false), _topicRestricted(false),
	  _hasKey(false), _hasLimit(false) {}

Channel::~Channel() {}

const std::string&	Channel::name() const { return _name; }
const std::string&	Channel::topic() const { return _topic; }
const std::string&	Channel::key() const { return _key; }
size_t				Channel::userLimit() const { return _userLimit; }

void	Channel::setTopic(const std::string &t) { _topic = t; }
void	Channel::setKey(const std::string &k) { _key = k; _hasKey = true; }
void	Channel::clearKey() { _key.clear(); _hasKey = false; }
void	Channel::setUserLimit(size_t l) { _userLimit = l; _hasLimit = true; }
void	Channel::clearUserLimit() { _userLimit = 0; _hasLimit = false; }

bool	Channel::inviteOnly() const { return _inviteOnly; }
bool	Channel::topicRestricted() const { return _topicRestricted; }
bool	Channel::hasKey() const { return _hasKey; }
bool	Channel::hasUserLimit() const { return _hasLimit; }

void	Channel::setInviteOnly(bool v) { _inviteOnly = v; }
void	Channel::setTopicRestricted(bool v) { _topicRestricted = v; }

void	Channel::addMember(Client *c) { _members.insert(c); }

void	Channel::removeMember(Client *c) {
	_members.erase(c);
	_operators.erase(c);
}

bool	Channel::hasMember(Client *c) const {
	return _members.find(c) != _members.end();
}

bool	Channel::isEmpty() const { return _members.empty(); }
std::set<Client*>&	Channel::members() { return _members; }

void	Channel::addOperator(Client *c) { _operators.insert(c); }
void	Channel::removeOperator(Client *c) { _operators.erase(c); }
bool	Channel::isOperator(Client *c) const {
	return _operators.find(c) != _operators.end();
}

void	Channel::inviteNick(const std::string &nick) { _invited.insert(nick); }
bool	Channel::isInvited(const std::string &nick) const {
	return _invited.find(nick) != _invited.end();
}
void	Channel::uninviteNick(const std::string &nick) { _invited.erase(nick); }

void	Channel::broadcast(const std::string &msg, Client *except) {
	for (std::set<Client*>::iterator it = _members.begin(); it != _members.end(); ++it) {
		if (*it != except)
			(*it)->appendWrite(msg);
	}
}

std::string	Channel::namesList() {
	std::string r;
	for (std::set<Client*>::iterator it = _members.begin(); it != _members.end(); ++it) {
		if (!r.empty()) r += " ";
		if (isOperator(*it)) r += "@";
		r += (*it)->nick();
	}
	return r;
}
