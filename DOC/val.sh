#!/usr/bin/env bash

# ============================================================
#  ft_ping - script de test
#  Usage: ./test.sh           -> tous les tests
#         ./test.sh valgrind  -> tests avec valgrind seulement
#         ./test.sh diff      -> compare avec le vrai ping
#         ./test.sh flags     -> tests des flags (parsing)
#         ./test.sh errors    -> tests des cas d'erreur
# ============================================================

# on se place a la racine du projet (la ou est le script)
cd "$(dirname "$0")" || exit 1

BIN=./ft_ping
REAL=$(command -v ping || echo /run/wrappers/bin/ping)

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
BLUE="\033[0;34m"
RESET="\033[0m"

PASS=0
FAIL=0

# ------------------------------------------------------------
#  Utils
# ------------------------------------------------------------
title() {
    echo ""
    echo -e "${BLUE}============================================================${RESET}"
    echo -e "${BLUE} $1${RESET}"
    echo -e "${BLUE}============================================================${RESET}"
}

#  run_test "desc" "cmd"           -> on attend exit 0
#  run_test_fail "desc" "cmd"      -> on attend exit != 0 (test d'erreur)
run_test() {
    local desc="$1"
    local cmd="$2"
    echo ""
    echo -e "${YELLOW}[TEST]${RESET} $desc"
    echo -e "  ${YELLOW}>${RESET} $cmd"
    eval "$cmd"
    local rc=$?
    if [ $rc -eq 0 ]; then
        echo -e "  ${GREEN}[OK]${RESET} (exit $rc)"
        PASS=$((PASS+1))
    else
        echo -e "  ${RED}[KO]${RESET} (exit $rc, attendu 0)"
        FAIL=$((FAIL+1))
    fi
}

run_test_fail() {
    local desc="$1"
    local cmd="$2"
    echo ""
    echo -e "${YELLOW}[TEST]${RESET} $desc ${BLUE}(exit != 0 attendu)${RESET}"
    echo -e "  ${YELLOW}>${RESET} $cmd"
    eval "$cmd"
    local rc=$?
    if [ $rc -ne 0 ]; then
        echo -e "  ${GREEN}[OK]${RESET} (exit $rc, rejet correct)"
        PASS=$((PASS+1))
    else
        echo -e "  ${RED}[KO]${RESET} (exit 0 mais on attendait une erreur)"
        FAIL=$((FAIL+1))
    fi
}

build() {
    title "BUILD"

    # Si le script tourne en root (sudo ./test.sh), le build appartiendrait
    # a root et casserait un make re ulterieur en user.
    # -> on ne build JAMAIS en root.
    if [ "$EUID" -eq 0 ] && [ -n "$SUDO_USER" ]; then
        echo -e "${YELLOW}-> build en user ($SUDO_USER), pas en root${RESET}"
        sudo -u "$SUDO_USER" make re || { echo -e "${RED}Build failed${RESET}"; exit 1; }
    else
        make re || { echo -e "${RED}Build failed${RESET}"; exit 1; }
    fi

    if [ ! -f "$BIN" ]; then
        echo -e "${RED}Binary $BIN introuvable${RESET}"
        exit 1
    fi

    if ! getcap "$BIN" 2>/dev/null | grep -q cap_net_raw; then
        echo -e "${YELLOW}Astuce :${RESET} sudo setcap cap_net_raw+ep $BIN  (pour run sans sudo)"
    fi
}

