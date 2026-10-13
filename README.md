# 📏 SonicRuler — Ultrasonic Range Finder That Corrects Itself

**Most ultrasonic range finders lie a little.** The speed of sound
changes with air temperature, so a cheap HC-SR04 is off by ~0.2% per
degree. This ESP32-C3 range finder measures the temperature, corrects
the speed of sound on every reading, and medians 9 samples before it
believes any of them.

## What it does

- Fires a 10 µs trigger pulse (GPIO 4) and times the echo pulse
  (GPIO 5) with microsecond resolution
- Reads the chip's internal temperature sensor and compensates the
  speed of sound: `v = 331.3 + 0.606 × T` m/s
- Takes 9 samples per reading, throws out timeouts and out-of-range
  echoes, and reports the **median** — not the mean, so one wild echo
  can't drag the result
- Times out cleanly at 30 ms and reports `OUT OF RANGE` instead of
  hanging or printing garbage

```
Distance: 42.7cm at 24C
```

## Hardware

- ESP32-C3 board
- HC-SR04 ultrasonic sensor (TRIG on GPIO 4, ECHO on GPIO 5)

## Build and flash

Requires ESP-IDF 6.1.

```bash
idf.py set-target esp32c3
idf.py build flash monitor
```

## Files

All of the code lives in one source file:

- `main/range_finder.c` — trigger/echo timing, temperature
  compensation, median filtering, main loop
- `sdkconfig.defaults` — target selection (ESP32-C3)
- The two `CMakeLists.txt` files are the standard ESP-IDF build glue
