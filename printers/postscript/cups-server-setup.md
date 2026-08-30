# Printing from the Commander X16

By Desert-Fox

## Setup for a Raspberry Pi Print Server for X16EDIT, with Stefan's help

---

Stefan Jakobsson wrote a printer driver for X16 Edit that sends PostScript out
over the network. He asked for people with network printers to try it. I had a
Raspberry Pi sitting in a drawer and a laser printer that has never in its life
understood a PostScript command, so I volunteered.

It works. This is what I did, including the two things that cost me an
afternoon, so you don't have to lose the same afternoon.

I'm not claiming this is the only way or the best way. It's the way that
worked. Stefan answered questions along the way and I'd have been stuck
without him.

### A note on the addresses in here

Every IP address below is made up. They stand in for the ones on my network,
and yours will be different.

- **192.168.1.50** — wherever you see this, that's the Pi. Use your Pi's
  address.
- **192.168.1.60** — that's the printer. Use your printer's address.

Same for usernames and the printer queue name. Substitute your own throughout.
If you copy a command straight out of this document without changing the
addresses, nothing is going to work.

---

## What this setup actually does

The X16 speaks PostScript. Most printers don't. PostScript in the printer was
a premium feature twenty years ago and it mostly died off with the parallel
port. If you happen to own a real PostScript printer, you can point the X16
straight at it and ignore everything below.

The rest of us put a Raspberry Pi in the middle. It runs CUPS, CUPS runs
Ghostscript, and Ghostscript turns the PostScript into whatever your printer
actually understands. Mine is a PCL-only Brother laser and it never knows the
difference.

The chain runs: X16 → Serial & Network card → your LAN → Pi running CUPS →
printer.

Worth being clear about one thing, because it confused me at first. **Nothing
plugs into the X16.** It has no USB and no printer port. Everything leaves
through the network card. The printer connects to the Pi, or sits on your
network where the Pi can reach it. Either works — from the X16's side it's all
the same, because the X16 is only ever talking to the Pi.

## What I used

- A Raspberry Pi. I used an original Model B+ from 2014. 512 MB of RAM, one
  700 MHz core. I expected it to struggle and it never did. Any Pi will do.
- An 8 GB SD card.
- A Brother laser on Wi-Fi. A USB printer plugged into the Pi works the same
  way.
- A Commander X16 with the Serial & Network card and ZiModem.
- Stefan's `X16EDITPD-POSTSCRIPT.DRV` and a copy of X16 Edit.

My Pi is on Ethernet. It doesn't need Wi-Fi — the wireless part of this setup
is the X16's network card, not the Pi. The Pi just has to be reachable on your
LAN.

---

## Building the card

I used the Raspberry Pi Imager from raspberrypi.com. It runs on Windows, macOS
and Linux.

**Choose Device** — pick your actual Pi model. It filters the OS list so you
can't grab something that won't boot.

**Choose OS** — don't take the first entry, that's the full desktop. Go down to
"Raspberry Pi OS (other)" and pick **Raspberry Pi OS Lite**.

Which Lite depends on the board:

- Pi 1, Pi Zero, Zero W → **32-bit**. These are ARMv6 and the 64-bit image will
  not boot on them.
- Pi 3, 4, 5 → **64-bit**.

I flashed the desktop version by mistake on my first pass. On 512 MB it swaps
itself into the ground. Lite gives you a login prompt and nothing else, which
is all a print server needs.

**Choose Storage** — check the size it reports matches the card you actually
put in the reader. Imager wipes whatever you point it at, no second chances.

**Customisation** — don't skip this screen. It saves you needing a keyboard and
monitor on the Pi at all:

- Hostname — something you'll recognize. Mine is `laser`.
- Username and password — your own. Don't use `pi`.
- Wi-Fi — I filled it in even though I'm on Ethernet. Costs nothing and it's a
  way back in if the cable situation changes.
- Locale and keyboard.
- **Enable SSH** — password authentication is fine.
- Raspberry Pi Connect — I left it off. No reason to route through a cloud
  service to reach a box on my own desk.

Write it, card in the Pi, boot it.

## First boot

Log in at the console. The banner shows the address it picked up.

Get the MAC so you can pin the address down:

```
ip -br link
```

Take the `eth0` line if you're wired, `wlan0` if you're not, and set a DHCP
reservation on your router. Do this early. You're going to type this address
into the X16 by hand, and you don't want it moving after a power cut.

