#!/bin/bash

# Check if directory and output file are provided
if [ -z "$1" ]; then
    echo "Usage: $0 <.elf>"
    exit 1
fi
bold=$(tput bold)
normal=$(tput sgr0)
underline=$(tput smul)
nounderline=$(tput rmul)

# Input directory
elf_file="$1"

analysis="$(nm --size-sort -l $elf_file)"
code="$(echo "$analysis" | grep -E "* [^bB] ")"
memory="$(echo "$analysis" | grep -E "* [bB] ")"

targets=('wolfssl/' 'lwip/' 'assets_bundle.h.c')
output=""
output+="- ${underline}size${nounderline} ${underline}memory${nounderline}"$'\n'
for val in "${targets[@]}"; do
    size="$(echo "$code" | grep -E "$val" | awk 'BEGIN {sum = 0} {sum += strtonum("0x" $1)} END {print sum}' | numfmt --to=iec)"
    size_memory="$(echo "$memory" | grep -E "$val" | awk 'BEGIN {sum = 0} {sum += strtonum("0x" $1)} END {print sum}' | numfmt --to=iec)"
    output+="$val ${bold}$size${normal} ${bold}$size_memory${normal}"$'\n'
done
size="$(echo "$code" | awk '{sum += strtonum("0x" $1)} END {print sum}' | numfmt --to=iec)"
size_memory="$(echo "$memory" | awk '{sum += strtonum("0x" $1)} END {print sum}' | numfmt --to=iec)"
output+="total ${bold}$size${normal} ${bold}$size_memory${normal}"$'\n'
echo "$(echo "$output" | xargs -n3 printf '%-20s  %-20s  %-20s\n')"
