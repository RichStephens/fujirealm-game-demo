# FujiRealm — CoCo 3 client

A [CMOC](http://perso.b2b2c.ca/~sarrazip/dev/cmoc.html) C client for the
Tandy Color Computer 3. **Work in progress:** the world is drawn as flat
colored 16×16 tiles with small colored markers for players, enemies, items
and shots — placeholder graphics until real art exists.

It speaks the same protocol as the other clients over a plain N: TCP
connection through FujiNet's DriveWire interface (no netstream), polling for
data.

## What it needs

- A CoCo 3 with **128K or 512K**. 512K adds double buffering and the
  hardware-scrolling renderer (CLEAR switches between HW and SW); 128K
  always uses the software renderer.
- A FujiNet for the CoCo (bitbanger or Becker-style), or XRoar with a
  FujiNet-PC.
- An RGB or composite monitor; the palette is chosen on first run and F1
  switches it.

## Build

cmoc and decb are not on the host PATH; build through `defoogi`:

```sh
defoogi make coco SERVER_HOST=192.168.1.100    # from the repo root
```

The output is `FUJIRLM3.dsk`. Like the other clients, the endpoint is baked
in at build time; copy `config.mk.example` to `config.mk` to make it stick.
A host saved from the setup screen (F2) overrides it at runtime. The first
build clones and builds fujinet-lib-experimental into `_cache/`.

## Run

Mount `FUJIRLM3.dsk` and boot: AUTOEXEC runs `FRLOGIN`, which asks for the
display type on first run, logs in (or resumes), then loads the game,
`FRPLAY`.

## Controls

| Key | Action |
| --- | --- |
| Arrows / WASD, or joystick | Move |
| SPACE or joystick button | Fire (a stick's first button press only selects it) |
| ENTER | Talk, pick up, accept |
| H / M / I | Help, map, inventory |
| P | Toggle PvP |
| V | Walk speed |
| F1 | RGB / composite palette |
| CLEAR | HW / SW scrolling (512K) |
| BREAK | Exit to BASIC; in a dialogue, decline |
