# DS18B20-Data-Logger

A data logger is an electronic device used to store data based on time or location. The type of data can encompass virtually anything: distance, speed, acceleration, temperature, pressure, humidity, current, voltage, resistance…

To keep track of events, it's important to match each event with its corresponding time, location, or both. Subsequently, a storage device is required to hold that data.

Depending on the volume of data, you can use the internal memory of the microcontroller, an external memory chip, memory cards, or even hard drives.

For this project, you will need the following items:

- 1x Arduino Nano 3.0
- 1x DS1307 real-time clock module
- 1x SD card module (with voltage conversion chip from 5V to 3.3V)
- 1x SD card
- 3x DS18B20 temperature sensor
- 1x 4.7kOhm resistor
- 1x 10kOhm resistor
- Breadboard and wires

Now it is time to assemble the electronic circuit, which you can find below:

![alt text](resources/DS18B20-Data-Logger_bb.png?raw=true)

Before compiling the code, you need to download, uncompress, and install the following libraries: [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library), [OneWire](https://github.com/PaulStoffregen/OneWire), [RTClib](https://github.com/adafruit/RTClib), and [SD](https://github.com/adafruit/SD). Keep in mind that there is a standard SD library included with the Arduino IDE package, but you need to replace it with this one or the code will not compile (renaming the folder of the standard SD library might cause the same issue, so I recommend removing it). To install all these libraries, simply copy them into the "libraries" folder. Alternatively, you can refer to the official tutorial: [Installing Additional Arduino Libraries](http://arduino.cc/en/Guide/Libraries).


Upload the [code](DS18B20_Data_Logger.ino) to the board.

Serial Monitor:

![alt text](resources/SerialMonitor.jpg?raw=true)