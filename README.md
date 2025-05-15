# Wii PNG to TPL Converter

This is a Wii homebrew application that allows you to browse PNG files on your SD card and convert them to TPL format using a pointer-based UI with GRRLIB.

It also includes embedded graphics (like a logo and background) from pre-converted `.tpl` files (`LogoN64.tpl`, `Back_a.tpl`) using `bin2c`.

---

## 📁 Project Structure

.
├── Makefile
├── source/
│ ├── main.c
│ ├── LogoN64.h # Embedded logo as TPL
│ └── Back_a.h # Embedded background as TPL
├── data/
│ ├── pointer.png # Pointer icon
│ └── font.ttf # UI font
└── build/ # Build output

---

## 🛠 Requirements

- [devkitPro](https://devkitpro.org/wiki/Getting_Started)
- `devkitPPC` toolchain
- Portlibs:
  - `wii-grrlib`
  - `wii-libpng`, `wii-zlib`, `wii-libfat`, `wii-wiiuse`, `wii-libogc`
- Optional: `bin2c` (for embedding `.tpl` files into `.h` headers)

Install with:

```
sudo dkp-pacman -S wii-grrlib wii-libpng wii-zlib wii-libfat wii-wiiuse wii-libogc
```

## 🔧 Building


Run this in the project root:

```
make
```

## 🎮 How to Use

Launch from Homebrew Channel.

Use Wii Remote pointer to navigate.

Click a .png to convert it.

Converted files are saved to /arc/timg/filename.png.tpl.


## 📦 Credits

GRRLIB

libpng, libfat, and devkitPro contributors


Press Home to exit.

## 🧽 Clean Build
```
make clean
```
