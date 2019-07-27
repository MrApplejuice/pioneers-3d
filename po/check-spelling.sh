#/bin/sh
#
# Apply a spelling check
#
# Usage:
#   check-spelling.sh <language code>
# where <language code> is e.g. nl
#
# This script is tested on Debian and requires the package spellutils
#
pospell -n $1.po -p aspell -- --encoding utf-8 --lang $1 check %f
