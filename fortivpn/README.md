# FortiVPN client setup

This directory contains a couple of config files I use to set up my fortivpn connection.

- [fortivpn.sh](./fortivpn.sh): Copy to `/usr/local/bin/fortivpn.sh`, customize and `chmod 0755`. The main goal of this file is to call `resolvectl domain ppp0 <VPN domain>` once the vpn is up, because it seems that openfortivpn and systemd-resolved don't play nicely together otherwise.
- [config](./config): Copy to `/etc/openfortivpn/config` and customize.
- [fortivpn.desktop](./fortivpn.desktop): Copy to `~/.local/share/applications/fortivpn.desktop`, and `chmod 0755`.

Once the files are in place and customized, I start my client with:

```bash
SUDO_ASKPASS=/usr/bin/ksshaskpass ./termtray -- /usr/bin/sudo -A /usr/local/bin/fortivpn.sh
```

If you install `termtray` under `/usr/local/bin/termtray`, the desktop launcher in [fortivpn.desktop](/home/rafa/projects/termtray/fortivpn/fortivpn.desktop) keeps working unchanged.
