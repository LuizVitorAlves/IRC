#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Utils.hpp"
#include "Replies.hpp"

void	CommandDispatcher::cmdJoin(Client *c, ParsedMsg &m) {
	const std::string &sn = _srv.serverName();
	const std::string &nk = c->nick();

	if (m.params.empty()) {
		c->appendWrite(":" + sn + " " + ERR_NEEDMOREPARAMS + " " + nk
			+ " JOIN :Not enough parameters\r\n");
		return;
	}
	std::vector<std::string> chans = Utils::splitKeep(m.params[0], ',');
	std::vector<std::string> keys;
	if (m.params.size() >= 2) keys = Utils::splitKeep(m.params[1], ',');

	for (size_t i = 0; i < chans.size(); ++i) {
		std::string chname = chans[i];
		if (!Utils::isValidChannelName(chname)) {
			c->appendWrite(":" + sn + " " + ERR_NOSUCHCHANNEL + " " + nk
				+ " " + chname + " :No such channel\r\n");
			continue;
		}
		std::string providedKey = (i < keys.size()) ? keys[i] : std::string("");
		Channel *ch = _srv.findChannel(chname);
		bool created = false;
		if (!ch) {
			ch = _srv.getOrCreateChannel(chname);
			created = true;
		} else {
			// checks
			if (ch->hasMember(c)) continue;
			if (ch->inviteOnly() && !ch->isInvited(nk)) {
				c->appendWrite(":" + sn + " " + ERR_INVITEONLYCHAN + " " + nk
					+ " " + chname + " :Cannot join channel (+i)\r\n");
				continue;
			}
			if (ch->hasKey() && providedKey != ch->key()) {
				c->appendWrite(":" + sn + " " + ERR_BADCHANNELKEY + " " + nk
					+ " " + chname + " :Cannot join channel (+k)\r\n");
				continue;
			}
			if (ch->hasUserLimit() && ch->members().size() >= ch->userLimit()) {
				c->appendWrite(":" + sn + " " + ERR_CHANNELISFULL + " " + nk
					+ " " + chname + " :Cannot join channel (+l)\r\n");
				continue;
			}
		}
		ch->addMember(c);
		if (created) ch->addOperator(c);
		ch->uninviteNick(nk);
		c->channels().insert(Utils::toUpper(chname));

		std::string joinMsg = ":" + c->prefix() + " JOIN :" + ch->name() + "\r\n";
		ch->broadcast(joinMsg, NULL);

		if (!ch->topic().empty()) {
			c->appendWrite(":" + sn + " " + RPL_TOPIC + " " + nk + " " + ch->name()
				+ " :" + ch->topic() + "\r\n");
		} else {
			c->appendWrite(":" + sn + " " + RPL_NOTOPIC + " " + nk + " " + ch->name()
				+ " :No topic is set\r\n");
		}
		c->appendWrite(":" + sn + " " + RPL_NAMREPLY + " " + nk + " = " + ch->name()
			+ " :" + ch->namesList() + "\r\n");
		c->appendWrite(":" + sn + " " + RPL_ENDOFNAMES + " " + nk + " " + ch->name()
			+ " :End of /NAMES list\r\n");
	}
}

void	CommandDispatcher::cmdPart(Client *c, ParsedMsg &m) {
	const std::string &sn = _srv.serverName();
	const std::string &nk = c->nick();
	if (m.params.empty()) {
		c->appendWrite(":" + sn + " " + ERR_NEEDMOREPARAMS + " " + nk
			+ " PART :Not enough parameters\r\n");
		return;
	}
	std::string reason = (m.params.size() >= 2) ? m.params[1] : std::string("");
	std::vector<std::string> chans = Utils::splitKeep(m.params[0], ',');
	for (size_t i = 0; i < chans.size(); ++i) {
		Channel *ch = _srv.findChannel(chans[i]);
		if (!ch) {
			c->appendWrite(":" + sn + " " + ERR_NOSUCHCHANNEL + " " + nk + " "
				+ chans[i] + " :No such channel\r\n");
			continue;
		}
		if (!ch->hasMember(c)) {
			c->appendWrite(":" + sn + " " + ERR_NOTONCHANNEL + " " + nk + " "
				+ ch->name() + " :You're not on that channel\r\n");
			continue;
		}
		std::string msg = ":" + c->prefix() + " PART " + ch->name();
		if (!reason.empty()) msg += " :" + reason;
		msg += "\r\n";
		ch->broadcast(msg, NULL);
		ch->removeMember(c);
		c->channels().erase(Utils::toUpper(ch->name()));
		if (ch->isEmpty()) _srv.removeChannel(ch->name());
	}
}

static void	sendToTarget(Server &srv, Client *c, const std::string &target,
						 const std::string &kind, const std::string &text,
						 bool errors) {
	const std::string &sn = srv.serverName();
	const std::string &nk = c->nick();
	if (target.empty()) return;
	if (target[0] == '#' || target[0] == '&') {
		Channel *ch = srv.findChannel(target);
		if (!ch) {
			if (errors)
				c->appendWrite(":" + sn + " " + ERR_NOSUCHCHANNEL + " " + nk + " "
					+ target + " :No such channel\r\n");
			return;
		}
		if (!ch->hasMember(c)) {
			if (errors)
				c->appendWrite(":" + sn + " " + ERR_CANNOTSENDTOCHAN + " " + nk + " "
					+ ch->name() + " :Cannot send to channel\r\n");
			return;
		}
		std::string msg = ":" + c->prefix() + " " + kind + " " + ch->name()
			+ " :" + text + "\r\n";
		ch->broadcast(msg, c);
	} else {
		Client *dest = srv.findClientByNick(target);
		if (!dest) {
			if (errors)
				c->appendWrite(":" + sn + " " + ERR_NOSUCHNICK + " " + nk + " "
					+ target + " :No such nick/channel\r\n");
			return;
		}
		dest->appendWrite(":" + c->prefix() + " " + kind + " "
			+ dest->nick() + " :" + text + "\r\n");
	}
}

void	CommandDispatcher::cmdPrivmsg(Client *c, ParsedMsg &m) {
	const std::string &sn = _srv.serverName();
	const std::string &nk = c->nick();
	if (m.params.empty()) {
		c->appendWrite(":" + sn + " " + ERR_NORECIPIENT + " " + nk
			+ " :No recipient given (PRIVMSG)\r\n");
		return;
	}
	if (m.params.size() < 2 || m.params[1].empty()) {
		c->appendWrite(":" + sn + " " + ERR_NOTEXTTOSEND + " " + nk
			+ " :No text to send\r\n");
		return;
	}
	std::vector<std::string> targets = Utils::splitKeep(m.params[0], ',');
	for (size_t i = 0; i < targets.size(); ++i)
		sendToTarget(_srv, c, targets[i], "PRIVMSG", m.params[1], true);
}

void	CommandDispatcher::cmdNotice(Client *c, ParsedMsg &m) {
	if (m.params.size() < 2 || m.params[1].empty()) return;
	std::vector<std::string> targets = Utils::splitKeep(m.params[0], ',');
	for (size_t i = 0; i < targets.size(); ++i)
		sendToTarget(_srv, c, targets[i], "NOTICE", m.params[1], false);
}