Reboot, then check it came back where you put it:

```
hostname -I
```

Then get off the console and SSH in from your regular machine.

**Windows** — the SSH client is built in now, no PuTTY needed. Command Prompt
or Terminal:

```
ssh youruser@192.168.1.50
```

I keep a batch file on the desktop so I don't retype it:

```
@echo off
ssh youruser@192.168.1.50
pause
```

**macOS or Linux** — same command in Terminal. Or put an entry in
`~/.ssh/config`:

```
Host laser
    HostName 192.168.1.50
    User youruser
```

and then it's just `ssh laser`.

Update everything before going further:

```
sudo apt update && sudo apt full-upgrade -y
```

On the B+ this takes a while. Go get coffee.

## Installing CUPS

```
sudo apt install cups
sudo usermod -a -G lpadmin youruser
```

That group is what lets you administer printers through the web interface.

**Log out and back in now.** Group membership doesn't apply to a session that
was already open. I skipped this and spent twenty minutes convinced I was
mistyping my own password.

Then turn on network access:

```
sudo cupsctl --remote-admin --share-printers
```

Stefan's notes say `cupsctl --remote any`. That option doesn't exist anymore —
it's been replaced by `--remote-admin`, `--remote-any` and `--share-printers`.
Use `--remote-admin`, which covers your LAN. `--remote-any` opens it to the
internet and you don't want that.

## The part that cost me the afternoon

Check whether CUPS is actually listening:

```
ss -tln | grep 631
```

If a line comes back showing `*:631`, you're fine, skip ahead. I got nothing.

Here's what's happening. On current Debian, cupsd starts through systemd socket
activation — the unit runs `/usr/sbin/cupsd -l`. In that mode cupsd only uses
the sockets systemd hands it and **ignores the `Port 631` line in cupsd.conf
completely**. So you can open the config file, see `Port 631` sitting right
there in plain text, and still have nothing listening on the network.

I checked that config file three times before I worked out it wasn't being
read.

The fix is to tell the socket unit to listen on TCP:

```
sudo mkdir -p /etc/systemd/system/cups.socket.d
sudo tee /etc/systemd/system/cups.socket.d/tcp.conf > /dev/null <<'EOF'
[Socket]
ListenStream=631
EOF
sudo systemctl daemon-reload
sudo systemctl restart cups.socket
```

Check again:

```
ss -tln | grep 631
```

You want `LISTEN 0 4096 *:631 *:*`.

One thing that looks broken but isn't: `systemctl status cups` shows inactive
after a minute or so of no traffic. That's how socket activation works. The
socket stays listening and fires cupsd up when something connects. As long as
`cups.socket` says `active (listening)`, you're good.

## Finding the printer

Skip this if your printer is on the Pi's USB port — it'll show up under Local
Printers when you go to add it.

For a network printer you need its actual IP address.

```
sudo apt install avahi-utils
avahi-browse -rt _ipp._tcp
```

That lists every IPP printer announcing itself. **Read the output carefully.**
This is the second thing that got me. Once CUPS is running and sharing, the Pi
advertises its own print queue — under the printer's name. So you get entries
that look exactly like your printer but resolve to the Pi.

Tell them apart by the hostname:

```
hostname = [BRW001122334455.local]     <- a real Brother. BRW plus its MAC.
hostname = [laser.local]               <- my Pi, echoing its own queue back.
```

The `address = [...]` line under the real printer entry is the one you want.

If the printer doesn't show up, or shows up but times out when it tries to
resolve, it's asleep. Wi-Fi printers shut the radio down in deep sleep and the
old announcement hangs around after they stop answering. Press a button on the
panel to wake it and browse again. Mine did exactly this and I spent a while
pinging an address with nothing on it.

Give the printer a DHCP reservation too. It's about to be hardcoded into a CUPS
queue.

## Adding the printer

From a browser on your regular machine:

```
http://192.168.1.50:631
```

**Administration**, then **Add Printer**. It asks for your Pi login.

If you get bounced with "Upgrade Required" and a mangled IPv6 address in the
message, go straight to `https://192.168.1.50:631/admin` instead. Note the
**s** — CUPS forces admin pages onto HTTPS with a self-signed certificate.
Accept the browser warning, it's your own machine.

A USB printer shows up under **Local Printers**. A network printer will be in
the discovered list, probably several times over on different protocols — take
the **driverless** entry if one's there.

Then:

