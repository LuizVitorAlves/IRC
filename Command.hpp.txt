#ifndef COMMAND_HPP
# define COMMAND_HPP

# include <string>
# include <vector>
# include <map>

class Server;
class Client;

struct ParsedMsg {
	std::string					prefix;
	std::string					command;
	std::vector<std::string>	params;
};

class CommandDispatcher {
public:
	CommandDispatcher(Server &srv);
	~CommandDispatcher();

	void	handleLine(Client *c, const std::string &line);

private:
	Server	&_srv;

	static ParsedMsg	parse(const std::string &line);

	// registration / auth
	void	cmdPass(Client *c, ParsedMsg &m);
	void	cmdNick(Client *c, ParsedMsg &m);
	void	cmdUser(Client *c, ParsedMsg &m);
	void	cmdPing(Client *c, ParsedMsg &m);
	void	cmdPong(Client *c, ParsedMsg &m);
	void	cmdQuit(Client *c, ParsedMsg &m);
	void	cmdCap(Client *c, ParsedMsg &m);

	// channel
	void	cmdJoin(Client *c, ParsedMsg &m);
	void	cmdPart(Client *c, ParsedMsg &m);
	void	cmdPrivmsg(Client *c, ParsedMsg &m);
	void	cmdNotice(Client *c, ParsedMsg &m);
	void	cmdNames(Client *c, ParsedMsg &m);
	void	cmdWho(Client *c, ParsedMsg &m);

	// operator
	void	cmdKick(Client *c, ParsedMsg &m);
	void	cmdInvite(Client *c, ParsedMsg &m);
	void	cmdTopic(Client *c, ParsedMsg &m);
	void	cmdMode(Client *c, ParsedMsg &m);

	void	tryWelcome(Client *c);
};

#endif
