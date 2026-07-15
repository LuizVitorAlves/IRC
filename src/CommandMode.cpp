#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Utils.hpp"
#include "Replies.hpp"
#include <cstdlib>
#include <sstream>

// MODE <channel> [modestring [mode arguments...]]
// Supported channel modes: i, t, k, o, l
void	CommandDispatcher::cmdMode(Client *c, ParsedMsg &m) {
	const std::string &sn = _srv.serverName();
	const std::string &nk = c->nick();

	if (m.params.empty()) {
		c->appendWrite(":" + sn + " " + ERR_NEEDMOREPARAMS + " " + nk
			+ " MODE :Not enough parameters\r\n");
		return;
	}

	const std::string &target = m.params[0];

	// user modes: silently ignore MODE <nick> queries so clients don't error out
	if (target.empty() || (target[0] != '#' && target[0] != '&')) {
		if (Utils::iequals(target, nk) && m.params.size() == 1) {
			c->appendWrite(":" + sn + " " + RPL_UMODEIS + " " + nk + " +\r\n");
		}
		return;
	}

	Channel *ch = _srv.findChannel(target);
	if (!ch) {
		c->appendWrite(":" + sn + " " + ERR_NOSUCHCHANNEL + " " + nk + " "
			+ target + " :No such channel\r\n");
		return;
	}

	// query current modes
	if (m.params.size() == 1) {
		std::string modes = "+";
		std::string args;
		if (ch->inviteOnly()) modes += "i";
		if (ch->topicRestricted()) modes += "t";
		if (ch->hasKey()) { modes += "k"; args += " " + ch->key(); }
		if (ch->hasUserLimit()) {
			modes += "l";
			std::ostringstream oss; oss << ch->userLimit();
			args += " " + oss.str();
		}
		c->appendWrite(":" + sn + " " + RPL_CHANNELMODEIS + " " + nk + " "
			+ ch->name() + " " + modes + args + "\r\n");
		return;
	}

	if (!ch->isOperator(c)) {
		c->appendWrite(":" + sn + " " + ERR_CHANOPRIVSNEEDED + " " + nk + " "
			+ ch->name() + " :You're not channel operator\r\n");
		return;
	}

	const std::string &modestr = m.params[1];
	size_t argIdx = 2;
	bool adding = true;

	std::string appliedPos;   // characters after '+'
	std::string appliedNeg;   // characters after '-'
	std::vector<std::string> appliedArgs;

	for (size_t i = 0; i < modestr.size(); ++i) {
		char ch1 = modestr[i];
		if (ch1 == '+') { adding = true; continue; }
		if (ch1 == '-') { adding = false; continue; }
		switch (ch1) {
			case 'i':
				ch->setInviteOnly(adding);
				(adding ? appliedPos : appliedNeg) += 'i';
				break;
			case 't':
				ch->setTopicRestricted(adding);
				(adding ? appliedPos : appliedNeg) += 't';
				break;
			case 'k': {
				if (adding) {
					if (argIdx >= m.params.size()) break;
					ch->setKey(m.params[argIdx]);
					appliedPos += 'k';
					appliedArgs.push_back(m.params[argIdx]);
					++argIdx;
				} else {
					ch->clearKey();
					appliedNeg += 'k';
				}
				break;
			}
			case 'l': {
				if (adding) {
					if (argIdx >= m.params.size()) break;
					long lim = Utils::atoiSafe(m.params[argIdx]);
					if (lim > 0) {
						ch->setUserLimit(static_cast<size_t>(lim));
						appliedPos += 'l';
						appliedArgs.push_back(m.params[argIdx]);
					}
					++argIdx;
				} else {
					ch->clearUserLimit();
					appliedNeg += 'l';
				}
				break;
			}
			case 'o': {
				if (argIdx >= m.params.size()) break;
				Client *tgt = _srv.findClientByNick(m.params[argIdx]);
				if (!tgt || !ch->hasMember(tgt)) {
					c->appendWrite(":" + sn + " " + ERR_USERNOTINCHANNEL + " "
						+ nk + " " + m.params[argIdx] + " " + ch->name()
						+ " :They aren't on that channel\r\n");
					++argIdx;
					break;
				}
				if (adding) { ch->addOperator(tgt); appliedPos += 'o'; }
				else { ch->removeOperator(tgt); appliedNeg += 'o'; }
				appliedArgs.push_back(tgt->nick());
				++argIdx;
				break;
			}
			default:
				c->appendWrite(":" + sn + " " + ERR_UNKNOWNMODE + " " + nk + " "
					+ std::string(1, ch1) + " :is unknown mode char to me\r\n");
				break;
		}
	}

	// broadcast applied mode change
	std::string finalStr;
	if (!appliedPos.empty()) finalStr += "+" + appliedPos;
	if (!appliedNeg.empty()) finalStr += "-" + appliedNeg;
	if (finalStr.empty()) return;

	std::string msg = ":" + c->prefix() + " MODE " + ch->name() + " " + finalStr;
	for (size_t i = 0; i < appliedArgs.size(); ++i) msg += " " + appliedArgs[i];
	msg += "\r\n";
	ch->broadcast(msg, NULL);
}
