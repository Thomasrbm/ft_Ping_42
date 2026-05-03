#!/usr/bin/env bash
# Compare ft_ping vs inetutils 2.2 sur tous les flags supportes.
# Lancer depuis le repo : ./tester.sh

set -u

INET="/nix/store/kvaksdin2x6fb97zya571m2vn9fh5243-inetutils-2.2/bin/ping"
FTPG="./ft_ping"

GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[0;33m'; CYAN='\033[0;36m'; NC='\033[0m'
PASS=0; FAIL=0; XFAIL=0

if [[ ! -x "$FTPG" ]]; then
    echo -e "${RED}ft_ping not built. Run 'make' first.${NC}"
    exit 1
fi
if [[ ! -x "$INET" ]]; then
    echo -e "${RED}inetutils ping not found at $INET${NC}"
    exit 1
fi

# Normalise les valeurs qui varient pour pouvoir comparer textuellement
normalize() {
    sed -E \
        -e "s|/nix/store/[^/]+/bin/ping|ping|g" \
        -e "s|\\bft_ping\\b|ping|g" \
        -e "s|time=[0-9.,]+ ms|time=N ms|g" \
        -e "s|ttl=[0-9]+|ttl=N|g" \
        -e "s|min/avg/max(/stddev)? = [0-9.,/]+ ms|min/avg/max/stddev = N/N/N/N ms|g" \
        -e "s|icmp_seq=[0-9]+|icmp_seq=N|g" \
        -e "s|id 0x[0-9a-fA-F]+ = [0-9]+|id 0xN = N|g" \
        -e "s|Try 'ping --help' or 'ping --usage' for more information\\.|Try 'ping -?' for more information.|g"
}

# Teste deux commandes, normalise, diff. $1=nom, $2=args, $3=needs_sudo(0|1)
run_diff() {
    local name="$1" args="$2" sudo_flag="${3:-0}"
    local out_in out_ft
    if [[ "$sudo_flag" == "1" ]]; then
        out_in=$(timeout 20 sudo -n $INET $args 2>&1 | normalize)
        out_ft=$(timeout 20 sudo -n $FTPG $args 2>&1 | normalize)
    else
        out_in=$(timeout 5 $INET $args 2>&1 | normalize)
        out_ft=$(timeout 5 $FTPG $args 2>&1 | normalize)
    fi
    if [[ "$out_in" == "$out_ft" ]]; then
        echo -e "${GREEN}[PASS]${NC} $name  (args: $args)"
        ((PASS++))
    else
        echo -e "${RED}[FAIL]${NC} $name  (args: $args)"
        echo -e "${CYAN}  --- inet ---${NC}"
        echo "$out_in" | sed 's/^/    /'
        echo -e "${CYAN}  --- ftpg ---${NC}"
        echo "$out_ft" | sed 's/^/    /'
        ((FAIL++))
    fi
}

# Pareil, mais skip les N premieres lignes de ft_ping (pour -V qui a 2 lignes en plus)
run_diff_skip_ft() {
    local name="$1" args="$2" skip="$3"
    local out_in out_ft
    out_in=$($INET $args 2>&1 | normalize)
    out_ft=$($FTPG $args 2>&1 | tail -n +$((skip+1)) | normalize)
    if [[ "$out_in" == "$out_ft" ]]; then
        echo -e "${GREEN}[PASS]${NC} $name  (args: $args)  ${YELLOW}[skip $skip lines]${NC}"
        ((PASS++))
    else
        echo -e "${RED}[FAIL]${NC} $name  (args: $args)"
        echo -e "${CYAN}  --- inet ---${NC}"; echo "$out_in" | sed 's/^/    /'
        echo -e "${CYAN}  --- ftpg ---${NC}"; echo "$out_ft" | sed 's/^/    /'
        ((FAIL++))
    fi
}

# Test ou on s'attend a une divergence (comportement assume different)
expect_diff() {
    local name="$1" args="$2" sudo_flag="${3:-0}"
    local out_in out_ft
    if [[ "$sudo_flag" == "1" ]]; then
        out_in=$(timeout 8 sudo -n $INET $args 2>&1 | normalize)
        out_ft=$(timeout 8 sudo -n $FTPG $args 2>&1 | normalize)
    else
        out_in=$(timeout 5 $INET $args 2>&1 | normalize)
        out_ft=$(timeout 5 $FTPG $args 2>&1 | normalize)
    fi
    if [[ "$out_in" != "$out_ft" ]]; then
        echo -e "${YELLOW}[XFAIL]${NC} $name  (divergence assumee)"
        ((XFAIL++))
    else
        echo -e "${RED}[UNEXPECTED-PASS]${NC} $name (etait suppose diverger)"
        ((FAIL++))
    fi
}

echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}  Tests SANS reseau (parsing/usage)${NC}"
echo -e "${CYAN}=========================================${NC}"

run_diff           "missing host"              ""
run_diff           "invalid option -Z"         "-Z"
run_diff           "invalid option -X"         "-X"
run_diff           "option -c sans valeur"     "-c"
run_diff           "option -w sans valeur"     "-w"
run_diff           "option -W sans valeur"     "-W"
run_diff           "option -i sans valeur"     "-i"
run_diff           "option -s sans valeur"     "-s"
run_diff           "valeur non num -c abc"     "-c abc 8.8.8.8"
run_diff           "valeur non num -w foo"     "-w foo 8.8.8.8"
# quirk inet : pour -i non-num il add 'Try ...' (pas pour -c/-w). Reproduit cote ftpg.
run_diff           "valeur non num -i x"       "-i x 8.8.8.8"
run_diff           "valeur trop grande -W"     "-W 999999999999 8.8.8.8"
# inet : pas de cap sur -c (lit en ulong). Reproduit cote ftpg, parsing OK.
# (run_diff : marche si le tester est lance avec sudo, sinon ft_ping fail au socket raw)
run_diff           "valeur trop grande -c"     "-c 99999999999999999999 8.8.8.8"
# -V : on vise inetutils 2.0 (pas la 2.2 du tester) + 2 lignes en plus volontaires
expect_diff        "version -V"                "-V"

# --ttl : range 1-255, erreurs style inet (too small / too big / invalid value)
run_diff           "--ttl=0 (too small)"       "--ttl=0 8.8.8.8"
run_diff           "--ttl=256 (too big)"       "--ttl=256 8.8.8.8"
run_diff           "--ttl=-1 (negatif)"        "--ttl=-1 8.8.8.8"
run_diff           "--ttl=abc (non num)"       "--ttl=abc 8.8.8.8"
run_diff           "--ttl seul (missing arg)"  "--ttl"

echo ""
echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}  Divergences assumees${NC}"
echo -e "${CYAN}=========================================${NC}"

expect_diff        "valeur negative -c -5"     "-c -5 8.8.8.8"
expect_diff        "help -? (--help vs -?)"    "--help"

echo ""
echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}  Tests AVEC reseau (sudo requis)${NC}"
echo -e "${CYAN}=========================================${NC}"
echo -e "${YELLOW}Note: les RTT/TTL/seq sont normalises pour la comparaison${NC}"
echo -e "${YELLOW}Authentification sudo (mot de passe demande maintenant)...${NC}"
if ! sudo -v; then
    echo -e "${RED}sudo non dispo, skip tests reseau${NC}"
    exit $((FAIL>0))