# ------------------------------------------------------------
#  Tests parsing / flags
# ------------------------------------------------------------
test_flags() {
    title "PARSING / FLAGS"

    run_test_fail "no arg"                "$BIN"
    run_test      "-?"                    "$BIN '-?'"
    run_test      "-V"                    "$BIN -V"
    run_test_fail "--help (non gere)"     "set -o pipefail; $BIN --help 2>&1 | head -20"
    run_test_fail "flag inconnu"          "$BIN -Z google.com"
    run_test_fail "-c sans valeur"        "$BIN -c"
    run_test_fail "-c valeur invalide"    "$BIN -c abc google.com"
    # -c 0 et -c <negatif> : valeurs valides (quirk inet -> infini). Borne par -w 1 sinon ne s arrete jamais.
    run_test      "-c 0 (infini, -w 1)"   "$BIN -c 0 -w 1 127.0.0.1"
    run_test      "-c -5 (infini, -w 1)"  "$BIN -c -5 -w 1 127.0.0.1"
    run_test_fail "-s overflow"           "$BIN -s 99999 google.com"
    run_test_fail "-s 65508 (just over)"  "$BIN -s 65508 google.com"
    run_test_fail "-w overflow"           "$BIN -w 99999999999 google.com"
    run_test_fail "-w 0 (min 1)"          "$BIN -w 0 127.0.0.1"
    run_test_fail "-W overflow"           "$BIN -W 99999999999 google.com"
    run_test_fail "-W 0 (min 1)"          "$BIN -W 0 127.0.0.1"
    run_test_fail "host invalide"         "$BIN nimportequoi.invalide.tld"
    run_test_fail "host vide"             "$BIN ''"
    # --ttl (nouveau flag, range 1..255)
    run_test_fail "--ttl=0 (too small)"   "$BIN --ttl=0 127.0.0.1"
    run_test_fail "--ttl=256 (too big)"   "$BIN --ttl=256 127.0.0.1"
    run_test_fail "--ttl=-1 (negatif)"    "$BIN --ttl=-1 127.0.0.1"
    run_test_fail "--ttl=abc (non num)"   "$BIN --ttl=abc 127.0.0.1"
    run_test_fail "--ttl seul (no val)"   "$BIN --ttl"
    # flags supprimes (anciens -i / -n) -> doivent etre unknown
    run_test_fail "-i (supprime)"         "$BIN -i 1 127.0.0.1"
    run_test_fail "-n (supprime)"         "$BIN -n 127.0.0.1"
}

# ------------------------------------------------------------
#  Tests fonctionnels (golden path)
# ------------------------------------------------------------
test_basic() {
    title "BASIC (run)"

    run_test "ping localhost -c 3"        "$BIN -c 3 127.0.0.1"
    run_test "ping google -c 2"           "$BIN -c 2 google.com"
    run_test "ping -q -c 3"               "$BIN -q -c 3 127.0.0.1"
    run_test "ping -v -c 2"               "$BIN -v -c 2 127.0.0.1"
    run_test "ping -s 0 (autorise)"       "$BIN -s 0 -c 2 127.0.0.1"
    run_test "ping -s 100 -c 2"           "$BIN -s 100 -c 2 127.0.0.1"
    run_test "ping -s 65399 -c 1 (MAX)"   "$BIN -s 65399 -c 1 127.0.0.1"
    run_test "ping -w 2"                  "$BIN -w 2 127.0.0.1"
    run_test "ping -W 1 -c 2 ip morte"    "$BIN -W 1 -c 2 10.255.255.1"
    # nouveaux flags : -r (SO_DONTROUTE) et --ttl
    run_test "ping -r -c 2 local"         "$BIN -r -c 2 127.0.0.1"
    run_test "ping --ttl=64 -c 2"         "$BIN --ttl=64 -c 2 127.0.0.1"
    run_test "ping --ttl 64 -c 2 (sep)"   "$BIN --ttl 64 -c 2 127.0.0.1"
    run_test "ping --ttl=255 -c 1"        "$BIN --ttl=255 -c 1 127.0.0.1"
    run_test "ping --ttl=1 -c 1 local"    "$BIN --ttl=1 -c 1 127.0.0.1"
    # quirk -c 0 / -c <neg> : infini, borne par -w
    run_test "ping -c 0 -w 2 (inf borne)" "$BIN -c 0 -w 2 127.0.0.1"
    run_test "ping -c -3 -w 2 (inf borne)" "$BIN -c -3 -w 2 127.0.0.1"
    # combos
    run_test "combo -v -c 2 -s 128 ttl"   "$BIN -v -c 2 -s 128 --ttl=64 127.0.0.1"
    run_test "combo -q -r -c 2"           "$BIN -q -r -c 2 127.0.0.1"
}

