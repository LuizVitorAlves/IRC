#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <set>

class Client {
public:
	Client(int fd, const std::string &host);
	~Client();

	int					fd() const;
	const std::string&	host() const;
	const std::string&	nick() const;
	const std::string&	user() const;
	const std::string&	realname() const;
	const std::string&	readBuf() const;
	std::string&		readBuf();
	const std::string&	writeBuf() const;
	std::string&		writeBuf();

	bool	passOk() const;
	bool	nickSet() const;
	bool	userSet() const;
	bool	registered() const;
	bool	markedForDeletion() const;

	void	setPassOk();
	void	setNick(const std::string &n);
	void	setUser(const std::string &u, const std::string &real);
	void	setRegistered();
	void	markForDeletion();

	std::string	prefix() const; // nick!user@host

	void	appendWrite(const std::string &data);
	std::set<std::string>&	channels();

private:
	int			_fd;
	std::string	_host;
	std::string	_nick;
	std::string	_user;
	std::string	_realname;
	std::string	_readBuf;
	std::string	_writeBuf;
	bool		_passOk;
	bool		_nickSet;
	bool		_userSet;
	bool		_registered;
	bool		_delete;
	std::set<std::string>	_channels;
};

#endif
