#include "Utils.hpp"
#include <sstream>
#include <cctype>
#include <cstdlib>

namespace Utils {

std::vector<std::string>	split(const std::string &s, char delim) {
	std::vector<std::string> out;
	std::string cur;
	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] == delim) {
			if (!cur.empty()) { out.push_back(cur); cur.clear(); }
		} else {
			cur += s[i];
		}
	}
	if (!cur.empty()) out.push_back(cur);
	return out;
}

std::vector<std::string>	splitKeep(const std::string &s, char delim) {
	std::vector<std::string> out;
	std::string cur;
	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] == delim) { out.push_back(cur); cur.clear(); }
		else cur += s[i];
	}
	out.push_back(cur);
	return out;
}

std::string	trim(const std::string &s) {
	size_t a = 0, b = s.size();
	while (a < b && (s[a] == ' ' || s[a] == '\r' || s[a] == '\n' || s[a] == '\t')) ++a;
	while (b > a && (s[b-1] == ' ' || s[b-1] == '\r' || s[b-1] == '\n' || s[b-1] == '\t')) --b;
	return s.substr(a, b - a);
}

std::string	toUpper(const std::string &s) {
	std::string r = s;
	for (size_t i = 0; i < r.size(); ++i)
		r[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(r[i])));
	return r;
}

bool	iequals(const std::string &a, const std::string &b) {
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); ++i)
		if (std::tolower(static_cast<unsigned char>(a[i])) !=
			std::tolower(static_cast<unsigned char>(b[i])))
			return false;
	return true;
}

bool	isValidNick(const std::string &n) {
	if (n.empty() || n.size() > 30) return false;
	char c = n[0];
	if (!std::isalpha(static_cast<unsigned char>(c)) &&
		c != '[' && c != ']' && c != '\\' && c != '`' &&
		c != '_' && c != '^' && c != '{' && c != '}' && c != '|')
		return false;
	for (size_t i = 1; i < n.size(); ++i) {
		char x = n[i];
		if (!std::isalnum(static_cast<unsigned char>(x)) &&
			x != '[' && x != ']' && x != '\\' && x != '`' &&
			x != '_' && x != '^' && x != '{' && x != '}' && x != '|' && x != '-')
			return false;
	}
	return true;
}

bool	isValidChannelName(const std::string &n) {
	if (n.size() < 2 || n.size() > 50) return false;
	if (n[0] != '#' && n[0] != '&') return false;
	for (size_t i = 1; i < n.size(); ++i) {
		char x = n[i];
		if (x == ' ' || x == ',' || x == 7 || x == '\r' || x == '\n' || x == ':')
			return false;
	}
	return true;
}

std::string	itos(long v) {
	std::ostringstream oss;
	oss << v;
	return oss.str();
}

long	atoiSafe(const std::string &s) {
	if (s.empty()) return -1;
	for (size_t i = 0; i < s.size(); ++i)
		if (!std::isdigit(static_cast<unsigned char>(s[i]))) return -1;
	return std::atol(s.c_str());
}

} // namespace Utils
