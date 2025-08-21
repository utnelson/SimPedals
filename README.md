#H1 DIY Sim Pedals

Adjust your sensor values based on your minimum and maximum values. The correct adjustment depends on the position of your hall mounts, springs, and TPU bushings.

```cpp
// 🔧 Kalibrierung
const int clutchMin = 579;
const int clutchMax = 693;

const int throttleMin = 619;
const int throttleMax = 770;

const long brakeMin = 90000;
const long brakeMax = 140000;
```
