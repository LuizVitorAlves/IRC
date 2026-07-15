NAME		= ircserv

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -pedantic -Iinclude

SRCDIR		= src
OBJDIR		= obj

SRCS		= main.cpp \
			  Server.cpp \
			  Client.cpp \
			  Channel.cpp \
			  Command.cpp \
			  CommandAuth.cpp \
			  CommandChannel.cpp \
			  CommandOps.cpp \
			  CommandMode.cpp \
			  CommandMisc.cpp \
			  Utils.cpp

OBJS		= $(SRCS:%.cpp=$(OBJDIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "  ✓ $(NAME) built"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)
	@echo "  ✓ objects cleaned"

fclean: clean
	rm -f $(NAME)
	@echo "  ✓ binary cleaned"

re: fclean all

.PHONY: all clean fclean re