# ------------------------------------------------------------
#  Comparaison avec le vrai ping
# ------------------------------------------------------------
test_diff() {
    title "DIFF vs /bin/ping"

    if [ ! -x "$REAL" ]; then
        echo -e "${RED}ping introuvable a $REAL${RESET}"
        return
    fi
    echo -e "${YELLOW}-> ping reel : $REAL${RESET}"

    local targets=("127.0.0.1" "8.8.8.8" "google.com")
    for t in "${targets[@]}"; do
        echo ""
        echo -e "${YELLOW}[DIFF]${RESET} $t"
        echo -e "${BLUE}--- ft_ping ---${RESET}"
        $BIN -c 2 "$t" 2>&1 | tee /tmp/ft_ping.out
        echo -e "${BLUE}--- ping ---${RESET}"
        $REAL -c 2 "$t" 2>&1 | tee /tmp/real_ping.out
        echo -e "${YELLOW}--- diff ---${RESET}"
        diff <(sed -E 's/[0-9]+(\.[0-9]+)?//g' /tmp/ft_ping.out) \
             <(sed -E 's/[0-9]+(\.[0-9]+)?//g' /tmp/real_ping.out) || true
    done
}

# ------------------------------------------------------------
#  Valgrind
# ------------------------------------------------------------
test_valgrind() {
    title "VALGRIND (memcheck)"

    if ! command -v valgrind >/dev/null; then
        echo -e "${RED}valgrind pas installe${RESET}"
        return
    fi

    local VG="valgrind --leak-check=full --show-leak-kinds=all \
        --track-origins=yes --error-exitcode=42 -q"

    # Si pas de cap_net_raw, faut sudo. valgrind + sudo : sudo -E valgrind
    local PREFIX=""
    if ! getcap "$BIN" 2>/dev/null | grep -q cap_net_raw; then
        echo -e "${YELLOW}-> on lance avec sudo (raw socket requis)${RESET}"
        PREFIX="sudo -E"
    fi

    # NOTE: pour les tests d'erreur, on enrobe avec "rc=$?; [ $rc -ne 42 ]"
    # -> on tolere n'importe quel exit sauf 42 (qui est le code reserve aux leaks valgrind)

    # --- meta / parsing pur (pas de socket, pas de reseau) ---
    run_test "valgrind no arg"            "$PREFIX $VG $BIN; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind -?"                "$PREFIX $VG $BIN '-?'; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind -V"                "$PREFIX $VG $BIN -V; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind flag inconnu"      "$PREFIX $VG $BIN -Z 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind host invalide"     "$PREFIX $VG $BIN nope.invalide.tld; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind host vide"         "$PREFIX $VG $BIN ''; rc=\$?; [ \$rc -ne 42 ]"

    # --- -c (count) ---
    run_test "valgrind -c 2 localhost"    "$PREFIX $VG $BIN -c 2 127.0.0.1"
    run_test "valgrind -c 3 google"       "$PREFIX $VG $BIN -c 3 google.com"
    run_test "valgrind -c sans val"       "$PREFIX $VG $BIN -c; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind -c abc"            "$PREFIX $VG $BIN -c abc 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"
    # -c 0 / -c -5 : infini comme inet -> borne par -w 1 sinon valgrind ne finit jamais
    run_test "valgrind -c 0 -w 1"         "$PREFIX $VG $BIN -c 0 -w 1 127.0.0.1"
    run_test "valgrind -c -5 -w 1"        "$PREFIX $VG $BIN -c -5 -w 1 127.0.0.1"

    # --- -s (packet size) ---
    run_test "valgrind -s 0 (autorise)"   "$PREFIX $VG $BIN -s 0 -c 2 127.0.0.1"
    run_test "valgrind -s 56 -c 2"        "$PREFIX $VG $BIN -s 56 -c 2 127.0.0.1"
    run_test "valgrind -s 1472 -c 2"      "$PREFIX $VG $BIN -s 1472 -c 2 127.0.0.1"
    run_test "valgrind -s 65399 -c 1 MAX" "$PREFIX $VG $BIN -s 65399 -c 1 127.0.0.1"
    run_test "valgrind -s overflow"       "$PREFIX $VG $BIN -s 99999 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"

    # NOTE: tout test impliquant une attente reelle (-w, -W sur ip morte,
    # -i > 0, -i/-w/-W overflow) est skip -> sous valgrind, meme une attente
    # de 1-2 sec devient des dizaines de sec et rend la suite interminable.
    # On garde uniquement : parsing errors (rejet immediat) + runs comptes (-c)
    # qui finissent sous la seconde sur localhost.

    # --- flags supprimes (-i / -n) : doivent etre rejetes comme inconnus, sans leak ---
    run_test "valgrind -i (supprime)"     "$PREFIX $VG $BIN -i 1 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind -n (supprime)"     "$PREFIX $VG $BIN -n 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"

    # --- -s edge cases ---
    run_test "valgrind -s 65508 (just>)"  "$PREFIX $VG $BIN -s 65508 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"

    # --- -w / -W min (1) ---
    run_test "valgrind -w 0 (rejet)"      "$PREFIX $VG $BIN -w 0 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind -W 0 (rejet)"      "$PREFIX $VG $BIN -W 0 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"

    # --- -w (deadline) : pas de run, juste parsing ---
    # (overflow skip aussi -> ft_strtol/cmp peut prendre du temps selon la val)

    # --- -W (timeout par reply) : tout skip, c'est par definition une attente ---

    # --- -r (SO_DONTROUTE) : run rapide localhost uniquement ---
    run_test "valgrind -r local -c 2"     "$PREFIX $VG $BIN -r -c 2 127.0.0.1"

    # --- --ttl : runs cours + tous les rejets immediats ---
    run_test "valgrind --ttl=64 -c 2"     "$PREFIX $VG $BIN --ttl=64 -c 2 127.0.0.1"
    run_test "valgrind --ttl=255 -c 2"    "$PREFIX $VG $BIN --ttl=255 -c 2 127.0.0.1"
    run_test "valgrind --ttl=0 (small)"   "$PREFIX $VG $BIN --ttl=0 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind --ttl=300 (big)"   "$PREFIX $VG $BIN --ttl=300 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind --ttl=-1 (big)"    "$PREFIX $VG $BIN --ttl=-1 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind --ttl=abc"         "$PREFIX $VG $BIN --ttl=abc 127.0.0.1; rc=\$?; [ \$rc -ne 42 ]"
    run_test "valgrind --ttl sans val"    "$PREFIX $VG $BIN --ttl; rc=\$?; [ \$rc -ne 42 ]"

    # --- -v / -n / -q ---
    run_test "valgrind -v -q -c 2"        "$PREFIX $VG $BIN -v -q -c 2 127.0.0.1"
    run_test "valgrind -v -c 2 google"    "$PREFIX $VG $BIN -v -c 2 google.com"
    run_test "valgrind -q -c 3 local"     "$PREFIX $VG $BIN -q -c 3 127.0.0.1"

    # --- combos de flags (sans -i / -w / -W pour eviter les attentes) ---
    run_test "valgrind combo full"        "$PREFIX $VG $BIN -v -c 2 -s 128 --ttl=64 127.0.0.1"
    run_test "valgrind combo r+ttl"       "$PREFIX $VG $BIN -r --ttl=32 -c 2 127.0.0.1"
    run_test "valgrind combo q+s"         "$PREFIX $VG $BIN -q -s 200 -c 2 127.0.0.1"
}

# ------------------------------------------------------------
#  Resume
# ------------------------------------------------------------
summary() {
    title "SUMMARY"
    echo -e "  ${GREEN}PASS:${RESET} $PASS"
    echo -e "  ${RED}FAIL:${RESET} $FAIL"
    echo ""
    [ $FAIL -eq 0 ] && exit 0 || exit 1
}

# ------------------------------------------------------------
#  Main
# ------------------------------------------------------------
case "${1:-all}" in
    flags)    build; test_flags;    summary ;;
    basic)    build; test_basic;    summary ;;
    diff)     build; test_diff;     summary ;;
    valgrind) build; test_valgrind; summary ;;
    errors)   build; test_flags;    summary ;;
    all)      build; test_flags; test_basic; test_diff; test_valgrind; summary ;;
    *)        echo "Usage: $0 [all|flags|basic|diff|valgrind|errors]"; exit 1 ;;
esac