- **Name** — short, no spaces, no underscores, something you can type on a
  Commodore keyboard. I used `hl2460`. Don't reuse your hostname or you'll
  confuse yourself later.
- **Share This Printer** — check it. The X16 reaches this queue over the
  network, so it has to be shared.
- **Make** — **Generic**.
- **Model** — **IPP Everywhere**.

Don't take "Generic PostScript Printer" just because this is a PostScript
project. That tells CUPS your printer speaks PostScript and passes it through
untouched. If your printer really spoke PostScript you wouldn't need the Pi at
all. IPP Everywhere asks the printer what it can do and converts to match.

You don't need a PPD file. Leave that field alone.

## Pointing the queue at an IP instead of a name

Network printers only.

When CUPS discovers a printer over mDNS, the queue ends up pointing at
something like:

```
ipps://Brother%20HL-L2460DW._ipps._tcp.local/
```

That works until it doesn't. It depends on mDNS resolving every single time,
and there's an encoded space in the middle of it that has caused people
trouble. I pointed mine at the IP.

The web interface won't let you change this on a discovered printer — the field
is greyed out on the Add screen and on Modify Printer both. Command line:

```
sudo lpadmin -p hl2460 -v ipp://192.168.1.60/ipp/print
```

That edits the existing queue in place. Name, sharing and driver all stay as
they were.

Then make sure the Pi can reach it:

```
ping -c 3 192.168.1.60
```

If that fails, nothing past this point will work. Fix it first.

## Proving the Pi side works

In the CUPS web interface: **Printers** → your queue → **Maintenance** →
**Print Test Page**.

Do this before you touch the X16. It matters more than it looks — the CUPS test
page is itself a PostScript document, so if paper comes out you've just proved
that PostScript goes in one end and your non-PostScript printer produces a page
at the other. That's the entire job this Pi exists to do.

It also means that if printing fails later, you already know which half to
suspect.

## The X16 side

Copy these to the **root folder** of the X16's SD card:

- `X16EDITPD-POSTSCRIPT.DRV`
- Your copy of X16 Edit

Only one file starting with `X16EDITPD-` can be on the card at a time. If you
already have another printer driver there, rename it so it doesn't start with
that prefix.

Boot X16 Edit, type a few lines so you have something to print, and hit
**Ctrl+H**.

Down the dialog:

- **I/O PORT** — has to match the jumpers on your Serial & Network card. Wrong
  value gives "UART error-check I/O port".
- **BAUD RATE** — has to match what ZiModem is actually set to. You can't change
  it from this dialog; it's set with `ATB` from a terminal program. Wrong value
  gives "UART error-check configuration".
- **PAPER SIZE** — A4 or Letter. Match what's in the tray and what you set in
  CUPS.
- **UNITS / MARGINS / FONT SIZE / LINE SPACING** — cosmetic. Defaults are fine.
- **IPP ADDRESS** — this one:

```
192.168.1.50:631/printers/hl2460
```

Your Pi's address, port 631, then `/printers/` and your queue name. No
`http://` on the front.

Hit **Ctrl+S** to save settings before you print, so you don't have to retype
all that if the first attempt errors out. Then **Ctrl+P** to print.

Mine printed first try once I had the address right.

## When it doesn't work

**"UART error-check I/O port"** — the I/O port doesn't match your card jumpers.

**"UART error-check configuration"** — nearly always the baud rate. ZiModem
isn't answering `AT` at the speed you picked.

**"Write timeout"** — ZiModem went about a minute without the far end reading a
byte. Check the Pi is up and CUPS is listening on 631.

**"Document was sent to printer" but nothing comes out** — the X16 did its part
and the problem is downstream. On the Pi:

```
lpstat -o
sudo ls -al /var/spool/cups
```

No spool file means the job never arrived — go back and check the address you
typed into X16 Edit, character by character. If there is a spool file, copy it
somewhere readable and see whether it's valid PostScript:

```
sudo cp /var/spool/cups/FILENAME ./
sudo chmod a+r FILENAME
ps2pdf FILENAME
```

If Ghostscript chokes on it, the PostScript arrived damaged. If it produces a
PDF, open it — if that PDF looks right, the trouble is between CUPS and your
printer and has nothing to do with the X16.

**Printer dropped off the network** — it's asleep. Wake it and re-check its
address.

---

Thanks to Stefan Jakobsson for the driver and for answering a lot of questions
while I got this running.
