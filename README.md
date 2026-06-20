<h1 align="center">
  <br>
  <img src="https://raw.githubusercontent.com/Shim06/Anemoia/main/assets/Anemoia.png" alt="Anemoia" width="150">
  <br>
  <b>Anemoia-ESP32</b>
  <br>
</h1>

<p align="center">
  Anemoia-ESP32 is a rewrite and port of the Anemoia Nintendo Entertainment System (NES) emulator running directly on the ESP32.
  It is written in C++ and is designed to bring classic NES games to the ESP32 with support for both TFT displays and composite video output.
  This project focuses on performance, being able to run the emulator at native speeds and with full audio emulation and save states implemented.
  <br/>
  <b>Flash the emulator instantly using the <a href="https://shim06.github.io/Anemoia-ESP32/" target="_blank">Web Flash</a>!</b>
  <br/>
  Anemoia-ESP32 is available on GitHub under the <a href="https://github.com/Shim06/Anemoia-ESP32/blob/main/LICENSE" target="_blank">GNU General Public License v3.0 (GPLv3)</a>.
</p>

<div align="center">
  <video src="https://github.com/user-attachments/assets/2b766040-4717-4ae2-9f72-5637c5ec5cd3"> </video>
</div>

---

## Sponsor

[<img width="200" height="69" alt="NextPCB" src="https://github.com/user-attachments/assets/f6b9bda9-1b32-4372-8df8-b126741eb5a7">](https://www.nextpcb.com?code=Shim)


This project is proudly sponsored by [NextPCB](https://www.nextpcb.com?code=Shim). Their support helps fund the development and continuation of this project, and I'm very grateful to have them as my first ever sponsor.

Want to make a PCB? NextPCB offers PCB fabrication and assembly services with fast turnaround times and affordable pricing to help bring your electronics projects to the next level.

---

## Table of Contents

- [Performance](#performance)
- [Compatibility](#compatibility)
- [Hardware Overview](#hardware-overview)
  - [Composite Video Output](#composite-video-output)
  - [Original Hardware](#original-hardware)
  - [Cheap Yellow Display](#cheap-yellow-display)
  - [Custom-made PCBs](#custom-made-pcbs)
    - [Module-based PCB](#module-based-pcb)
    - [Discrete PCB](#discrete-pcb)
  - [Where to Buy](#where-to-buy)
- [Controls](#controls)
  - [Menu Access](#menu-access)
  - [Controller Button Mappings](#controller-button-mappings)
- [ROM Backends](#rom-backends)
  - [LRU Cache](#lru-cache-default)
  - [Flash Partition](#flash-partition-mmap)
- [Getting Started](#getting-started)
  - [Option 1 - Web Flash](#option-1---web-flash-recommended)
  - [Option 2 - Build from Source](#option-2---build-from-source)
  - [After Flashing](#after-flashing)
- [How to Build and Upload](#how-to-build-and-upload)
- [How it works](#how-it-works)
- [Contributing](#contributing)
- [License](#license)

---

## Performance
Anemoia-ESP32 is heavily optimized to achieve native NES speeds on the ESP32, running at ~60.098 FPS (NTSC) with 1 frame skip and full audio emulation enabled.

Here are the performance benchmarks for several popular NES games.
> [!NOTE]
> The following benchmarks show average framerates recorded over 8192 frames (~2 minutes) of emulation time. Some games, such as `Kirby's Adventure`, which frequently switch banks may experience significant FPS drops in certain sections.

| Game                    | Mapper    | Average FPS   |
|-------------------------|-----------|---------------|
| **Super Mario Bros.**   | NROM (0)  | **60.10 FPS** |
| **Contra**              | UxROM (2) | **60.10 FPS** |
| **The Legend of Zelda** | MMC1 (1)  | **60.10 FPS** |
| **Mega Man 2**          | MMC1 (1)  | **60.10 FPS** |
| **Castlevania**         | UxROM (2) | **60.10 FPS** |
| **Metroid**             | MMC1 (1)  | **60.10 FPS** |
| **Kirby's Adventure**   | MMC3 (4)  | **59.57 FPS** |
| **Donkey Kong**         | NROM (0)  | **60.10 FPS** |


## Compatibility

As of now, Anemoia-ESP32 has implemented six major memory mappers:
* Mapper 0
* Mapper 1
* Mapper 2
* Mapper 3
* Mapper 4
* Mapper 69

Totalling to around 79% of the entire NES game catalogue.

If you'd like to check if a certain game is supported, visit
[NesCartDB](https://nescartdb.com/) and search for the game on the
right-hand side of the site. Select the specific game version
and look for the `iNES Mapper` number in the cart properties.
The game should be supported if the iNES Mapper number is in the list
of implemented mappers above.

Feel free to open an issue if a game has glitches or fails to boot.

---

## Hardware Overview
Anemoia-ESP32 requires a dual-core ESP32 with a minimum of 1 MB flash memory and <u><strong>NO PSRAM IS REQUIRED</strong></u>.

### Composite Video Output

Anemoia-ESP32 supports composite video output via the `COMPOSITE_VIDEO` define in `config.h`, based on [esp_8_bit](https://github.com/rossumur/esp_8_bit) by Peter Barrett. This lets the emulator output directly to a CRT television or any display with a composite input.

**Additional hardware needed:**
- Any CRT or display with a composite RCA input
- 1kΩ resistor and 10nF capacitor

**Wiring:**
```
-----------
|         |
|      25 |-------------------------------► Video out (RCA)
|         |
|      18 |----[1kΩ]----+---------------► Audio out
|  ESP32  |             |
|         |            ---
|         |            --- 10nF
|         |             |
|         |             ▼ GND
-----------
```

| Signal    | ESP32 Pin |
|-----------|-----------|
| Video out | GPIO25    |
| Audio out | GPIO18    |

> [!NOTE]
> GPIO18 is the default audio pin and can be changed via `AUDIO_PIN` in `config.h`.

To enable composite video, open `config.h` and uncomment:
```cpp
#define COMPOSITE_VIDEO
```

Then set your video standard and audio pin as needed:
```cpp
#define VIDEO_STANDARD 1  // 0 = PAL, 1 = NTSC
#define AUDIO_PIN      18
```

> [!IMPORTANT]
> Composite video and TFT output are mutually exclusive. Enabling `COMPOSITE_VIDEO` disables the SPI display pipeline entirely.

### Original Hardware

- ESP32
  - e.g. ESP32-DevKitC or ESP32-WROOM-32
- A 240x320 SPI TFT screen (no touch needed)
  - Either an ST7789-based screen as depicted, or
  - an ILI9341-based screen with 240x320 pixels
- Audio Amplifier
  - e.g. a PAM8403 or PAM8302
- Speaker
- MicroSD card module
- 8 Tactile push buttons, or
- Supported Controller
  - NES controller
  - SNES controller
  - PS1 controller
  - PS2 controller
  - Serial controller (WebSerial or UART adapter)

> [!NOTE]
> ST7789-based displays are recommended as they seem to fare better with 80MHz SPI speeds and are the most compatible.

> [!IMPORTANT]
> **ILI9341 users:** ILI9341-based screens may experience display problems at 80MHz. Reduce the SPI frequency to **40MHz** in your `User_Setup.h`. This will cause the emulator to run a few FPS slower than ST7789 screens.

### Default Pin Setup
![Default pin schematic](https://github.com/user-attachments/assets/ded0f955-20be-4b0b-87f4-d7528cb23e67)

### TFT Display
| Signal   | ESP32 Pins     |
|----------|----------------|
| MOSI     | GPIO23         |
| MISO     | -1 (N/A)       |
| SCLK     | GPIO18         |
| CS       | GPIO5          |
| DC       | GPIO2          |
| RST      | EN             |

### MicroSD
| Signal   | ESP32 Pins     |
|----------|----------------|
| MOSI     | GPIO13         |
| MISO     | GPIO12         |
| SCLK     | GPIO14         |
| CS       | GND            |

> [!IMPORTANT]
> <img width="169" alt="3V3-microsd-module-img" src="https://github.com/user-attachments/assets/be990b45-e1c7-4b2b-b575-c105c55849c9" />
>
> If using this **3.3V microSD card module**, the pull-up resistor on **MISO (GPIO12)** must be **removed**. GPIO12 is a bootstrapping pin (MTDI) that must be LOW during boot. The external pull-up on the microSD module conflicts with the boot strapping process, preventing the ESP32 from booting correctly.

### Audio Amplifier
| Signal   | ESP32 Pins     |
|----------|----------------|
| Input    | GPIO25         |

### Controller
There are currently four input methods: Tactile push buttons, an NES/SNES controller, a PS1/PS2 controller, and a Serial controller.

### Tactile Push Buttons
| Signal   | ESP32 Pins           |
|----------|----------------------|
| A        | GPIO19 & GND         |
| B        | GPIO26 & GND         |
| Left     | GPIO32 & GND         |
| Right    | GPIO33 & GND         |
| Up       | GPIO15 & GND         |
| Down     | GPIO4 & GND          |
| Start    | GPIO27 & GND         |
| Select   | GPIO16 (RX2) & GND   |
<br>

### NES/SNES controller

<img width="338" height="187" alt="NES/SNES controller Pinout" src="https://github.com/user-attachments/assets/15c992a0-cdb9-4662-91be-3cf615ce1b41"/>

| Signal   | ESP32 Pins     |
|----------|----------------|
| Clock    | GPIO32         |
| Latch    | GPIO33         |
| Data     | GPIO35         |
<br>

### PS1/PS2 controller

<img width="338" alt="PS1/PS2 controller Pinout" src="https://github.com/user-attachments/assets/f1960910-e42b-432b-a3c2-ec0165d14599"/>

| Signal    | ESP32 Pins     |
|-----------|----------------|
| Data      | GPIO32         |
| Command   | GPIO33         |
| Attention | GPIO26         |
| Clock     | GPIO27         |
<br>

Also connect the power and ground lines if using a controller.
Most controllers should work fine from 3.3V power supply.

### Serial Controller

Button presses can be sent over serial via two independent methods, provided by the SerialGameControllerAdapter project. Both can coexist and are handled separately.

#### Method 1 — USB to Serial (WebSerial)
Button input is read over the main USB serial connection. Open [WebSerialController.html](https://jethomson.github.io/SerialGameControllerAdapter/WebSerialController.html) in a Chromium-based browser — it translates keyboard, mouse, touch, or USB controller input into serial button commands. No extra hardware is required, making it ideal for testing Anemoia-ESP32 before any wiring or soldering.

#### Method 2 — UART Adapter (ESP32-to-ESP32)
A second ESP32 running [SerialGameControllerAdapter](https://github.com/jethomson/SerialGameControllerAdapter) firmware. The adapter reads inputs from an NES, SNES, PS1, PS2, or Bluetooth controller and forwards button presses over a secondary serial port (`Serial1`). A separate UART port is used specifically to avoid interfering with USB programming of the main board.

| Signal                    | CYD Pin |
|---------------------------|---------|
| TX (Adapter) → RX (ESP32)   | GPIO22  |
| RX (Adapter) ← TX (ESP32)   | GPIO27  |

---

### Cheap Yellow Display

[Cheap Yellow Displays](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display) (CYD) are an all-in-one ESP32 board that comes with most of the hardware needed in this project already integrated, making it ideal for Anemoia-ESP32. Because of the limited pins brought out by the CYD, it is only practical to use a NES controller or a serial controller.

**Hardware Needed:**
- Cheap Yellow Display
- NES/SNES controller (or serial controller — see [Serial Controller](#serial-controller))
- Speaker (optional) - Can be attached with a 1.25mm JST connector to "SPEAK" or soldered directly

### NES/SNES controller

| Signal   | ESP32 Pins     |
|----------|----------------|
| Clock    | GPIO22 (CN1/P3)|
| Latch    | GPIO27 (CN1)   |
| Data     | GPIO35 (P3)    |

---
### Custom-made PCBs
The schematics, PCB design files, enclosures, and 3D models are available in the `/hardware` and `/3d-model` folder.

#### Module-based PCB
A PCB that provides a clean, organized way to connect and manage all peripheral modules in one place.
![Module-based PCB demo](https://github.com/user-attachments/assets/46687b5f-1b71-4be0-8754-2d366c9603dd)
![Module-based PCB schematic](hardware/Anemoia-ESP32/schematics/Anemoia-ESP32.png)

#### Discrete PCB
A PCB that offers a more complete, permanent, and compact handheld by using discrete ICs instead of breakout modules.
![Discrete PCB demo](https://github.com/user-attachments/assets/29aa4584-0e9d-4032-93b5-9dca02997e03)
![Discrete PCB schematic](hardware/Anemoia-ESP32-SMD/schematics/Anemoia-ESP32-SMD.png)

---

## Where to Buy
These are the recommended parts to use for this project.<br>
*These are affiliate links. Buying through them helps support me at no extra cost to you. Thank you for your support.*

- [ESP32](https://s.click.aliexpress.com/e/_c3B4YJhz)
- [240x320 ST7789 Display](https://s.click.aliexpress.com/e/_c2wkMWbV)
- [PAM8403 Amplifier Module](https://s.click.aliexpress.com/e/_c3EWffgT)
- [MicroSD Card Module](https://s.click.aliexpress.com/e/_c3ORlv7p)
- [TP4056 Charging Module](https://s.click.aliexpress.com/e/_c2xSu8Mn)
- [S09 Buck Converter](https://s.click.aliexpress.com/e/_c4LFB5JD)
- [SS12F17 Slide Switch](https://s.click.aliexpress.com/e/_c3DdrYLV)
- [12×12×7.3mm Tactile Push Buttons](https://s.click.aliexpress.com/e/_c3ABhrhV)
- [40mm Speaker](https://s.click.aliexpress.com/e/_c4Ci9359)

### Cheap Yellow Display
- [ST7789 CYD](https://s.click.aliexpress.com/e/_c30TMd05)

---

## Controls

### Menu Access
Press **Start + Select** simultaneously in a game to open the menu.
Press **Select** to change the ROM backend. See [ROM Storage Backends](#rom-storage-backends) for details.

### Controller Button Mappings

#### SNES Controller
| NES Button | SNES Buttons |
|------------|--------------|
| A          | B, A, R      |
| B          | Y, X, L      |
| Start      | Start        |
| Select     | Select       |
| Up         | D-Pad Up     |
| Down       | D-Pad Down   |
| Left       | D-Pad Left   |
| Right      | D-Pad Right  |

#### PS1/PS2 Controller
| NES Button | PS1/PS2 Buttons              |
|------------|------------------------------|
| A          | R1, R2, R3, X, O             |
| B          | L1, L2, L3, Square, Triangle |
| Start      | Start                        |
| Select     | Select                       |
| Up         | D-Pad Up                     |
| Down       | D-Pad Down                   |
| Left       | D-Pad Left                   |
| Right      | D-Pad Right                  |

---

## ROM Backends

ROMs are always sourced from the SD card. The two backends differ in how the ROM data is accessed at runtime. You can switch between them in the game selection screen by pressing the **Select** button.

### LRU Cache (Default)
ROM data is read from the SD card and cached in RAM using an LRU cache. This works well for most games, but games that frequently switch banks may experience slowdowns.

### Flash Partition (mmap)
Useful for games that run too slowly under the LRU cache due to RAM pressure. On first selection, the ROM is copied from the SD card into a dedicated `nesrom` flash partition. Afterwards, the partition is memory mapped and the mapper reads ROM data directly from flash via `esp_partition_mmap()`. Subsequent launches will skip the copy.

> [!WARNING]
> Every time a ROM is copied to the flash partition, a write cycle is consumed. ESP32 flash memory is rated for a limited number of erase/write cycles (typically ~100,000). Frequently switching ROMs using this backend will very slowly degrade the flash over time and may eventually cause flash corruption or failure. It is recommended to only use this when neccessary.

> [!NOTE]
> Only one ROM can be stored in the flash partition at a time. Selecting a new ROM will overwrite the existing one.

---

## Getting Started

### Option 1 - Web Flash (Recommended)
No software installation required.

1. Visit the [Web Flash](https://shim06.github.io/Anemoia-ESP32/) website.
2. Connect your ESP32 via USB.
3. Click **Flash** and select your ESP32's COM port.

> [!NOTE]
> Web flashing requires a Chromium-based browser (Chrome, Edge, Opera) with WebSerial support. Firefox is not supported.


### Option 2 - Build from Source

1. Build and upload the `Anemoia-ESP32.ino` program into the ESP32 following the [How to build and upload](#how-to-build-and-upload) instructions below.

---
### After Flashing
1. Format your microSD card to `FAT32`.
2. Put .nes game ROMs inside the root of the microSD card.
3. Insert the microSD card into the microSD card module.
4. Power on the ESP32 and select a game from the file select menu.

## How to build and upload

### Step 1

Either use `git clone https://github.com/Shim06/Anemoia-ESP32.git` on the command line to clone the repository or use Code → Download zip button and extract to get the files.

### Step 2
1. Download and install the Arduino IDE.
2. In <b> File → Preferences → Additional boards manager URLs </b> , add:
```cmd
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```
3. Download the ESP32 board support `v3.2.1` through <b> Tools → Board → Boards Manager </b>.
> [!IMPORTANT]
> Make sure to download version 3.2.1, as different board versions may have worse performance.
4. Download the `SdFat` and `TFT_eSPI` libraries from <b> Tools → Manage Libraries </b>.

### Step 3 - Configure TFT_eSPI
Copy and paste the TFT_eSPI configuration file into the TFT_eSPI folder.
1. Navigate to your Arduino Libraries folder:
(Default location): `Documents/Arduino/libraries/TFT_eSPI`
2. Copy your desired `User_Setup.h` file in the `/User_Setups` folder from this repository into
`TFT_eSPI/` and overwrite the file. Optionally, edit the `#define` pins as desired.
> [!NOTE]
> If using a screen with the ILI9341 driver, open `User_Setup.h` in a text editor and comment out `#define ST7789_DRIVER` and uncomment `#define ILI9341_DRIVER`.
> ```C++
> // #define ST7789_DRIVER
> #define ILI9341_DRIVER
> ```
> Also reduce the SPI frequency to **40MHz**. ILI9341 screens are not reliable at 80MHz and will cause display issues. This will result in a small FPS reduction compared to ST7789 screens.

### Step 4 - Apply custom build flags
1. Locate your ESP32 Arduino platform directory. This is typically at:
```cmd
\Users\{username}\AppData\Local\Arduino15\packages\esp32\hardware\esp32\{version}\
```
2. Copy the `platform.txt` file from this repository and paste into that folder.
This file defines additional compiler flags and optimizations used by Anemoia-ESP32.
> [!WARNING]
> Backup your `platform.txt` file if you have your own custom settings already.

### Step 5 - Upload
1. Connect your ESP32 via USB.
2. In the Arduino IDE, go to <b> Tools → Board </b> and select your ESP32 board (e.g., ESP32 Dev Module).
3. Click Upload or press `Ctrl+U` to build and flash the emulator. Optionally, edit the `#define` pins as desired.

## How it works

The ESP32 runs at 240 MHz and has 520 KB of SRAM. That sounds like plenty until you actually try to run an NES emulator on it. Then it stops feeling like plenty very fast. Every optimization here came from hitting a wall and figuring out how to get around it.

### Line Buffer + DMA Rendering

A full NES framebuffer at 256×240 pixels in 16-bit RGB eats 120 KB of RAM. That's roughly a third of the ESP32's total RAM. And this is without accounting for the memory consumed by the emulator itself. So the framebuffer has to go. Instead, only a few scanlines get buffered at a time and pushed to the display in batches.

The problem with pushing constantly is that pushing data over SPI takes time, and that takes precious time from the processor for emulation. The fix is DMA. Instead of the CPU sitting there transferring bytes to the display, you hand it off to the DMA controller and let it run in the background. Resulting in very little overhead in pushing pixels to the display.

The emulator also runs with a constant frame skip of 1. Every other frame gets skipped entirely. The emulation keeps running at full speed, only the display output is affected, and it's what gives the emulator enough headroom to stay stable across pretty much everything the NES throws at it.

### Scanline-Based PPU

The real NES renders one pixel per clock cycle with the PPU and CPU running in tight lockstep. Emulating that timing accurately on the ESP32 just isn't viable. The overhead is just too high to even get anywhere close to 60 FPS.

So instead of rendering dot-by-dot, the PPU renders an entire horizontal line at once. All the sprite evaluation, tile lookups, palette reads, and pixel priority for that row get handled in one pass. Through this, batching CPU and PPU operations can be done. It's less accurate than how the real hardware works, but the performance gain is massive and most games don't care. The ones that rely on mid-scanline PPU tricks are rare enough that it's an acceptable tradeoff.

### Storing Game ROMs

NES cartridges can be up to 1 MB, but the console can only address 40 KB of ROM at a time. Mappers handle this by swapping banks in and out of the address space.

How do you fit a 1 MB game cartridge into a device that has 384KB of RAM, with most of it used by the emulator?

Short answer is you don't. The ESP32 can't hold a full ROM in RAM, so the obvious solution is loading banks from the microSD card when the mapper asks for them.

The obvious solution also turned out to be completely unusable. Games were switching banks several times per frame. Loading a 16–32 KB chunk from SD that often over SPI is way too slow, and the emulator ground to a halt.

The fix was using the remaining free RAM as a cache. Loaded banks stay in RAM, and if the mapper requests one that's already there, no SD read happens. When the cache fills up, the least recently used bank gets evicted. Turns out games tend to cycle through the same small set of banks for any given section, so once the cache is warm, the SD card barely gets touched. The slowdown disappeared entirely.

However, some games will still switch to various different banks too often and tank performance, so there is an additional option of flash memory mapping. By copying the ROM from the SD card into the ESP32's flash and memory mapping it via `esp_partition_mmap()`, you can access it directly as if it were RAM. No dynamic loading, just a pointer. The tradeoff is that constantly rewriting to the flash will slowly degrade it, so it should only be used when neccessary.

### Offloading the Audio Emulation

The NES APU has five sound channels, each with their own timers, envelopes, and sweep units. This is expensive enough to emulate that throwing it onto the main core alongside everything else would have tanked performance.

The saving grace is that the APU isn't tightly coupled to the CPU and PPU. It doesn't need to run in perfect sync, so it can live on the other core entirely on its own. Additionally, Input polling can be offloaded there too. The result is that audio emulation and input polling has basically zero impact on emulation performance.

### Compiler Flags

Once everything else was in place, some extra GCC flags on top of `-Ofast` were applied to let the compiler optimize harder on hot paths. That alone took the emulator from ~58 FPS to ~66 FPS. This leaves enough headroom to hold a stable 60 FPS with room to spare on heavy scenes.

## Contributing

Pull requests are welcome. For major changes, please open an issue first
to discuss what you would like to change.

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3) - see the [LICENSE](LICENSE) file for more details.
