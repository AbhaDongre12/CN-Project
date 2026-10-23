# BlindLink — Change-Triggered V2V Blind-Spot Warning System

BlindLink is a Computer Networks project that explores a low-cost, infrastructure-independent way for nearby nodes to share detected-object information. The prototype uses ESP32 development boards, HC-SR04 ultrasonic sensing, object-state tracking, and ESP-NOW wireless communication. The receiver maintains a local table of neighbouring objects and evaluates the most urgent active warning.

> **Prototype disclaimer:** BlindLink is an academic prototype, not a certified vehicle safety system. Its readings and alerts must not replace mirrors, driver attention, or certified safety equipment.

## Contents

- [Problem and objective](#problem-and-objective)
- [System architecture](#system-architecture)
- [Core approach: CTOSB](#core-approach-ctosb)
- [Hardware](#hardware)
- [Sender wiring](#sender-wiring)
- [Message types](#message-types)
- [Software setup](#software-setup)
- [Build, upload, and serial monitor](#build-upload-and-serial-monitor)
- [Receiver logic and tests](#receiver-logic-and-tests)
- [Current validation status](#current-validation-status)
- [Troubleshooting](#troubleshooting)
- [Team roles](#team-roles)
- [Limitations and future work](#limitations-and-future-work)

## Problem and objective

Blind spots can hide nearby vehicles, cyclists, or pedestrians, especially around large vehicles. A conventional on-board system may only share information with the vehicle that carries the sensor. BlindLink explores whether a low-cost pair of embedded nodes can exchange object-state information directly, without cellular service or a roadside access point.

The project's central objective is to communicate new or changed object information without repeatedly transmitting the full state when nothing meaningful has changed.

## System architecture

```text
SENDER NODE
┌─────────────────────┐
│ HC-SR04 sensors     │
└──────────┬──────────┘
           v
┌─────────────────────┐
│ Distance readings   │
│ and object tracking │
└──────────┬──────────┘
           v
┌─────────────────────┐
│ Local state table   │
│ and change detection│
└──────────┬──────────┘
           v
┌─────────────────────┐
│ Message composition │
│ FULL/DELTA/CLEAR    │
└──────────┬──────────┘
           v
       ESP-NOW
           ⇅
       ESP-NOW
           v
RECEIVER NODE
┌─────────────────────┐
│ Receive and validate│
│ messages            │
└──────────┬──────────┘
           v
┌─────────────────────┐
│ Merge receiver-side │
│ object state        │
└──────────┬──────────┘
           v
┌─────────────────────┐
│ Warning evaluation │
│ and priority choice │
└──────────┬──────────┘
           v
┌─────────────────────┐
│ Warning output      │
│ (as configured)     │
└─────────────────────┘

Receiver acknowledgement (ACK) is returned to the sender
where supported by the implemented protocol.
```

ESP-NOW provides direct communication between compatible ESP32 nodes without requiring a Wi-Fi access point or internet connection.

## Core approach: CTOSB

**Change-Triggered Object State Broadcast (CTOSB)** is the project's approach to reducing redundant state transmissions.

1. Read sensor distance and determine whether an object is present.
2. Track the object and associate it with an object identifier and zone.
3. Compare its current state with the previously stored/reported state.
4. Choose the appropriate action:
   - **NEW:** send a `FULL` message containing the complete state.
   - **CHANGED:** send a `DELTA` message containing the changed field(s).
   - **UNCHANGED:** suppress redundant updates, subject to configured heartbeat behavior.
   - **CLEARED:** send a `CLEAR` message when a tracked object is no longer active.
5. At the receiver, validate the message, update the matching object record, recalculate warning priority, and acknowledge accepted messages where implemented.

The intended benefit is less redundant communication while keeping receiver state useful and current. Bandwidth, energy, and latency improvements should only be claimed after measurement against a defined baseline.

## Hardware

### Sender
- ESP32 DevKit
- Five HC-SR04 ultrasonic sensors
- Breadboards and jumper wires
- USB data cable
- Voltage dividers or level shifters for sensor ECHO signals

### Receiver
- ESP32 DevKit
- One HC-SR04 ultrasonic sensor in the current physical setup
- Breadboard and jumper wires
- Warning hardware such as a buzzer, if connected and enabled by the firmware
- USB data cable

The receiver's physical sensor and warning components are part of the prototype setup. The exact receiver GPIO assignments must be taken from the receiver firmware and confirmed wiring, rather than inferred from the sender configuration.

## Sender wiring

The sender sensor configuration in the project uses these GPIO assignments:

| Sensor | Zone | TRIG | ECHO |
|---|---|---:|---:|
| 1 | Front-left | GPIO 5 | GPIO 18 |
| 2 | Front | GPIO 19 | GPIO 21 |
| 3 | Rear-left | GPIO 27 | GPIO 26 |
| 4 | Front-right | GPIO 17 | GPIO 16 |
| 5 | Rear-right | GPIO 33 | GPIO 32 |

### Electrical safety

- HC-SR04 modules commonly use 5 V power; check the specification of the exact module.
- HC-SR04 ECHO commonly outputs 5 V. ESP32 GPIOs are **not 5 V tolerant**. Use a suitable voltage divider or logic-level shifter on every ECHO line.
- Connect sensor GND and ESP32 GND to a common ground.
- Verify TRIG/ECHO wiring against the code before powering the circuit.
- Use a USB data cable, not a charge-only cable, for serial programming and monitoring.

## Message types

The project uses the following message type identifiers:

| Identifier | Type | Purpose |
|---|---|---|
| `0x01` | `FULL` | Send a full object-state update |
| `0x02` | `DELTA` | Send only changed state fields |
| `0x03` | `CLEAR` | Mark a tracked object as no longer active |
| `0x04` | `HEARTBEAT` | Indicate liveness or support state freshness |
| `0x05` | `ACK` | Acknowledge a received message |

The receiver's state logic also covers sequence checking, rejection of stale updates, unknown-object `DELTA` handling, sequence-number wraparound, and warning-priority recalculation as exercised by the documented native tests. Exact packet fields and implementation details are defined by the source code.

## Software setup

Requirements:

- Visual Studio Code
- PlatformIO IDE extension
- ESP32 Arduino framework as configured in `platformio.ini`
- Appropriate USB-to-UART driver for the board (for example, CP210x or CH340)

Open the repository folder in VS Code and run commands in the PlatformIO terminal.

## Build, upload, and serial monitor

COM numbers can differ between computers. The commands below use the current example setup: sender on `COM3` and receiver on `COM4`. Check available ports with `pio device list`.

### Sender — COM3

Build:

```powershell
pio run -e person2_sender
```

Upload:

```powershell
pio run -e person2_sender -t upload --upload-port COM3
```

Open serial monitor:

```powershell
pio device monitor -p COM3 -b 115200
```

### Receiver — COM4

Build:

```powershell
pio run -e esp32dev
```

Upload:

```powershell
pio run -e esp32dev -t upload --upload-port COM4
```

Open serial monitor:

```powershell
pio device monitor -p COM4 -b 115200
```

Close the serial monitor on a board before uploading new firmware to that same port.

## Receiver logic and tests

The receiver-side software maintains object records rather than treating every packet as a separate object. A `DELTA` changes only the fields present in the message; fields omitted from a delta should remain unchanged. A `CLEAR` removes an object from active warning evaluation, after which priority is recalculated.

The native receiver tests documented for this project cover:

| Test scenario | Expected/observed behavior in the documented native tests |
|---|---|
| `FULL` at 250 cm | Add object; warning `NONE` |
| `DELTA` distance to 180 cm | Apply update; warning `CAUTION` |
| `DELTA` zone update | Update zone without losing other state |
| Old sequence number | Reject stale update |
| Add another object at 180 cm | Recalculate overall warning |
| Add object at 80 cm | Warning escalates to `DANGER` |
| Clear the dangerous object | Remove it from priority; warning returns to `CAUTION` |
| `DELTA` for unknown object | Reject update |
| Sequence `255` followed by `0` | Accept valid sequence wraparound case |
| Empty receiver state | No priority object; warning `NONE` |

These are native/software test results for simulated messages. They do not by themselves measure physical wireless range, packet loss, sensor accuracy, end-to-end latency, or real-world safety.

## Current validation status

The project evidence documents:

- Receiver native tests for state creation/merge, stale sequence rejection, unknown-object delta rejection, clearing, sequence wraparound, and warning-priority selection.
- Physical ESP32 and HC-SR04 breadboard prototypes.
- Sender sensor diagnostics reporting five configured sensors and valid readings in the observed scan.
- Sender/receiver ESP-NOW message exchange and acknowledgement logs observed during later hardware testing.

The following still need controlled, repeatable evidence before making stronger performance claims:

- Compare sensor readings with measured reference distances across the working range.
- Record paired sender and receiver logs for known message sequences.
- Test warning transitions and automatic buzzer activation/reset on the physical receiver.
- Measure message delivery, packet loss, and end-to-end warning latency over repeated trials.
- Compare CTOSB message counts/bytes with a defined periodic-full-update baseline.

## Troubleshooting

### No COM port appears
- Try another known data-capable USB cable and USB port.
- Check Device Manager for an unknown device.
- Install the driver matching the board's USB-to-UART chip.
- Run `pio device list` and use the port Windows actually assigns.

### Sensor shows `NO ECHO` or implausible readings
- Check VCC, GND, TRIG, and ECHO connections.
- Confirm GPIO numbers match the firmware.
- Verify ECHO voltage-level protection.
- Test sensors individually with a flat target at a known distance.

### Receiver does not show messages or ACKs
- Confirm both boards are running the correct firmware.
- Check the sender and receiver serial monitors.
- Verify ESP-NOW peer configuration, MAC addresses, and channel compatibility in the code.
- Confirm the message definitions used by both sides match.

### Warning output does not activate
- First test the buzzer/output independently.
- Confirm the GPIO, polarity, power, and wiring against the receiver code.
- Then test the full message-to-warning path using a known `DANGER` state.
- Do not claim automatic warning activation until it has been observed and recorded.

## Team roles

| Team role | Responsibility |
|---|---|
| Person 1 — Sensing and Object Tracking | Read sensor data, assign zones/object IDs, and track detections |
| Person 2 — Core Algorithm and V2V Communication | Maintain local state, detect changes, compose messages, and handle wireless communication |
| Person 3 — Receiver and Warning System | Validate messages, merge receiver-side state, manage warning priority, and support integration testing |

## Limitations and future work

- Ultrasonic readings depend on target shape, angle, surface, and environmental conditions.
- Multiple ultrasonic sensors can interfere; sequential sampling can reduce but not eliminate cross-talk.
- ESP-NOW performance depends on radio conditions and correct peer/channel configuration.
- A single distance sensor cannot classify every road user or provide complete vehicle awareness.
- Add repeatable performance measurements, more multi-node tests, and robust stale-state handling.
- Review packet validation and replay/stale-message protections as the design develops.

## Repository

GitHub: [AbhaDongre12/CN-Project](https://github.com/AbhaDongre12/CN-Project)

Keep this README synchronized with the files actually committed to the repository, especially if sender or receiver source files are moved or renamed.

## License

No license is specified here. Add a `LICENSE` file if you intend to define permissions for reuse, modification, or redistribution.
