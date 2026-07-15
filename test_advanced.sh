#!/usr/bin/env bash
# test_advanced.sh — cenarios complexos multi-cliente para ircserv
# Rode: ./ircserv 6667 hunter2 em outro terminal, depois bash test_advanced.sh

HOST=127.0.0.1
PORT=6667
PASS=hunter2

PASSED=0
FAILED=0
declare -a FAILURES

TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

# open_client <name> -> exporta FD_<name> e OUT_<name>
open_client() {
    local name="$1"
    local out="$TMPDIR/$name.out"
    local fifo="$TMPDIR/$name.fifo"
    mkfifo "$fifo"
    exec {fd}<>/dev/tcp/$HOST/$PORT
    eval "FD_$name=$fd"
    eval "OUT_$name=$out"
    cat <&$fd > "$out" &
    eval "PID_$name=$!"
}

send() {
    local name="$1"; shift
    local fdvar="FD_$name"
    local fd=${!fdvar}
    printf '%b' "$*" >&$fd
}

close_client() {
    local name="$1"
    local fdvar="FD_$name"
    local pidvar="PID_$name"
    local fd=${!fdvar}
    kill ${!pidvar} 2>/dev/null
    exec {fd}<&-
}

check() {
    local desc="$1" file="$2" pattern="$3"
    if grep -q -- "$pattern" "$file"; then
        echo "  ✓ PASS: $desc"
        PASSED=$((PASSED+1))
    else
        echo "  ✗ FAIL: $desc"
        echo "    esperava: $pattern"
        echo "    recebido em $file:"
        sed 's/^/      /' "$file" | head -30
        FAILED=$((FAILED+1))
        FAILURES+=("$desc")
    fi
}

check_not() {
    local desc="$1" file="$2" pattern="$3"
    if ! grep -q -- "$pattern" "$file"; then
        echo "  ✓ PASS: $desc"
        PASSED=$((PASSED+1))
    else
        echo "  ✗ FAIL: $desc (padrao proibido encontrado)"
        FAILED=$((FAILED+1))
        FAILURES+=("$desc")
    fi
}

register() {
    local name="$1" nick="$2"
    send $name "PASS $PASS\r\nNICK $nick\r\nUSER $nick 0 * :$nick\r\n"
    sleep 0.4
}

echo "===================================================="
echo "  ircserv advanced test suite"
echo "===================================================="

# ------------------------------------------------------------
# Cenario 1: PRIVMSG canal (bob -> alice via #room)
# ------------------------------------------------------------
echo
echo "[1] PRIVMSG canal (bob -> alice via #room)"
open_client bob; register bob bob
send bob "JOIN #room\r\n"; sleep 0.3
open_client alice; register alice alice
send alice "JOIN #room\r\n"; sleep 0.3
send bob "PRIVMSG #room :ola alice\r\n"; sleep 0.5
check "alice recebe PRIVMSG do bob no #room" "${OUT_alice}" "bob.*PRIVMSG #room :ola alice"

# ------------------------------------------------------------
# Cenario 2: PRIVMSG direto (DM alice -> bob)
# ------------------------------------------------------------
echo
echo "[2] PRIVMSG direto (DM alice -> bob)"
send alice "PRIVMSG bob :dm secreta\r\n"; sleep 0.5
check "bob recebe DM da alice" "${OUT_bob}" "alice.*PRIVMSG bob :dm secreta"

# ------------------------------------------------------------
# Cenario 3: TOPIC (bob eh op, define topic; alice recebe)
# ------------------------------------------------------------
echo
echo "[3] TOPIC set pelo op bob"
send bob "TOPIC #room :welcome to room\r\n"; sleep 0.5
check "alice recebe TOPIC" "${OUT_alice}" "bob.*TOPIC #room :welcome to room"

# ------------------------------------------------------------
# Cenario 4: MODE +i (invite-only bloqueia charlie)
# ------------------------------------------------------------
echo
echo "[4] MODE +i bloqueia charlie sem convite"
send bob "MODE #room +i\r\n"; sleep 0.4
open_client charlie; register charlie charlie
send charlie "JOIN #room\r\n"; sleep 0.5
check "charlie recebe 473 (ERR_INVITEONLYCHAN)" "${OUT_charlie}" "473 charlie #room"

# ------------------------------------------------------------
# Cenario 5: INVITE libera charlie
# ------------------------------------------------------------
echo
echo "[5] INVITE libera charlie a entrar"
send bob "INVITE charlie #room\r\n"; sleep 0.4
send charlie "JOIN #room\r\n"; sleep 0.5
check "charlie recebe JOIN broadcast" "${OUT_charlie}" "charlie.*JOIN :#room"
check "alice ve charlie entrando"      "${OUT_alice}"   "charlie.*JOIN :#room"

# ------------------------------------------------------------
# Cenario 6: MODE +o promove alice a op
# ------------------------------------------------------------
echo
echo "[6] MODE +o promove alice a op"
send bob "MODE #room +o alice\r\n"; sleep 0.5
check "alice ve MODE +o" "${OUT_alice}" "MODE #room +o alice"

# ------------------------------------------------------------
# Cenario 7: KICK feito pela alice (ja eh op)
# ------------------------------------------------------------
echo
echo "[7] KICK: alice kicka charlie"
send alice "KICK #room charlie :bye\r\n"; sleep 0.5
check "charlie recebe KICK dele proprio" "${OUT_charlie}" "KICK #room charlie :bye"
check "bob ve KICK broadcast"             "${OUT_bob}"     "alice.*KICK #room charlie"

# ------------------------------------------------------------
# Cenario 8: TESTE DO SUBJECT - fragmentacao de pacote
#   Envia 'PRIVM' + 'SG #room :hel' + 'lo\r\n' e valida
#   que bob (no #room) recebe UM PRIVMSG completo
# ------------------------------------------------------------
echo
echo "[8] Fragmentacao (subject: com+man+d\\n) - alice envia em pedacos"
: > "${OUT_bob}"  # limpa buffer do bob para a validacao ficar limpa
send alice "PRIVM"; sleep 0.3
send alice "SG #room :hel"; sleep 0.3
send alice "lo\r\n"; sleep 0.7
check "bob recebe PRIVMSG reagregado" "${OUT_bob}" "alice.*PRIVMSG #room :hello"

# ------------------------------------------------------------
close_client bob
close_client alice
close_client charlie

echo
echo "===================================================="
echo "  Resultado: $PASSED passou, $FAILED falhou"
if [ $FAILED -gt 0 ]; then
    echo "  Falhas:"
    for f in "${FAILURES[@]}"; do echo "    - $f"; done
fi
echo "===================================================="
[ $FAILED -eq 0 ]
