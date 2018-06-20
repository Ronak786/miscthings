#!/bin/sh
while true; do
rm privkey.pem ../localpkgs/pubkey.pem
../signpkg.sh one-1.2
sleep 1
done
