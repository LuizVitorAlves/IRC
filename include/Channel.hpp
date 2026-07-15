#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <set>
# include <map>

class Client;

class Channel {
public:
	Channel(const std::string &name);
	~Channel();

	const std::string&	name() const;
	const std::string&	topic() const;
	const std::string&	key() const;
	size_t				userLimit() const;

	void	setTopic(const std::string &t);
	void	setKey(const std::string &k);
	void	clearKey();
	void	setUserLimit(size_t l);
	void	clearUserLimit();

	bool	inviteOnly() const;
	bool	topicRestricted() const;
	bool	hasKey() const;
	bool	hasUserLimit() const;

	void	setInviteOnly(bool v);
	void	setTopicRestricted(bool v);

	// member handling
	void	addMember(Client *c);
	void	removeMember(Client *c);
	bool	hasMember(Client *c) const;
	bool	isEmpty() const;
	std::set<Client*>&	members();

	// operator handling
	void	addOperator(Client *c);
	void	removeOperator(Client *c);
	bool	isOperator(Client *c) const;

	// invite handling
	void	inviteNick(const std::string &nick);
	bool	isInvited(const std::string &nick) const;
	void	uninviteNick(const std::string &nick);

	// broadcast
	void	broadcast(const std::string &msg, Client *except);
	std::string	namesList();

private:
	std::string			_name;
	std::string			_topic;
	std::string			_key;
	size_t				_userLimit;
	bool				_inviteOnly;
	bool				_topicRestricted;
	bool				_hasKey;
	bool				_hasLimit;
	std::set<Client*>	_members;
	std::set<Client*>	_operators;
	std::set<std::string>	_invited;
};

#endif
