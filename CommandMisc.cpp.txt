#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Utils.hpp"
#include "Replies.hpp"

void	CommandDispatcher::cmdNames(Client *c, ParsedMsg &m) {
	const std::string &sn = _srv.serverName();
	const std::string &nk = c->nick();
	if (m.params.empty()) {
		// list all channels the client shares
		std::map<std::string, Channel*> &chs = _srv.channels();
		for (std::map<std::string, Channel*>::iterator it = chs.begin(); it != chs.end(); ++it) {
			Channel *ch = it->second;
			c->appendWrite(":" + sn + " " + RPL_NAMREPLY + " " + nk + " = "
				+ ch->name() + " :" + ch->namesList() + "\r\n");
		}
		c->appendWrite(":" + sn + " " + RPL_ENDOFNAMES + " " + nk
			+ " * :End of /NAMES list\r\n");
		return;
	}
	std::vector<std::string> chans = Utils::splitKeep(m.params[0], ',');
	for (size_t i = 0; i < chans.size(); ++i) {
		Channel *ch = _srv.findChannel(chans[i]);
		if (ch) {
			c->appendWrite(":" + sn + " " + RPL_NAMREPLY + " " + nk + " = "
				+ ch->name() + " :" + ch->namesList() + "\r\n");
		}
		c->appendWrite(":" + sn + " " + RPL_ENDOFNAMES + " " + nk + " "
			+ chans[i] + " :End of /NAMES list\r\n");
	}
}

void	CommandDispatcher::cmdWho(Client *c, ParsedMsg &m) {
	const std::string &sn = _srv.serverName();
	const std::string &nk = c->nick();
	if (m.params.empty()) {
		c->appendWrite(":" + sn + " " + RPL_ENDOFWHO + " " + nk
			+ " * :End of /WHO list\r\n");
		return;
	}
	const std::string &target = m.params[0];
	Channel *ch = _srv.findChannel(target);
	if (ch) {
		std::set<Client*> &mem = ch->members();
		for (std::set<Client*>::iterator it = mem.begin(); it != mem.end(); ++it) {
			Client *u = *it;
			std::string flags = "H";
			if (ch->isOperator(u)) flags += "@";
			c->appendWrite(":" + sn + " " + RPL_WHOREPLY + " " + nk + " "
				+ ch->name() + " " + u->user() + " " + u->host() + " " + sn
				+ " " + u->nick() + " " + flags + " :0 " + u->realname() + "\r\n");
		}
	}
	c->appendWrite(":" + sn + " " + RPL_ENDOFWHO + " " + nk + " "
		+ target + " :End of /WHO list\r\n");
}
