#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Utils.hpp"
#include "Replies.hpp"

CommandDispatcher::CommandDispatcher(Server &srv) : _srv(srv) {}
CommandDispatcher::~CommandDispatcher() {}

// IRC message parser: [":"prefix SPACE] command *(SPACE param) [SPACE ":"trailing]
ParsedMsg	CommandDispatcher::parse(const std::string &line) {
	ParsedMsg m;
	size_t i = 0;
	const size_t n = line.size();

	if (n > 0 && line[0] == ':') {
		size_t sp = line.find(' ', 1);
		if (sp == std::string::npos) { m.prefix = line.substr(1); return m; }
		m.prefix = line.substr(1, sp - 1);
		i = sp + 1;
	}
	// command
	while (i < n && line[i] == ' ') ++i;
	size_t cmdStart = i;
	while (i < n && line[i] != ' ') ++i;
	m.command = Utils::toUpper(line.substr(cmdStart, i - cmdStart));

	// params
	while (i < n) {
		while (i < n && line[i] == ' ') ++i;
		if (i >= n) break;
		if (line[i] == ':') {
			m.params.push_back(line.substr(i + 1));
			break;
		}
		size_t p = i;
		while (i < n && line[i] != ' ') ++i;
		m.params.push_back(line.substr(p, i - p));
	}
	return m;
}

void	CommandDispatcher::handleLine(Client *c, const std::string &line) {
	ParsedMsg m = parse(line);
	if (m.command.empty()) return;

	const std::string &cmd = m.command;

	// commands allowed pre-registration
	if (cmd == "CAP")	{ cmdCap(c, m); return; }
	if (cmd == "PASS")	{ cmdPass(c, m); return; }
	if (cmd == "NICK")	{ cmdNick(c, m); return; }
	if (cmd == "USER")	{ cmdUser(c, m); return; }
	if (cmd == "QUIT")	{ cmdQuit(c, m); return; }
	if (cmd == "PING")	{ cmdPing(c, m); return; }
	if (cmd == "PONG")	{ cmdPong(c, m); return; }

	// must be registered from here on
	if (!c->registered()) {
		c->appendWrite(":" + _srv.serverName() + " " + ERR_NOTREGISTERED
			+ " * :You have not registered\r\n");
		return;
	}

	if (cmd == "JOIN")		cmdJoin(c, m);
	else if (cmd == "PART")		cmdPart(c, m);
	else if (cmd == "PRIVMSG")	cmdPrivmsg(c, m);
	else if (cmd == "NOTICE")	cmdNotice(c, m);
	else if (cmd == "KICK")		cmdKick(c, m);
	else if (cmd == "INVITE")	cmdInvite(c, m);
	else if (cmd == "TOPIC")	cmdTopic(c, m);
	else if (cmd == "MODE")		cmdMode(c, m);
	else if (cmd == "NAMES")	cmdNames(c, m);
	else if (cmd == "WHO")		cmdWho(c, m);
	else {
		c->appendWrite(":" + _srv.serverName() + " " + ERR_UNKNOWNCOMMAND
			+ " " + c->nick() + " " + cmd + " :Unknown command\r\n");
	}
}
