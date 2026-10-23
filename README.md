# BlindLink — ESP32-Based Obstacle Detection and Warning System

BlindLink is an ESP32-based prototype for detecting nearby obstacles and communicating their state wirelessly to a receiver. The sender uses HC-SR04 ultrasonic sensors to measure distance, assigns readings to directional zones, tracks changes in detected objects, and sends updates over ESP-NOW. The receiver side is designed to maintain an object-state table, evaluate warning levels, and acknowledge received messages.

> **Project status:** Academic hardware prototype. Test sensor readings, wireless messages, warning behavior, and power connections before relying on the system in real-world use.

## Features

- Five-zone ultrasonic sensing on the sender:
  - Front-left
  - Front
  - Rear-left
  - Front-right
  - Rear-right
- Sequential sensor sampling to help reduce ultrasonic cross-talk.
- Object tracking and change detection.
- ESP-NOW communication between sender and receiver.
- Message types for full updates, delta updates, clear events, heartbeats, and acknowledgements.
- Reliable-send/ACK handling and retry logic for clear events.
- Serial diagnostics for sensor readings and communication testing.

## System Overview

```text
HC-SR04 sensors
      |
      v
 ESP32 Sender
      |
      | ESP-NOW: FULL / DELTA / CLEAR / HEARTBEAT
      v
 ESP32 Receiver
      |
      +--> Update object-state table
      +--> Evaluate warning level
      +--> Trigger warning output (if configured)
      |
      +<-- ACK
```

ESP-NOW provides the wireless sender–receiver link; no physical signal wire is required between the two ESP32 boards.

## Hardware

### Sender
- ESP32 DevKit
- Five HC-SR04 ultrasonic sensors
- Breadboard(s) and jumper wires
- USB data cable for programming and power
- Voltage dividers or level shifters for sensor ECHO signals

### Receiver
- ESP32 DevKit
- Receiver-side warning components, if configured in the receiver firmware
- Breadboard(s), jumper wires, and USB data cable

Use the actual receiver firmware and wiring to confirm warning-output pins; do not assume example GPIO assignments match your build.

## Sender Sensor Pin Mapping

The sender firmware's sensor configuration uses these GPIO assignments:

| Sensor | Zone | TRIG GPIO | ECHO GPIO |
|---|---|---:|---:|
| 1 | Front-left | 5 | 18 |
| 2 | Front | 19 | 21 |
| 3 | Rear-left | 27 | 26 |
| 4 | Front-right | 17 | 16 |
| 5 | Rear-right | 33 | 32 |

### Power and electrical safety

- HC-SR04 modules are commonly powered from **5 V**; verify the requirements of your specific module.
- HC-SR04 ECHO commonly outputs **5 V**, while ESP32 GPIOs are **not 5 V tolerant**. Use a suitable voltage divider or level shifter on each ECHO line before connecting it to an ESP32 GPIO.
- Connected components must share a common ground.
- Use a USB data cable for programming; charge-only cables will not provide serial communication.

## Software Requirements

- Visual Studio Code
- PlatformIO IDE extension
- ESP32 Arduino framework supported by the project's PlatformIO configuration
- USB-to-UART driver appropriate for the ESP32 board (many boards use CP210x or CH340)

## Build and Upload

Open the project folder in VS Code and run commands in the PlatformIO terminal.

### Sender

Build:

```powershell
pio run -e person2_sender
```

Upload (replace `COM3` if Windows assigns another port):

```powershell
pio run -e person2_sender -t upload --upload-port COM3
```

Open the serial monitor:

```powershell
pio device monitor -p COM3 -b 115200
```

### Receiver

If receiver firmware and the `esp32dev` environment are present in your local project, build it with:

```powershell
pio run -e esp32dev
```

Upload (replace `COM4` with the receiver's actual port):

```powershell
pio run -e esp32dev -t upload --upload-port COM4
```

Open the receiver serial monitor:

```powershell
pio device monitor -p COM4 -b 115200
```

Windows may assign different COM port numbers on different computers. Check **Device Manager → Ports (COM & LPT)** or run:

```powershell
pio device list
```

Close a board's serial monitor before uploading firmware to that same port.

## ESP-NOW Message Types

| Type | Name | Purpose |
|---|---|---|
| `0x01` | `FULL` | Send a full object/state update |
| `0x02` | `DELTA` | Send a changed object/state update |
| `0x03` | `CLEAR` | Clear an object that is no longer detected |
| `0x04` | `HEARTBEAT` | Indicate that a node is active |
| `0x05` | `ACK` | Acknowledge a received message |

The sender tracks objects and detects state changes before sending updates. Reliable sends wait for acknowledgements; pending clear events are retained for retry when delivery is not confirmed.

## Testing Checklist

1. Build the sender firmware successfully.
2. Verify all five sensors produce plausible readings at known distances.
3. Check sender serial diagnostics for valid and invalid sensor readings.
4. Confirm the receiver gets sender updates and returns acknowledgements.
5. Test object appearance, distance/zone changes, and clear events.
6. Test that stale or out-of-order messages do not incorrectly restore cleared objects.
7. Repeat tests after restarting both boards.

A successful build alone does not prove that the sensors are calibrated or that all wireless edge cases are handled.

## Troubleshooting

### No COM port appears
- Try a known data-capable USB cable and a different USB port.
- Check Device Manager for an unknown device.
- Install the driver matching the board's USB-to-UART chip (for example, CP210x or CH340).
- Run `pio device list` to see ports PlatformIO detects.

### Sensor shows `NO ECHO` or invalid readings
- Check sensor VCC and GND.
- Verify TRIG/ECHO pin mapping against the firmware.
- Check jumper wires and breadboard connections.
- Ensure the ECHO line has suitable voltage-level protection.
- Test one sensor at a time with a flat object at a known distance.

### Receiver does not acknowledge messages
- Confirm both boards are powered and running the correct firmware.
- Check serial logs on both boards.
- Verify ESP-NOW peer configuration and MAC addresses in the project code.
- Ensure the receiver is listening and uses compatible message definitions.

## Repository Structure

```text
CN-Project/
├── common/       # Shared types, sensor utilities, and common logic
├── sender/       # Sender firmware and communication integration
├── platformio.ini
└── README.md
```

The public repository currently lists `common/` and `sender/`. If receiver firmware is maintained locally or separately, add its directory and setup instructions before describing this as a complete single-repository build.

## Limitations

- Ultrasonic readings vary with object shape, angle, surface, and environmental conditions.
- Sequential sampling helps reduce cross-talk but cannot eliminate every source of interference.
- ESP-NOW reliability depends on peer configuration, radio conditions, and the application's ACK/retry handling.
- BlindLink is a prototype and is not a certified mobility or safety device.

## Contributors

Built as a collaborative Computer Networks project. Add contributor names and individual responsibilities here.

## License

No license is specified in this README. Add a `LICENSE` file if you intend to define how others may use, modify, or distribute this project.
