#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <vector>

namespace Utils {
	// split by delimiter char, no empty tokens
	std::vector<std::string>	split(const std::string &s, char delim);
	// split by delimiter char, keep empty tokens (for CSV-like JOIN #a,#b)
	std::vector<std::string>	splitKeep(const std::string &s, char delim);
	// trim CR / LF / spaces on both ends
	std::string	trim(const std::string &s);
	// case-insensitive compare (IRC channel/nick case rules)
	bool	iequals(const std::string &a, const std::string &b);
	// uppercase copy (ASCII)
	std::string	toUpper(const std::string &s);
	// validation
	bool	isValidNick(const std::string &n);
	bool	isValidChannelName(const std::string &n);
	// int -> string
	std::string	itos(long v);
	// safe atoi (returns -1 on failure)
	long	atoiSafe(const std::string &s);
}

#endif
