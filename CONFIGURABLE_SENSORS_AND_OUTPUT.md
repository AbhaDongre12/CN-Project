# BlindLink — configurable sensors and readable receiver output

## What changed

- The number of configured sensors is derived from the entries in `common/sensor_config.h`.
- Both sender and receiver use the same reusable `SensorManager`; sensor reading code is not duplicated for each sensor.
- Receiver local sensor state is stored in a capacity-based array and loops over `localSensors.count()`, rather than assuming exactly two sensors.
- Receiver serial output clearly distinguishes `LOCAL SENSOR` readings from `REMOTE` packets and prints an overall warning summary when the warning state changes.
- A no-echo result does not immediately clear the last local detection; it is retained temporarily and then expires.
- Close-range packet validation in the receiver should accept measurements from 2 cm. Confirm this in `receiver/ReceiverCommunication.cpp` if you have replaced that file locally with a different version.

## Add or remove sensors

Edit only the appropriate configuration array in `common/sensor_config.h`.

Example entry:

```cpp
{17, 19, ZONE_REAR_LEFT, 250}
```

The fields are:

1. TRIG GPIO
2. ECHO GPIO
3. Zone/direction
4. Detection maximum in cm

The sensor count is calculated automatically from the array. Do not manually increment a separate count.

`MAX_CONFIGURED_SENSORS` is the memory capacity reserved by the manager, currently 8. The number of active sensors is not fixed at 8; it is the number of entries in each configuration array. If you need more than 8, increase this capacity and check GPIO availability, RAM, sensor interference, and tracking capacity.

## Default setup in this archive

Each ESP32 is configured for two sensors (front-left and front-right), which matches a four-sensor/two-vehicle prototype. The example GPIOs are placeholders based on the project's existing pin choices. Check every entry against your actual wiring before powering the boards. Each HC-SR04 ECHO pin must be reduced to a safe ESP32 logic level with its own voltage divider.

## Build and upload

From the project root:

```bash
pio run -e person2_sender
pio run -e esp32dev
```

Sender upload (COM3 example):

```bash
pio run -e person2_sender -t upload --upload-port COM3
pio device monitor -p COM3 -b 115200
```

Receiver upload (COM7 example):

```bash
pio run -e esp32dev -t upload --upload-port COM7
pio device monitor -p COM7 -b 115200
```

Close the serial monitor before uploading. If upload fails to connect, hold BOOT while the tool shows `Connecting...`, then release it once upload starts.

## Receiver Serial Monitor labels

- `LOCAL SENSOR ...`: reading from a sensor physically connected to this ESP32.
- `REMOTE ...`: packet/object data received from the other vehicle (printed by the receiver communication module).
- `OVERALL STATUS`: combined warning level derived from local and remote data.

## Important limits

- Each HC-SR04 needs unique TRIG and ECHO pins. Do not tie multiple sensor signals together.
- Trigger sensors sequentially to reduce acoustic crosstalk.
- `vehicle_id` identifies the vehicle reporting a remote packet; `object_id` identifies the tracked object. Keep local readings separate from remote records.
- The warning thresholds are prototype values, not validated automotive safety thresholds.
- Sensor counts are configurable within the fixed memory capacity, but the current object tracker and receiver table still have finite object capacity. Increase those capacities only after reviewing memory use and protocol/object-ID behavior.
