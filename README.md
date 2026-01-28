<h1 align="center">
  <br>
  <img src="https://raw.githubusercontent.com/Shim06/Anemoia/main/assets/Anemoia.png" alt="Anemoia" width="150">
  <br>
  <b>Anemoia-ESP32</b>
  <br>
</h1>

<p align="center">
  Anemoia-ESP32 is a rewrite and port of the Anemoia Nintendo Entertainment System (NES) emulator running directly on the ESP32.  
  It is written in C++ and is designed to bring classic NES games to the ESP32.  
  This project focuses on performance, being able to run the emulator at native speeds and with full audio emulation implemented. However, games with complex mappers may induce a small speed loss.
  <br/>
  Anemoia-ESP32 is available on GitHub under the <a href="https://github.com/Shim06/Anemoia-ESP32/blob/main/LICENSE" target="_blank">GNU General Public License v3.0 (GPLv3)</a>.
</p>

<div align="center">
  <video width="303" height="540" src="https://github.com/user-attachments/assets/d6822dde-2613-4256-b623-08484ff52037"></video>
</div>

---

## Sponsor

[<img width="200" height="69" alt="NextPCB" src="https://github.com/user-attachments/assets/f6b9bda9-1b32-4372-8df8-b126741eb5a7">](https://www.nextpcb.com?code=Shim)


This project is proudly sponsored by [NextPCB](https://www.nextpcb.com?code=Shim). Their support helps fund the development and continuation of this project, and I'm very grateful to have them as my first ever sponsor. 

Want to make a PCB? NextPCB offers PCB fabrication and assembly services with fast turnaround times and affordable pricing to help bring your electronics projects to the next level.

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
| **Kirby’s Adventure**   | MMC3 (4)  | **59.57 FPS** |
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

- ESP32
  - e.g. ESP32-DevKitC or ESP32-WROOM-32
- A 320x240 SPI TFT screen (no touch needed)
  - Either an ST7789-based screen as depicted, or
  - an ILI9341-based screen with 320x240 pixels
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

### Default Pin Setup
## TFT Display
| Signal   | ESP32 Pins     |
|----------|----------------|
| MOSI     | GPIO23         |
| MISO     | -1 (N/A)       |
| SCLK     | GPIO18         |
| CS       | GPIO4          |
| DC       | GPIO2          |
| RST      | EN             |

## MicroSD
| Signal   | ESP32 Pins     |
|----------|----------------|
| MOSI     | GPIO13         |
| MISO     | GPIO34         |
| SCLK     | GPIO26         |
| CS       | GND            |

## Audio Amplifier
| Signal   | ESP32 Pins     |
|----------|----------------|
| Input    | GPIO25         |
              
## Controller
There are currently three input methods: Tactile push buttons, an NES/SNES controller, and a PS1/PS2 controller.

### Tactile Push Buttons
| Signal   | ESP32 Pins     |
|----------|----------------|
| A        | GPIO22 & GND   |
| B        | GPIO21 & GND   |
| Left     | GPIO0 & GND    |
| Right    | GPIO12 & GND   |
| Up       | GPIO15 & GND   |
| Down     | GPIO5 & GND    |
| Start    | GPIO32 & GND   |
| Select   | GPIO27 & GND   |
<br>

### NES/SNES controller

<img width="338" height="187" alt="NES/SNES controller Pinout" src="https://github.com/user-attachments/assets/15c992a0-cdb9-4662-91be-3cf615ce1b41"/>

| Signal   | ESP32 Pins     |
|----------|----------------|
| Clock    | GPIO5          |
| Latch    | GPIO19         |
| Data     | GPIO21         |
<br>

### PS1/PS2 controller

<img width="338" alt="PS1/PS2 controller Pinout" src="https://github.com/user-attachments/assets/f1960910-e42b-432b-a3c2-ec0165d14599"/>

| Signal    | ESP32 Pins     |
|-----------|----------------|
| Data      | GPIO5          |
| Command   | GPIO19         |
| Attention | GPIO21         |
| Clock     | GPIO22         |
<br>


Also connect the power and ground lines if using a controller. 
Most controllers should work fine from 3.3V power supply. 

---

## Getting Started

1. Upload the `Anemoia-ESP32.ino` program into the ESP32
2. Put .nes game roms inside the root of a microSD card
3. Insert the microSD card into the microSD card module
4. Power on the ESP32 and select a game from the file select menu

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
The emulator uses a custom display configuration for the ST7789 display.
1. Navigate to your Arduino Libraries folder:
(Default location): `Documents/Arduino/libraries/TFT_eSPI`
2. Open `User_Setup_Select.h` in a text editor.
3. Comment out `#include <User_Setup.h>` and any other setup includes and add `<User_Setups/Anemoia-ST7789.h>`:
```C++
// #include <User_Setup.h>
#include <User_Setups/Anemoia-ST7789.h>
```
4. Copy the provided `Anemoia-ST7789.h` file from this repository into
`TFT_eSPI/User_Setups/`. Optionally, edit the `#define` pins as desired.
> [!NOTE]
> If using a screen with the ILI9341 driver, open `Anemoia-ST7789.h` in a text editor and comment out `#define ST7789_DRIVER` and uncomment `#define ILI9341_DRIVER`.
> ```C++
> // #define ST7789_DRIVER
> #define ILI9341_DRIVER
> ```

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

## Contributing

Pull requests are welcome. For major changes, please open an issue first
to discuss what you would like to change.

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3) - see the [LICENSE](LICENSE) file for more details.
