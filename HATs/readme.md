**HATs for the EsPiFF**

****Digital IO HAT****
![DIO](https://user-images.githubusercontent.com/96583658/206682544-97bb26f2-ebea-4f25-bf19-493255393e44.jpg)

The Digital IO HAT offers 8 individual configurable digital inputs or outputs. Each of the 8 DIO pins can be configured by software to be ether an digital output with up to 1.2A, or an digital input for voltages from 3.3V to 30 Volt. All inputs and outputs are protected against over-voltage, over-current and over-temperature, additional protection for voltage spikes(up to 4kV) and static discharge is build in as well. 

Inputs can be read with up to 1 MSps, the outputs can be set with up to 6kHz. The digital IO part is complete galvanic isolated from the EsPiFF, with 6mm creepage distance between the logic part and IO part. 

With an external 24V power supply, the digital HAT powers it self and the EsPiFF(or Pi Nano). If no external power is supplied, the digital HAT receive its power from the EsPiFF(or Pi Nano).

The Digital IO HAT is made particular for the EsPiFF, because of its shape it can not be used with an Raspberry Pi 2/3/4. But it can be used with a Raspberry Pi Nano or Nano 2, and several Raspberry Pi clones. 

The 40 pin connector is build with extra long pins, so that an additional HAT can be stacked on it

****Analog IO HAT****
![HAT_4ain_4aout](https://user-images.githubusercontent.com/96583658/206686323-8405f425-5337-4828-b41e-40408540d6e7.jpg)

The analog IO HAT offers 4 analog inputs and 4 analog outputs. Each input and output can be jumper configured for ether 4-20mA or 0-10V signals. The analog inputs are synchronous sampled by 16 bit ADCs, the analog outputs are synchronous set by 12 bit DACs. 

The ADCs are build by analog-to-PWM converters, to allow syncron sampling and save galvanic isolation. For the same reason, the DACs are build by PWM-to-analog converters. This offers the unique feature, that all analog inputs and outputs can be synchronous set with ns accuracy, what typical SPI or I2C based  converters can never offer. To read and set the converters, 4 PWM cannals, and 4 Pulse width measurement units are needed. This is a perfect job for the PIOs of the EsPiFF. A Raspberry Pi offer only 2 (hardware) PWM channels, so a PI would not be a good platform for this kind of converters.

Extra care has been taken to implement a low noise power supply: a galvanic isolated PSU module generates 15 Volt. A team of ferrite beads, common mode chokes and ultra low noise linear regulators filter out all noise from the switching regulators and outside world. Only this very careful design enables 15 bit net out of the 16 bit ADCs.  

****EsPiFF-PLC build from a EsPiFF, an digital and an analog HAT****
![HAT_espiff_stack](https://user-images.githubusercontent.com/96583658/206692327-288ef2ea-761b-4ec7-8b8e-1435d0be7dfb.jpg)

With the analog and digital HAT, an EsPiFF transforms to a nice PLC. It is compact in size, low in price and profit from the EsPiFF reliability features.
