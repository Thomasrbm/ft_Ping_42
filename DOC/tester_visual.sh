#!/usr/bin/env bash
# Tester de conformite visuelle : verifie que ft_ping affiche les bonnes lignes
# avec le bon format. Ignore les compteurs exacts (rate-limit Google, network jitter, etc.)
# Lancer : sudo ./tester_visual.sh

set -u

INET="/nix/store/kvaksdin2x6fb97zya571m2vn9fh5243-inetutils-2.2/bin/ping"
FTPG="./ft_ping"

GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[0;33m'; CYAN='\033[0;36m'; NC='\033[0m'
PASS=0; FAIL=0

if [[ ! -x "$FTPG" ]]; then
    echo -e "${RED}ft_ping not built. Run 'make' first.${NC}"
    exit 1
fi

# ===========================================================================
# Helpers
# ===========================================================================

# run_diff_simple : compare diff exact ft_ping vs inet (pour parsing/usage,
# deterministe, pas dependant du reseau)
run_diff_simple() {
    local name="$1" args="$2"
    local out_in out_ft
    out_in=$(timeout 5 $INET $args 2>&1 | sed -E "s|/nix/store/[^/]+/bin/ping|ping|g; s|\\bft_ping\\b|ping|g; s|Try 'ping --help' or 'ping --usage' for more information\\.|Try 'ping -?' for more information.|g")
    out_ft=$(timeout 5 $FTPG $args 2>&1 | sed -E "s|\\bft_ping\\b|ping|g")
    if [[ "$out_in" == "$out_ft" ]]; then
        echo -e "${GREEN}[PASS]${NC} $name  (args: $args)"
        ((PASS++))
    else
        echo -e "${RED}[FAIL]${NC} $name  (args: $args)"
        echo -e "${CYAN}  --- inet ---${NC}"; echo "$out_in" | sed 's/^/    /'
        echo -e "${CYAN}  --- ftpg ---${NC}"; echo "$out_ft" | sed 's/^/    /'
        ((FAIL++))
    fi
}

# run_pattern : verifie que la sortie ft_ping contient toutes les lignes regex demandees
# Args: name, args, sudo_flag, then list of expected regex patterns
run_pattern() {
    local name="$1"; shift
    local args="$1"; shift
    local sudo_flag="$1"; shift

    local out
    if [[ "$sudo_flag" == "1" ]]; then
        out=$(timeout 10 sudo -n $FTPG $args 2>&1)
    else
        out=$(timeout 5 $FTPG $args 2>&1)
    fi

    local missing=""
    for pat in "$@"; do
        if ! echo "$out" | grep -qE "$pat"; then
            missing+="\n    -> $pat"
        fi
    done

    if [[ -z "$missing" ]]; then
        echo -e "${GREEN}[PASS]${NC} $name  (args: $args)"
        ((PASS++))
    else
        echo -e "${RED}[FAIL]${NC} $name  (args: $args)"
        echo -e "${CYAN}  patterns manquants :${NC}$missing"
        echo -e "${CYAN}  --- ft_ping output ---${NC}"
        echo "$out" | sed 's/^/    /'
        ((FAIL++))
    fi
}

# run_no_pattern : verifie que la sortie NE contient PAS un certain pattern
# (ex: -q ne doit afficher AUCUNE ligne "bytes from")
run_no_pattern() {
    local name="$1" args="$2" sudo_flag="$3" forbidden="$4"
    local out
    if [[ "$sudo_flag" == "1" ]]; then
        out=$(timeout 10 sudo -n $FTPG $args 2>&1)
    else
        out=$(timeout 5 $FTPG $args 2>&1)
    fi
    if echo "$out" | grep -qE "$forbidden"; then
        echo -e "${RED}[FAIL]${NC} $name  (args: $args) - pattern interdit present : $forbidden"
        echo "$out" | sed 's/^/    /'
        ((FAIL++))
    else
        echo -e "${GREEN}[PASS]${NC} $name  (args: $args)"
        ((PASS++))
    fi
}

# ===========================================================================
# Patterns reutilisables
# ===========================================================================
P_HEADER='^PING [^ ]+ \([0-9.]+\): [0-9]+ data bytes$'
P_HEADER_VERBOSE='^PING [^ ]+ \([0-9.]+\): [0-9]+ data bytes, id 0x[0-9a-f]+ = [0-9]+$'
P_REPLY='^[0-9]+ bytes from [0-9.]+: icmp_seq=[0-9]+ ttl=[0-9]+( time=[0-9.,]+ ms)?$'
P_STATS_HDR='^--- [^ ]+ ping statistics ---$'
P_STATS_LINE='^[0-9]+ packets transmitted, [0-9]+ packets received, [0-9]+% packet loss$'
P_RTT='^round-trip min/avg/max/stddev = [0-9.,/]+ ms$'

# ===========================================================================
# Tests SANS reseau (parsing/usage) -- diff exact
# ===========================================================================
echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}  Tests SANS reseau (diff exact)${NC}"
echo -e "${CYAN}=========================================${NC}"

run_diff_simple "missing host"             ""
run_diff_simple "invalid option -Z"        "-Z"
run_diff_simple "invalid option -X"        "-X"
run_diff_simple "option -c sans valeur"    "-c"
run_diff_simple "option -w sans valeur"    "-w"
run_diff_simple "option -W sans valeur"    "-W"
run_diff_simple "option -s sans valeur"    "-s"
run_diff_simple "valeur non num -c abc"    "-c abc 8.8.8.8"
run_diff_simple "valeur non num -w foo"    "-w foo 8.8.8.8"
run_diff_simple "valeur trop grande -W"    "-W 999999999999 8.8.8.8"
run_diff_simple "--ttl=0 (too small)"      "--ttl=0 8.8.8.8"
run_diff_simple "--ttl=256 (too big)"      "--ttl=256 8.8.8.8"
run_diff_simple "--ttl=-1 (negatif)"       "--ttl=-1 8.8.8.8"
run_diff_simple "--ttl=abc (non num)"      "--ttl=abc 8.8.8.8"
run_diff_simple "--ttl seul (missing arg)" "--ttl"

echo ""
echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}  Tests AVEC reseau (conformite visuelle)${NC}"
echo -e "${CYAN}=========================================${NC}"
echo -e "${YELLOW}Verifie le format des lignes, pas les compteurs exacts${NC}"

if ! sudo -v; then
    echo -e "${RED}sudo requis pour les tests reseau${NC}"
    exit $((FAIL>0))
fi
( while true; do sudo -nv; sleep 30; done ) &
SUDO_KEEP=$!
trap "kill $SUDO_KEEP 2>/dev/null" EXIT

# Ping basique : doit afficher header + au moins une reply + stats
run_pattern "ping basique 8.8.8.8" "-c 3 8.8.8.8" 1 \
    "$P_HEADER" \
    "$P_REPLY" \
    "$P_STATS_HDR" \
    "$P_STATS_LINE"

sleep 2
# -q : header + stats, mais AUCUNE ligne reply
run_pattern "ping quiet -q (header + stats)" "-q -c 3 8.8.8.8" 1 \
    "$P_HEADER" \
    "$P_STATS_HDR" \
    "$P_STATS_LINE"
run_no_pattern "ping quiet -q (pas de reply lines)" "-q -c 3 8.8.8.8" 1 \
    "bytes from"

sleep 2
# -c 1 : header + au plus 1 reply + stats
run_pattern "ping count 1" "-c 1 8.8.8.8" 1 \
    "$P_HEADER" \
    "$P_STATS_HDR" \
    "$P_STATS_LINE"

sleep 2
# -s 100 : header avec "100 data bytes" + replies "108 bytes from"
run_pattern "ping size -s 100" "-s 100 -c 2 8.8.8.8" 1 \
    "^PING 8\.8\.8\.8 \(8\.8\.8\.8\): 100 data bytes$" \
    "^108 bytes from " \
    "$P_STATS_HDR"

sleep 2
# -s 0 : header avec "0 data bytes" + replies "8 bytes from" SANS time
run_pattern "ping size -s 0" "-s 0 -c 2 8.8.8.8" 1 \
    "^PING 8\.8\.8\.8 \(8\.8\.8\.8\): 0 data bytes$" \
    "^8 bytes from [0-9.]+: icmp_seq=[0-9]+ ttl=[0-9]+$" \
    "$P_STATS_HDR"

sleep 2
# -w deadline : format normal
run_pattern "ping deadline -w 2" "-w 2 8.8.8.8" 1 \
    "$P_HEADER" \
    "$P_STATS_HDR" \
    "$P_STATS_LINE"

sleep 2
# unreachable 240.0.0.1 : header + (reply ou erreur) + stats
run_pattern "ping unreachable 240.0.0.1" "-c 2 240.0.0.1" 1 \
    "^PING 240\.0\.0\.1 \(240\.0\.0\.1\): 56 data bytes$" \
    "$P_STATS_HDR"

sleep 2
# verbose unreachable : header verbose (avec id) + dump erreur + stats
run_pattern "ping verbose unreachable" "-v -c 2 240.0.0.1" 1 \
    "$P_HEADER_VERBOSE" \
    "$P_STATS_HDR"

sleep 2
# --ttl=64
run_pattern "--ttl=64 normal" "--ttl=64 -c 2 8.8.8.8" 1 \
    "$P_HEADER" \
    "$P_STATS_HDR" \
    "$P_STATS_LINE"

sleep 2
# --ttl 64 (espace)
run_pattern "--ttl 64 (espace)" "--ttl 64 -c 2 8.8.8.8" 1 \
    "$P_HEADER" \
    "$P_STATS_HDR"

sleep 2
# --ttl=255
run_pattern "--ttl=255 max" "--ttl=255 -c 2 8.8.8.8" 1 \
    "$P_HEADER" \
    "$P_STATS_HDR"

sleep 2
# --ttl=128
run_pattern "--ttl=128 mid" "--ttl=128 -c 2 8.8.8.8" 1 \
    "$P_HEADER" \
    "$P_STATS_HDR"

sleep 2
# -r --ttl=64 : doit afficher soit reply, soit "Network is unreachable"
run_pattern "-r --ttl=64" "-r --ttl=64 -c 1 8.8.8.8" 1 \
    "^PING 8\.8\.8\.8" \
    "(bytes from|Network is unreachable|sending packet)"

sleep 2
# -r unreachable
run_pattern "-r unreachable" "-r -c 1 8.8.8.8" 1 \
    "^PING 8\.8\.8\.8" \
    "(bytes from|Network is unreachable|sending packet)"

sleep 2
# -c 0 borne par -w : doit envoyer au moins 1 paquet et afficher stats
run_pattern "-c 0 + -w 1 (inf borne)" "-c 0 -w 1 8.8.8.8" 1 \
    "$P_HEADER" \
    "$P_STATS_HDR" \
    "$P_STATS_LINE"

sleep 2
run_pattern "-c -5 + -w 1 (inf borne)" "-c -5 -w 1 8.8.8.8" 1 \
    "$P_HEADER" \
    "$P_STATS_HDR" \
    "$P_STATS_LINE"

echo ""
echo -e "${CYAN}=========================================${NC}"
echo -e "  ${GREEN}PASS:  $PASS${NC}"
echo -e "  ${RED}FAIL:  $FAIL${NC}"
echo -e "${CYAN}=========================================${NC}"

exit $((FAIL > 0))
