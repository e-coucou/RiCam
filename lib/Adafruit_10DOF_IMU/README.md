# Adafruit 10DOF IMU
*Particle port of the Adafruit 10-DOF IMU Breakout L3GD20H LSM303 BMP180*

This is a port of the Adafruit libraries for the 10DOF (10 degree of freedom) IMU (Inertial Measurement Unit) breakout board for the Particle Photon, Electron, etc.

Wiring:

* Boad <-> Photon
* VIN  <-> 3V3
* GND  <-> GND
* SCL  <-> D1
* SDA  <-> D0
* Leave the rest of the pins unconnected


Board Description and purchase link: 
[https://www.adafruit.com/products/1604] (https://www.adafruit.com/products/1604)

[Adafruit How-To] (https://learn.adafruit.com/adafruit-10-dof-imu-breakout-lsm303-l3gd20-bmp180/introduction)

It is ported from these libraries:

- [Adafruit Unified Sensor Library] (https://github.com/adafruit/Adafruit_Sensor)
- [LSM303DLHC Library] (https://github.com/adafruit/Adafruit_LSM303DLHC)
- [L3GD20 Library] (https://github.com/adafruit/Adafruit_L3GD20_U)
- [BMP180 Library] (https://github.com/adafruit/Adafruit_BMP085_Unified)
- [Adafruit 10DOF Library] (https://github.com/adafruit/Adafruit_10DOF)

To use it in your code, simply add the Adafruit_10DOF_IMU library to your project. It should automatically add the approrpiate include:

```
#include "Adafruit_10DOF_IMU/Adafruit_10DOF_IMU.h"
```

