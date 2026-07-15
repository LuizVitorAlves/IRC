#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Utils.hpp"
#include "Replies.hpp"
#include <cstdlib>

void	CommandDispatcher::cmdKick(Client *c, ParsedMsg &m) {
	const std::string &sn = _srv.serverName();
	const std::string &nk = c->nick();
	if (m.params.size() < 2) {
		c->appendWrite(":" + sn + " " + ERR_NEEDMOREPARAMS + " " + nk
			+ " KICK :Not enough parameters\r\n");
		return;
	}
	Channel *ch = _srv.findChannel(m.params[0]);
	if (!ch) {
		c->appendWrite(":" + sn + " " + ERR_NOSUCHCHANNEL + " " + nk + " "
			+ m.params[0] + " :No such channel\r\n");
		return;
	}
	if (!ch->hasMember(c)) {
		c->appendWrite(":" + sn + " " + ERR_NOTONCHANNEL + " " + nk + " "
			+ ch->name() + " :You're not on that channel\r\n");
		return;
	}
	if (!ch->isOperator(c)) {
		c->appendWrite(":" + sn + " " + ERR_CHANOPRIVSNEEDED + " " + nk + " "
			+ ch->name() + " :You're not channel operator\r\n");
		return;
	}
	std::string reason = (m.params.size() >= 3) ? m.params[2] : nk;
	std::vector<std::string> targets = Utils::splitKeep(m.params[1], ',');
	for (size_t i = 0; i < targets.size(); ++i) {
		Client *victim = _srv.findClientByNick(targets[i]);
		if (!victim || !ch->hasMember(victim)) {
			c->appendWrite(":" + sn + " " + ERR_USERNOTINCHANNEL + " " + nk + " "
				+ targets[i] + " " + ch->name() + " :They aren't on that channel\r\n");
			continue;
		}
		std::string msg = ":" + c->prefix() + " KICK " + ch->name() + " "
			+ victim->nick() + " :" + reason + "\r\n";
		ch->broadcast(msg, NULL);
		ch->removeMember(victim);
		victim->channels().erase(Utils::toUpper(ch->name()));
		if (ch->isEmpty()) { _srv.removeChannel(ch->name()); return; }
	}
}

void	CommandDispatcher::cmdInvite(Client *c, ParsedMsg &m) {
	const std::string &sn = _srv.serverName();
	const std::string &nk = c->nick();
	if (m.params.size() < 2) {
		c->appendWrite(":" + sn + " " + ERR_NEEDMOREPARAMS + " " + nk
			+ " INVITE :Not enough parameters\r\n");
		return;
	}
	Client *target = _srv.findClientByNick(m.params[0]);
	if (!target) {
		c->appendWrite(":" + sn + " " + ERR_NOSUCHNICK + " " + nk + " "
			+ m.params[0] + " :No such nick\r\n");
		return;
	}
	Channel *ch = _srv.findChannel(m.params[1]);
	if (!ch) {
		c->appendWrite(":" + sn + " " + ERR_NOSUCHCHANNEL + " " + nk + " "
			+ m.params[1] + " :No such channel\r\n");
		return;
	}
	if (!ch->hasMember(c)) {
		c->appendWrite(":" + sn + " " + ERR_NOTONCHANNEL + " " + nk + " "
			+ ch->name() + " :You're not on that channel\r\n");
		return;
	}
	if (ch->inviteOnly() && !ch->isOperator(c)) {
		c->appendWrite(":" + sn + " " + ERR_CHANOPRIVSNEEDED + " " + nk + " "
			+ ch->name() + " :You're not channel operator\r\n");
		return;
	}
	if (ch->hasMember(target)) {
		c->appendWrite(":" + sn + " " + ERR_USERONCHANNEL + " " + nk + " "
			+ target->nick() + " " + ch->name() + " :is already on channel\r\n");
		return;
	}
	ch->inviteNick(target->nick());
	c->appendWrite(":" + sn + " " + RPL_INVITING + " " + nk + " "
		+ target->nick() + " " + ch->name() + "\r\n");
	target->appendWrite(":" + c->prefix() + " INVITE " + target->nick()
		+ " :" + ch->name() + "\r\n");
}

void	CommandDispatcher::cmdTopic(Client *c, ParsedMsg &m) {
	const std::string &sn = _srv.serverName();
	const std::string &nk = c->nick();
	if (m.params.empty()) {
		c->appendWrite(":" + sn + " " + ERR_NEEDMOREPARAMS + " " + nk
			+ " TOPIC :Not enough parameters\r\n");
		return;
	}
	Channel *ch = _srv.findChannel(m.params[0]);
	if (!ch) {
		c->appendWrite(":" + sn + " " + ERR_NOSUCHCHANNEL + " " + nk + " "
			+ m.params[0] + " :No such channel\r\n");
		return;
	}
	if (!ch->hasMember(c)) {
		c->appendWrite(":" + sn + " " + ERR_NOTONCHANNEL + " " + nk + " "
			+ ch->name() + " :You're not on that channel\r\n");
		return;
	}
	if (m.params.size() < 2) {
		if (ch->topic().empty()) {
			c->appendWrite(":" + sn + " " + RPL_NOTOPIC + " " + nk + " "
				+ ch->name() + " :No topic is set\r\n");
		} else {
			c->appendWrite(":" + sn + " " + RPL_TOPIC + " " + nk + " "
				+ ch->name() + " :" + ch->topic() + "\r\n");
		}
		return;
	}
	if (ch->topicRestricted() && !ch->isOperator(c)) {
		c->appendWrite(":" + sn + " " + ERR_CHANOPRIVSNEEDED + " " + nk + " "
			+ ch->name() + " :You're not channel operator\r\n");
		return;
	}
	ch->setTopic(m.params[1]);
	std::string msg = ":" + c->prefix() + " TOPIC " + ch->name() + " :"
		+ m.params[1] + "\r\n";
	ch->broadcast(msg, NULL);
}
