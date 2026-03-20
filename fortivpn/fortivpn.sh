#!/bin/bash

# Exit early if the interface already exists
if ip link show ppp0 > /dev/null 2>&1; then
    echo "[Error] ppp0 interface has already been created." >&2
    sleep 30
    exit 1
fi

cleanup() {
    if ip link show ppp0 > /dev/null 2>&1; then
        ip link del ppp0 > /dev/null 2>&1 || true
    fi
}

trap cleanup EXIT

# 1. Wait for PPP interface to be created, then configure DNS
wait_for_dns() {
    echo "[DNS setup] waiting for ppp interface to show up..."
    while ! ip addr show ppp0 > /dev/null 2>&1; do
        sleep 1
    done
    echo "[DNS setup] ¡ppp0 detected! Refreshing DNS in 5 seconds..."
    sleep 5
    resolvectl domain ppp0 local.domain
    resolvectl flush-caches
    echo "[DNS setup] DNS setup done"
}

# 2. Run detector in background
wait_for_dns &

# 3. Run forticlient in foreground
openfortivpn -c /etc/openfortivpn/config --pinentry=pinentry-qt
