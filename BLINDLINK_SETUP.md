# BlindLink ESP32 prototype

This project uses two ESP32 boards with HC-SR04 sensors and ESP-NOW communication.

## Configurable sensor count

Edit `common/sensor_config.h` to add or remove entries from `SENDER_SENSOR_CONFIG` and `RECEIVER_SENSOR_CONFIG`. The active count is derived from each array automatically; there is no separate count to update. `MAX_CONFIGURED_SENSORS` is the allocated capacity (currently 8 sensors per board), not the required number of connected sensors.

Each entry is:

```cpp
{TRIG_GPIO, ECHO_GPIO, ZONE, DETECTION_MAX_CM}
```

Use unique TRIG/ECHO pins per sensor, and check that the selected ESP32 GPIOs are suitable. Every HC-SR04 ECHO output must be reduced to 3.3 V-safe logic with its own voltage divider. The two sensors on a board are sampled sequentially to reduce crosstalk.

## Human-readable receiver output

The receiver serial monitor runs at 115200 baud. Output labels are:

- `LOCAL SENSOR`: a reading from a sensor wired to this ESP32.
- `REMOTE DETECTION`: a message received from the other vehicle over ESP-NOW.
- `REMOTE OBJECT CLEARED`: a remote object's CLEAR message was received.
- `OVERALL STATUS`: combined local/remote warning state.

## Build

```bash
pio run -e person2_sender
pio run -e esp32dev
```

## Upload and serial monitor

Sender example (COM3):

```bash
pio run -e person2_sender -t upload --upload-port COM3
pio device monitor -p COM3 -b 115200
```

Receiver example (COM7):

```bash
pio run -e esp32dev -t upload --upload-port COM7
pio device monitor -p COM7 -b 115200
```

Close the monitor before uploading. If upload cannot connect, hold BOOT while the uploader says `Connecting...`, then release it when upload begins.

## Notes

- Current default configuration is two sensors per board, suitable for four sensors across two vehicles.
- Current warning thresholds are prototype values (DANGER <= 100 cm, CAUTION <= 200 cm), not validated vehicle safety thresholds.
- An HC-SR04 measures distance to a reflecting surface; it does not identify a vehicle or object type.
- The object tracker and remote state table have finite capacities. If the design grows beyond the existing object capacity, update those capacities and test memory/ID behavior too.
- The current code is a prototype, not a certified automotive safety system.
