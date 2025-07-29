## Cura the Smart Trash Bin ^w^

![Cura Banner](Image/Cura.png)

**Cura** is a smart trash bin project powered by Arduino and an ultrasonic sensor, featuring a cute animated LCD display. It measures the fill level of your trash bin and displays real-time status and fun animations to make waste management more engaging. (｡･ω･｡)

---

### Features ✧˖°
- **Real-time Fill Level Monitoring:** Uses an ultrasonic sensor to measure how full your trash bin is.
- **Animated LCD Display:** Shows cute expressions and progress bars on a 16x2 I2C LCD.
- **Idle Mode:** Enters a screensaver-like mode with full-screen animations when the bin is not in use.
- **Status Alerts:** Displays messages when the bin is almost full or getting full.
- **Smooth Readings:** Uses averaging and smoothing to avoid flickering and false readings.
- **Fun Startup Animation:** Greets you with a special message and face on boot. UwU

---

### Hardware Requirements ⚙️
- Arduino Uno (or compatible)
- HC-SR04 Ultrasonic Distance Sensor
- 16x2 I2C LCD Display (address 0x27)
- Jumper wires
- Breadboard or soldering tools
- (Optional) Enclosure for the electronics

#### Pin Connections
- **Ultrasonic Sensor:**
  - Trig: Pin 10
  - Echo: Pin 11
- **LCD:**
  - I2C SDA/SCL to Arduino SDA/SCL

![Schematic Diagram](Image/sch.png)

---

### Required Libraries ⚡
- **LiquidCrystal_I2C:** [https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library](https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library)

---

### Setup & Usage OwO
1. **Wiring:**
   - Connect the ultrasonic sensor and LCD as described above.
2. **Upload Code:**
   - Open `cura.ino` in the Arduino IDE.
   - Install the `LiquidCrystal_I2C` library if not already installed.
   - Select your board and port, then upload the sketch.
3. **Power Up:**
   - Power the Arduino. The LCD will display a welcome message, then start monitoring the bin.
4. **Operation:**
   - The LCD shows the trash fill percentage and a progress bar.
   - When idle, Cura displays cute full-screen animations.
   - Alerts appear as the bin gets full.

---

### Calibration Tool =.=
The project includes a sensor calibration utility (`sensorcalibrator.ino`) to help you determine the correct empty distance value for your specific trash bin:

1. Upload `sensorcalibrator.ino` to your Arduino.
2. Position the sensor where it will be mounted in your final setup.
3. Note the distance (in cm) when the bin is empty.
4. Use this value as `EMPTY_DISTANCE` in `cura.ino`.

---

### Customization ^_^
- Adjust `EMPTY_DISTANCE` in the code to match your bin's empty distance (in cm).
- Tweak animation timings or add new emotes in `cura.ino` for more personality.

---

### License +_+
MIT License. See `LICENSE` file if present.

---

### Credits ^-^
Created by [mykeyy](https://github.com/mykeyy)
# Cura