#!/usr/bin/env bash
# test.sh — bateria de testes automatizados para ircserv
# Rode primeiro: ./ircserv 6667 hunter2 (em outro terminal)
# Depois: bash test.sh
#
# Requer: bash (usa /dev/tcp), nao precisa de nc.

HOST=127.0.0.1
PORT=6667
PASS=hunter2

PASSED=0
FAILED=0
declare -a FAILURES

# --- helpers -------------------------------------------------------------

# Envia comandos e captura respostas por um fd de socket bash
# Uso: send_and_capture <output_file> <commands_multiline>
send_and_capture() {
    local out="$1"; shift
    local cmds="$1"
    local fd
    exec {fd}<>/dev/tcp/$HOST/$PORT
    # captura em background
    cat <&$fd > "$out" &
    local catpid=$!
    # envia comandos
    printf '%b' "$cmds" >&$fd
    sleep 1.2
    kill $catpid 2>/dev/null
    wait $catpid 2>/dev/null
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
        echo "    recebido:"
        sed 's/^/      /' "$file" | head -20
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
        echo "  ✗ FAIL: $desc (encontrou padrao proibido)"
        FAILED=$((FAILED+1))
        FAILURES+=("$desc")
    fi
}

TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

echo "===================================================="
echo "  ircserv test suite  |  target $HOST:$PORT"
echo "===================================================="
echo

# --- 1. autenticacao basica ---------------------------------------------
echo "[1] Auth completa (PASS/NICK/USER + welcome)"
send_and_capture "$TMPDIR/t1.out" \
"PASS $PASS\r\nNICK bob\r\nUSER bob 0 * :Bob Example\r\n"
check "recebe 001 (RPL_WELCOME)" "$TMPDIR/t1.out" "001 bob"
check "recebe 004 (RPL_MYINFO)"   "$TMPDIR/t1.out" "004 bob"

# --- 2. JOIN cria canal e vira op ---------------------------------------
echo
echo "[2] JOIN cria canal e promove a op"
send_and_capture "$TMPDIR/t2.out" \
"PASS $PASS\r\nNICK bob\r\nUSER bob 0 * :B\r\nJOIN #test\r\n"
check "JOIN broadcast"          "$TMPDIR/t2.out" "JOIN :#test"
check "NAMES mostra @bob (op)"  "$TMPDIR/t2.out" "353.*@bob"
check "End of NAMES (366)"      "$TMPDIR/t2.out" "366 bob #test"

echo
echo "===================================================="
echo "  Resultado: $PASSED passou, $FAILED falhou"
if [ $FAILED -gt 0 ]; then
    echo "  Falhas:"
    for f in "${FAILURES[@]}"; do echo "    - $f"; done
fi
echo "===================================================="
[ $FAILED -eq 0 ]
