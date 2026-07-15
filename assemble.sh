#!/usr/bin/env bash
# assemble.sh — monta a árvore do projeto ft_irc a partir dos arquivos .txt
# baixados do chat. Execute na mesma pasta onde estão os .txt.
#
# Uso:
#   chmod +x assemble.sh
#   ./assemble.sh
#
# O script:
#   1. Cria include/ e src/
#   2. Renomeia Makefile_FINAL.txt -> Makefile
#   3. Move *.hpp.txt  para include/*.hpp
#   4. Move *.cpp.txt  para src/*.cpp
#   5. Deixa README.md como está
#   6. Reporta arquivos faltantes (se houver)

set -e

echo "==> Criando estrutura de pastas..."
mkdir -p include src

# Lista esperada de headers (sem o sufixo .txt)
HEADERS=(Server.hpp Client.hpp Channel.hpp Command.hpp Replies.hpp Utils.hpp)

# Lista esperada de fontes (sem o sufixo .txt)
SOURCES=(main.cpp Server.cpp Client.cpp Channel.cpp Command.cpp \
         CommandAuth.cpp CommandChannel.cpp CommandOps.cpp \
         CommandMode.cpp CommandMisc.cpp Utils.cpp)

MISSING=()

# 1) Makefile
if [ -f "Makefile_FINAL.txt" ]; then
    mv -f Makefile_FINAL.txt Makefile
    echo "  [ok] Makefile"
elif [ -f "Makefile.txt" ]; then
    mv -f Makefile.txt Makefile
    echo "  [ok] Makefile (a partir de Makefile.txt)"
elif [ -f "Makefile" ]; then
    echo "  [ok] Makefile (já presente)"
else
    MISSING+=("Makefile_FINAL.txt (ou Makefile.txt)")
fi

# 2) Headers -> include/
echo "==> Movendo headers para include/..."
for h in "${HEADERS[@]}"; do
    if [ -f "${h}.txt" ]; then
        mv -f "${h}.txt" "include/${h}"
        echo "  [ok] include/${h}"
    elif [ -f "include/${h}" ]; then
        echo "  [ok] include/${h} (já presente)"
    else
        MISSING+=("${h}.txt")
    fi
done

# 3) Sources -> src/
echo "==> Movendo fontes para src/..."
for s in "${SOURCES[@]}"; do
    if [ -f "${s}.txt" ]; then
        mv -f "${s}.txt" "src/${s}"
        echo "  [ok] src/${s}"
    elif [ -f "src/${s}" ]; then
        echo "  [ok] src/${s} (já presente)"
    else
        MISSING+=("${s}.txt")
    fi
done

# 4) README.md — só confere presença
if [ -f "README.md" ]; then
    echo "  [ok] README.md"
else
    MISSING+=("README.md")
fi

# 5) Relatório final
echo
if [ ${#MISSING[@]} -eq 0 ]; then
    echo "======================================================"
    echo "  Tudo pronto! Estrutura montada com sucesso."
    echo "======================================================"
    echo
    echo "Árvore final:"
    find . -maxdepth 2 -type f \( -name 'Makefile' -o -name '*.md' \
        -o -name '*.hpp' -o -name '*.cpp' \) | sort
    echo
    echo "Agora compile com:"
    echo "  make"
    echo
    echo "E rode com:"
    echo "  ./ircserv 6667 hunter2"
else
    echo "======================================================"
    echo "  Faltam os seguintes arquivos na pasta atual:"
    echo "======================================================"
    for f in "${MISSING[@]}"; do
        echo "  - ${f}"
    done
    echo
    echo "Baixe-os do chat e rode o script de novo."
    exit 1
fi
