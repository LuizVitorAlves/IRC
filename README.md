*This project has been created as part of the 42 curriculum by lalves.*

# ft_irc — Internet Relay Chat server

## Description

`ft_irc` is a minimal but functional IRC server written in **C++98**, following the
subject requirements of the 42 curriculum. It implements the core of the IRC
protocol (RFC 1459 / RFC 2812 subset) so that any standard IRC client
(irssi, HexChat, WeeChat, netcat, etc.) can connect, authenticate, join channels,
and exchange messages.

The server is fully **non-blocking** and uses a **single `poll()`** loop to
multiplex all I/O (accept, read, write) across every connected client — no
forking, no threads, no busy-wait.

### Feature list

- Password-protected connection (`PASS`)
- Nickname and user registration (`NICK`, `USER`)
- Channel management (`JOIN`, `PART`, `NAMES`, `TOPIC`)
- Private and channel messaging (`PRIVMSG`, `NOTICE`)
- Server keep-alive (`PING`, `PONG`)
- Clean disconnect (`QUIT`)
- Operator commands: `KICK`, `INVITE`, `TOPIC`
- Channel modes: `i` (invite-only), `t` (topic restricted), `k` (key/password),
  `o` (operator promotion), `l` (user limit)
- Partial-packet reassembly (`nc -C` split-input test passes)
- Graceful shutdown on `SIGINT` / `SIGTERM`
- `SIGPIPE` ignored so a client dropping mid-send does not kill the server

## Instructions

### Build

```sh
make
```

Builds the `ircserv` binary with `-Wall -Wextra -Werror -std=c++98 -pedantic`.

Other targets:

- `make clean` — remove object files
- `make fclean` — remove object files and the binary
- `make re` — full rebuild

### Run

```sh
./ircserv <port> <password>
```

- `port` — TCP port to listen on (1–65535)
- `password` — connection password required by any client

Example:

```sh
./ircserv 6667 hunter2
```

### Connect

With `irssi`:

```sh
irssi -c 127.0.0.1 -p 6667 -w hunter2 -n mynick
```

With `nc` (raw protocol):

```sh
nc -C 127.0.0.1 6667
PASS hunter2
NICK bob
USER bob 0 * :Bob Example
JOIN #hello
PRIVMSG #hello :hi everyone
```

The `-C` flag makes `nc` send CRLF line endings, which is what IRC expects.
You can even split a command across several `Ctrl+D` bursts (`com`, `man`,
`d\n`) — the server aggregates partial packets before parsing.

## Project layout

```
ircserv/
├── Makefile
├── README.md
├── include/
│   ├── Channel.hpp
│   ├── Client.hpp
│   ├── Command.hpp
│   ├── Replies.hpp
│   ├── Server.hpp
│   └── Utils.hpp
└── src/
    ├── main.cpp
    ├── Server.cpp          # poll loop, accept, read/write, buffers
    ├── Client.cpp          # per-connection state
    ├── Channel.cpp         # members, ops, modes, invited list
    ├── Command.cpp         # IRC parser + dispatcher
    ├── CommandAuth.cpp     # PASS NICK USER PING PONG QUIT CAP
    ├── CommandChannel.cpp  # JOIN PART PRIVMSG NOTICE
    ├── CommandOps.cpp      # KICK INVITE TOPIC
    ├── CommandMode.cpp     # MODE i/t/k/o/l
    ├── CommandMisc.cpp     # NAMES WHO
    └── Utils.cpp           # helpers (split, trim, validation)
```

## Technical choices

- **Single `poll()`** for every fd — including the listening socket — as required
  by the subject.
- **Per-client `readBuf` / `writeBuf`** allow full-duplex non-blocking I/O and
  correct reassembly of partial commands split across TCP segments.
- **`POLLOUT` is armed only when there is pending output**, so the loop does not
  spin on writable sockets when there is nothing to send.
- **`fcntl(fd, F_SETFL, O_NONBLOCK)`** is the only `fcntl` flag combination used,
  matching the subject constraint for macOS portability.
- **Channel and nickname lookups are case-insensitive**, following the IRC RFC.
- **Channel names normalized in uppercase** internally, while the original casing
  is preserved for display.

## Resources

Documentation and references used during development:

- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [Modern IRC Client Protocol reference (ircdocs.horse)](https://modern.ircdocs.horse/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- `man 2 poll`, `man 2 socket`, `man 7 tcp`, `man 3 getaddrinfo`

### AI usage disclosure

AI (Claude) was used as a **thought partner** to:

- Sketch the initial module layout (Server / Client / Channel / Command split).
- Review the RFC 2812 numeric replies subset relevant for the mandatory commands.
- Cross-check the `poll()` loop against typical pitfalls (POLLHUP handling,
  iterator invalidation when a client disconnects mid-iteration, `SIGPIPE`).

All final code was written and validated by the author. AI was **not** used for
tasks that violate the subject (no forbidden functions, no external libraries,
no forking, no threads).
