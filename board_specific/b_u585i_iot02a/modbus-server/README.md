# Modbus Server example for the STM32 U585
- Modbus server runs on port 1502
- Sensor registers are automatically refreshed every 500ms
- Writeable registers take effect immediately

## Register definitions
The following registers are exposed:

| Register Address | Name               | Description                         | Type      | Access     | Notes                            |
|------------------|--------------------|-------------------------------------|-----------|------------|----------------------------------|
| `0x00`           | `REGISTER_LED`     | LED control register                | `uint16`  | Read/Write | Bit 0x01 is Red, 0x02 is green   |
| `0x01`           | `REGISTER_BUTTON`  | Button press count                  | `uint16`  | Read       | Increments on press              |
| `0x02`           | `REGISTER_ACCEL_X` | Acceleration X                      | `float32` | Read       | `0x02` = Low, `0x03` = High word |
| `0x04`           | `REGISTER_ACCEL_Y` | Acceleration Y                      | `float32` | Read       | `0x04` = Low, `0x05` = High word |
| `0x06`           | `REGISTER_ACCEL_Z` | Acceleration Z                      | `float32` | Read       | `0x06` = Low, `0x07` = High word |
| `0x08`           | `REGISTER_GYRO_X`  | Gyroscope X                         | `float32` | Read       | `0x08` = Low, `0x09` = High word |
| `0x0A`           | `REGISTER_GYRO_Y`  | Gyroscope Y                         | `float32` | Read       | `0x0A` = Low, `0x0B` = High word |
| `0x0C`           | `REGISTER_GYRO_Z`  | Gyroscope Z                         | `float32` | Read       | `0x0C` = Low, `0x0D` = High word |
| `0x0E`           | `REGISTER_MAGN_X`  | Magnetometer X                      | `float32` | Read       | `0x0E` = Low, `0x0F` = High word |
| `0x10`           | `REGISTER_MAGN_Y`  | Magnetometer Y                      | `float32` | Read       | `0x10` = Low, `0x11` = High word |
| `0x12`           | `REGISTER_MAGN_Z`  | Magnetometer Z                      | `float32` | Read       | `0x12` = Low, `0x13` = High word |
| `0x14`           | `REGISTER_HUM`     | Humidity                            | `float32` | Read       | `0x14` = Low, `0x15` = High word |
| `0x16`           | `REGISTER_TEMP`    | Ambient temp                        | `float32` | Read       | `0x16` = Low, `0x17` = High word |
| `0x18`           | `REGISTER_PRES`    | Atmospheric pressure (not working!) | `float32` | Read       | `0x18` = Low, `0x19` = High word |
| `0x20`           | `REGISTER_LIGHT`   | Ambient light (not working!)        | `float32` | Read       | `0x20` = Low, `0x21` = High word |