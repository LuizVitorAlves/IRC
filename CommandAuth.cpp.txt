#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Utils.hpp"
#include "Replies.hpp"

// CAP: minimal LS handling so clients like irssi/HexChat/WeeChat complete negotiation
void	CommandDispatcher::cmdCap(Client *c, ParsedMsg &m) {
	if (m.params.empty()) return;
	std::string sub = Utils::toUpper(m.params[0]);
	if (sub == "LS")
		c->appendWrite(":" + _srv.serverName() + " CAP * LS :\r\n");
	else if (sub == "REQ")
		c->appendWrite(":" + _srv.serverName() + " CAP * NAK :\r\n");
	else if (sub == "END")
		return;
}

void	CommandDispatcher::cmdPass(Client *c, ParsedMsg &m) {
	if (c->registered()) {
		c->appendWrite(":" + _srv.serverName() + " " + ERR_ALREADYREGISTRED
			+ " * :You may not reregister\r\n");
		return;
	}
	if (m.params.empty()) {
		c->appendWrite(":" + _srv.serverName() + " " + ERR_NEEDMOREPARAMS
			+ " * PASS :Not enough parameters\r\n");
		return;
	}
	if (m.params[0] != _srv.password()) {
		c->appendWrite(":" + _srv.serverName() + " " + ERR_PASSWDMISMATCH
			+ " * :Password incorrect\r\n");
		c->markForDeletion();
		return;
	}
	c->setPassOk();
}

void	CommandDispatcher::cmdNick(Client *c, ParsedMsg &m) {
	if (m.params.empty()) {
		c->appendWrite(":" + _srv.serverName() + " " + ERR_NONICKNAMEGIVEN
			+ " * :No nickname given\r\n");
		return;
	}
	if (!c->passOk()) {
		c->appendWrite(":" + _srv.serverName() + " " + ERR_PASSWDMISMATCH
			+ " * :Password required\r\n");
		c->markForDeletion();
		return;
	}
	std::string newNick = m.params[0];
	if (!Utils::isValidNick(newNick)) {
		c->appendWrite(":" + _srv.serverName() + " " + ERR_ERRONEUSNICKNAME
			+ " * " + newNick + " :Erroneous nickname\r\n");
		return;
	}
	Client *other = _srv.findClientByNick(newNick);
	if (other && other != c) {
		c->appendWrite(":" + _srv.serverName() + " " + ERR_NICKNAMEINUSE
			+ " * " + newNick + " :Nickname is already in use\r\n");
		return;
	}
	std::string oldPrefix = c->prefix();
	std::string oldNick = c->nick();
	c->setNick(newNick);
	if (c->registered()) {
		std::string msg = ":" + oldPrefix + " NICK :" + newNick + "\r\n";
		c->appendWrite(msg);
		// notify shared channels
		std::set<std::string> &chans = c->channels();
		for (std::set<std::string>::iterator it = chans.begin(); it != chans.end(); ++it) {
			Channel *ch = _srv.findChannel(*it);
			if (ch) ch->broadcast(msg, c);
		}
	} else {
		tryWelcome(c);
	}
	(void)oldNick;
}

void	CommandDispatcher::cmdUser(Client *c, ParsedMsg &m) {
	if (c->registered()) {
		c->appendWrite(":" + _srv.serverName() + " " + ERR_ALREADYREGISTRED
			+ " " + c->nick() + " :You may not reregister\r\n");
		return;
	}
	if (m.params.size() < 4) {
		c->appendWrite(":" + _srv.serverName() + " " + ERR_NEEDMOREPARAMS
			+ " * USER :Not enough parameters\r\n");
		return;
	}
	c->setUser(m.params[0], m.params[3]);
	tryWelcome(c);
}

void	CommandDispatcher::cmdPing(Client *c, ParsedMsg &m) {
	std::string token = m.params.empty() ? std::string("") : m.params[0];
	c->appendWrite(":" + _srv.serverName() + " PONG " + _srv.serverName()
		+ " :" + token + "\r\n");
}

void	CommandDispatcher::cmdPong(Client *, ParsedMsg &) {
	// no-op
}

void	CommandDispatcher::cmdQuit(Client *c, ParsedMsg &m) {
	std::string reason = m.params.empty() ? std::string("Client quit") : m.params[0];
	// signal to run loop that this client must be dropped once buffer flushes
	c->appendWrite("ERROR :Closing link: " + reason + "\r\n");
	c->markForDeletion();
}

void	CommandDispatcher::tryWelcome(Client *c) {
	if (c->registered()) return;
	if (!(c->passOk() && c->nickSet() && c->userSet())) return;
	c->setRegistered();

	const std::string &sn = _srv.serverName();
	const std::string &nk = c->nick();
	c->appendWrite(":" + sn + " " + RPL_WELCOME + " " + nk
		+ " :Welcome to the ft_irc network, " + c->prefix() + "\r\n");
	c->appendWrite(":" + sn + " " + RPL_YOURHOST + " " + nk
		+ " :Your host is " + sn + ", running ircserv-1.0\r\n");
	c->appendWrite(":" + sn + " " + RPL_CREATED + " " + nk
		+ " :This server was created for the 42 curriculum\r\n");
	c->appendWrite(":" + sn + " " + RPL_MYINFO + " " + nk
		+ " " + sn + " ircserv-1.0 o itkol\r\n");
}
