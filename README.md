# Sim Pedal Configurator

![Screenshot](https://github.com/user-attachments/assets/74715fda-f08c-47c7-87a0-bf244eedafb2)

An interactive GUI tool for configuring and calibrating sim racing pedals (clutch, throttle, brake) based on a microcontroller with EEPROM support.

---

## Features

- **Calibration** of pedals directly in the GUI **Automatic detection of Min/Max values**
- Real-time display of raw pedal values
- Adjustment of Arduino pins, deadzone
- Connection via Web Serial API
- Graphical visualization of pedal values using Chart.js
- Debug mode for displaying additional information

---
## Usage

- Connect via COM Port
- **Load** EEPROM
- **Save** EEPROM
- **Calibrate** After clicking on the button fully move your pedals a few times, klick again and save

---

## Hardware

- Microcontroller: Arduino-compatible (e.g., Arduino Leonardo, Pro Micro)
- Pedals:
  - Clutch
  - Throttle
  - Brake using **HX711** sensor (standard Version is locked to 10hz)
- Connected via analog pins and HX711 pins

---

## Installation

1. **Clone the repository:**

```bash
git clone https://github.com/deinusername/sim-pedal-configurator.git
cd sim-pedal-configurator
