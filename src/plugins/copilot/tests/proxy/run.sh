#!/bin/sh

touch /var/log/squid/access.log
chmod 640 /var/log/squid/access.log
chown proxy:proxy /var/log/squid/access.log
tail -f /var/log/squid/access.log &
exec squid --foreground
