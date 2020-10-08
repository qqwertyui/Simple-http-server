#!/bin/bash
# allow non-root users to bind port 80

sudo setcap CAP_NET_BIND_SERVICE=+eip $(pwd)/http_server
