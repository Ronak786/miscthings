#!/bin/bash

cd /home/sora

nohup /usr/local/bin/sslocal -c /etc/shadowsocks.json &
/etc/init.d/privoxy start