fi
# Garde le ticket sudo actif en arriere-plan pendant la duree des tests
( while true; do sudo -nv; sleep 30; done ) &
SUDO_KEEP=$!
trap "kill $SUDO_KEEP 2>/dev/null" EXIT

run_diff           "ping basique 8.8.8.8"      "-c 3 8.8.8.8"            1
run_diff           "ping quiet -q"             "-q -c 3 8.8.8.8"         1
run_diff           "ping numeric -n"           "-n -c 2 8.8.8.8"         1
run_diff           "ping count 1"              "-c 1 8.8.8.8"            1
run_diff           "ping size -s 100"          "-s 100 -c 2 8.8.8.8"     1
run_diff           "ping size -s 0"            "-s 0 -c 2 8.8.8.8"       1
# inet utilise un modele async qui envoie un paquet meme si la reponse precedente
# n'est pas encore arrivee. On reste sequentiel donc on envoie 1 paquet de moins.
expect_diff        "ping deadline -w 2"        "-w 2 8.8.8.8"            1
run_diff           "ping unreachable 240.0.0.1" "-c 2 240.0.0.1"        1
run_diff           "ping verbose unreachable"   "-v -c 2 240.0.0.1"     1
# --ttl + -r : nouveaux flags, runtime
run_diff           "--ttl=64 normal"           "--ttl=64 -c 2 8.8.8.8"   1
run_diff           "--ttl 64 (espace)"         "--ttl 64 -c 2 8.8.8.8"   1
# -r sur loopback : inet utilise SOCK_DGRAM (unprivileged), ft_ping utilise SOCK_RAW.
# SO_DONTROUTE + raw + lo => le kernel ne route pas vers lo, le reply n'arrive jamais.
# Difference kernel-level inevitable sans changer de socket type.
expect_diff        "-r local 127.0.0.1"        "-r -c 2 127.0.0.1"       1
run_diff           "-r unreachable"            "-r -c 1 8.8.8.8"         1

echo ""
echo -e "${CYAN}=========================================${NC}"
echo -e "  ${GREEN}PASS:  $PASS${NC}"
echo -e "  ${RED}FAIL:  $FAIL${NC}"
echo -e "  ${YELLOW}XFAIL: $XFAIL${NC}  (divergences assumees, OK)"
echo -e "${CYAN}=========================================${NC}"

exit $((FAIL > 0))
