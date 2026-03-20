# termtray

`termtray` is a small Linux desktop app written in C++ with Qt Widgets.

It runs a command, captures its `stdout` and `stderr`, and displays the live output in a compact terminal-style window. When the window is closed, the app hides to the system tray instead of exiting when tray support is available. When the child process exits, the app exits too.

This is useful for long-running terminal commands that you want to keep visible in a small GUI window, while still being able to tuck them away in the tray.

The app icon lives in [assets/icons/termtray.svg](/home/rafa/projects/termtray/assets/icons/termtray.svg). The desktop launcher can reuse that SVG as a normal icon theme asset.

## Features

- Runs any command passed on the command line
- Streams `stdout` and `stderr` into the window in real time
- Terminal-like presentation with a monospace text view and black background
- CRT-style green terminal output by default
- Configurable foreground color with `-color RGB`
- Configurable tray/window title with `-title TEXT`
- Configurable tray/window icon with `-icon NAME`
- Scrollback buffer of 200 lines
- Close-to-tray behavior when the desktop supports a tray
- Tray menu to restore the window after it has been hidden
- Exits automatically when the child process exits
- Quitting from the tray menu or window manager interrupts the child command first

## Requirements

- Linux desktop environment
- A C++17 compiler
- `make`
- `pkg-config`
- Qt development packages for either Qt 6 Widgets or Qt 5 Widgets

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install build-essential make pkg-config qt6-base-dev
```

If your distro still defaults to Qt 5, install `qtbase5-dev` instead.

## Build

```bash
make
```

The resulting binary is `./termtray`.

## Usage

```bash
termtray [-color RGB] [-title TEXT] [-icon NAME] -- <command> [args...]
```

Examples:

```bash
./termtray -- bash -lc 'echo hello; echo error >&2; sleep 2; echo done'
./termtray -color 6f6 -- tail -f /var/log/syslog
./termtray -title "VPN Monitor" -icon network-vpn -- ./fortivpn/fortivpn.sh
```

For a real life example, see [fortivpn/README.md](./fortivpn/README.md)

Note:

- `--` cleanly separates `termtray` options from the child command
- if you omit `--`, the first non-option argument is treated as the command

## Color Flag

The `-color` flag accepts a 3-digit hexadecimal RGB value:

- `6f6`: classic green terminal look
- `6a6`: the current default
- `ccc`: soft light gray
- `fa0`: amber/orange
- `0ff`: cyan

Each digit uses the range `0` to `f` and is expanded to a full 8-bit channel internally.

## Title And Icon Flags

- `-title TEXT` changes the main window title and the tray tooltip text
- `-icon NAME` uses the named icon from the current icon theme for the window and tray icon
- if the requested theme icon is unavailable, `termtray` falls back to its bundled icon, and then to the `utilities-terminal` theme icon

## Tray Behavior

- Closing the window hides it to the tray when tray support is available
- If tray support is unavailable, closing the window logs that it could not minimize to tray and exits instead
- Choosing `Show` from the tray restores it if it is hidden
- On Wayland, restoring does not necessarily mean forcing the window to the foreground
- If the child process exits, the app exits automatically
- If the app is exited through the tray menu or the window manager, the child process is interrupted before shutdown

## `sudo` and Askpass

Commands that expect a terminal may not behave exactly like they do inside a real PTY-backed terminal emulator.

For `sudo`, a common approach is to use askpass mode:

```bash
SUDO_ASKPASS=/usr/bin/ksshaskpass ./termtray -- sudo -A your-command
```

The `-A` flag tells `sudo` to use an askpass helper instead of prompting on a terminal.

## Wayland Notes

On Wayland, applications usually cannot force themselves to the foreground unless the compositor allows it. Because of that:

- restoring a hidden window works
- forcing an already visible window to the front may not work
- this is a Wayland/compositor policy limitation, not specific to this app alone

## Project Status

This project is intentionally small and focused. It is currently aimed at personal Linux desktop use with Qt Widgets.

Potential future improvements:

- PTY-backed execution for more terminal-like behavior
- configurable background color
- configurable font size
- packaging for easy desktop installation

## Development

Common commands:

```bash
make
make clean
```

## License

This project is licensed under the MIT License. See [LICENSE](/home/rafa/projects/termtray/LICENSE).
