**The EsPiFF-PLC**

Thanks to its reliability properties and its fast IO capacity, the EsPiFF is a great base for rock solid PLC (Pogrammable Logic Controller). We have made 2 HATs for the EsPiFF, what can be stacked on the EsPiFF, and an 3D printed enclosure, to use the EsPiFF as a PLC. Schematics for the digital HAT is here, for the analog HAT is here (Schematics are in alpha stage, because we could not yet test all functions).

To compare the EsPiFF-PLC with what the market offer, lets compare the EsPiFF-PLC with 2 popular PLCs, one from the Arduino base ([Industruino](https://industruino.com/)) and an other from Raspberry Pi base ([Pixtend](https://www.pixtend.de/pixtend-v2/hardware-v2) V2 -S-). We have worked with both of them, before we decided to make the EsPiFF. Finally, I added the brand new Arduino Opta for comparison.

We do not cover PLCs without C/C++ programming, because Ladder Logic/FBD/ST from companies like Siemens, ABB, Rockwell, Mitsubishi etc. is out of the scope for most makers. Also most of them require an initial investment of several thousand $$ for there proprietary IDE / tools, before you can even start. 

## Comparisons

|                            | EsPiFF-PLC          | Industruino                 | Pixtend-V2 -S-     | Arduino Opta      |
| ---                        | ---                 | ---                         | ---                | ---               |
| **PLC processor** ~active  | RP2040              | ATSAMD21G                   | ATMega324A         | STM32H747XI       | 
| **Digital inputs**         | Up to 8             | Up to 8                     | 8                  | 8                 |
| **Digital outputs**        | Up to 8             | Up to 8                     | 4                  | 0                 |
| **Digital block isolated** | Yes                 | Yes                         | No                 | No                |
| **Analog inputs**          | 4 (16 bit)          | 4 (18 bit)                  | 2 (10 bit)         | up to 8           |
| **Analog outputs**         | 4 (12 bit)          | 2 (12 bit)                  | 2 (V only)         | None              |
| **Analog block isolated**  | Yes                 | Yes                         | No                 | No                |
| **Relay outputs**          | 2                   | 0                           | 4                  | 4                 |
| **Retain memory**          | 2 kBytes            | 0                           | 32 Bytes           | 0                 |
| **External RTC**           | Yes                 | Yes                         | Yes                | unknown           |
| **External supervisior**   | Yes                 | No                          | No                 | unknown           |
| **Buffered RTC**           | Yes                 | No                          | Yes                | unknown           |
| **PLC processing**         | 2x133 MHz           | 48 MHz                      | 20 MHz             | 240 MHz           |
| **Ethernet**               | Yes                 | External (via SPI)          | Yes                | Yes               |
| **Price**                  | $169.00             | $272.72                     | $279.21            | $177.10           |

Its clear, that both competitors have serious limitations: The Pixtend have no galvanic isolation, only 4 digital outputs, only 10 bit analog inputs and  analog output. So its impossible to drive 4 - 20mA actuators like proportional valves. The analog resolution with just 10 bit ADCs does not allow accurate measurements, what limit its usefulness to a few low resolution applications. 
While the Pixtend offers enoumous processing power on the Raspberry Pi side, the real time processing power is very limited by the 8 bit ATMega324. As a result, no more then 500 to 800 cycles per second are possible (In a PLC, in one cycle all inputs are read and all output are set). The analog inputs have a "Cut-off frequency of 275 Hz", so my guess would be the analog inputs are limited to 100-150Hz.  A demanding servo application can not be build with this. The Retain memory memory is only guarantied for 10.000 write operation, the datasheet discourage the use of it (the FRAM on the EsPiFF guaranties 10 billion write cycles!). Finally the Pixtend is closed source, and Pis are not available because of the chip shortage. 

The Industruino is simple, easy to program, have 4 analog input with 18 bit, 2 analog output (12 bit), 8 digital IOs (each can be input or output), external RTC. Sound good until now. But: 
- No battery or capacitor to keep the RTC when power is removed,  
- no external supervision, 
- you need to buy the external Ethernet module, to get Ethernet, a socket for an FRAM, SD card socket. 
- only for a few parts schematics are open source,  
- a single  ATSAMD21G with 48MHz, 32 kByte RAM, 256kByte Flash must do all the work.
- quit expensive for the little functionality
- can NOT be programmed in Python

What the Pixtend had too much on processing power, the Industruino have too little. When you run a Modbus server, the MCU is mostly busy with network traffic. 

When we see the 500 to 800 cycles on the Pixtend as slow, it gets even worse with the  Industruino: the ADC on the Industruino is a MCP3424 I2C, what offer 3.75 SPS (18 bits), 15 SPS (16 bits), 60 SPS (14 bits) and 240 SPS (12 bits). SPS means "Samples Per Second". There will be applications, what are possible within these limits. But hey, that can be done better.

Just for reference, lets also take a look at the brand new Arduino Opta. It does not have analog outputs, not even digital outputs, so its usefulness is very limited. The concept of the Opta with a dual core controller is good, to seperate IO processing from network processing. Having relays integrated into the PLC is not often used, for a good reason: Users can simply install as many relays they need on a Din Rail, and choose the exact relay type they need for the particular application. Then control the relay with a digital output of the relay.

![DinRailRelay](https://s.alicdn.com/@sc04/kf/H43ffb4d0b59040bb9298856400a735a2L.jpg_960x960.jpg)


The relays in the Opta are only SPST (single pole, single throw, 2 contacts each), but many applications need a 3 contact version to flip between NC and NO. If you look at the [DIN Rail relay selection on Mouser](https://www.mouser.de/c/electromechanical/relays-contactors-solenoids/general-purpose-relays/?q=relay&mounting%20style=DIN%20Rail~~DIN%20Rail%20Mount&rp=electromechanical%2Frelays-contactors-solenoids%2Fgeneral-purpose-relays%7C~Mounting%20Style&sort=pricing), you see how many configurations are possible. Interesting to see, that Arduino move to closed source and pay-to-use software with there PLC products: "runtime activation for each device will require a license key". Bye bye open source Arduinos.  


We have 2 HATs for the EsPiFF, one analog and one digital HAT. Together with the EsPiFF, they form the ***EsPiFF-PLC***. At the moment, the hardware just arrived, the software need some time to arrive at the EsPiFF github. 

****EsPiFF-PLC****
![HAT_espiff_stack](https://user-images.githubusercontent.com/96583658/206708590-c72b0245-73a7-4690-abc7-aca0d1045446.jpg)

****digital IO HAT****
![HAT_dio](https://raw.githubusercontent.com/MDCservice/EsPiFF/main/images/DIO.jpg)


****analog IO HAT****
![HAT_analog](https://raw.githubusercontent.com/MDCservice/EsPiFF/main/images/HAT_4ain_4aout.jpg)

These 2 HATs are particular made for the EsPiFF: Even they could mechanically be mounted on a Raspberry Pi Nano, only the RP2040 PIOs have enough PWM channels and pulse-width measurement resources to operate the analog HAT. 

The HATs are stacked on the 40 pin Header, and are all controlled by the RP2040 from the EsPiFF. There is a detailed description about the HATs [on the EsPiFF github](https://github.com/MDCservice/EsPiFF/blob/main/HATs/readme.md), so I don't repeat that here.  Just the key facts:

4 analog in, jumper select-able for 0 to 10V or 4 - 20mA, fully protected.
4 analog out,  jumper select-able for 0 to 10V or 4 - 20mA, over-current, over-temperature, over-voltage protected.
All analog parts are galvanic isolated from the EsPiFF and the digital section.
8 digital IOs, each individual configurable as input or output, fully protected. As output, they can sink up to 1.2A permanent each, up to 32 Volt.  
All digital IOs are galvanic isolated from EsPiFF and from the analog section.  

The RP2040 as PLC-IO processor have multiple times the processing power, RAM and Flash, compared to the ATSAMD21G on the Industruino or the ATMega324 on the Pixtend. 16 MB Flash compared to 256kB(Industruino) or 32kB(Pixtend) result in 64/512 times bigger application. If the 16 MB additional SPI-RAM is soldered on the EsPiFF, the RP2040 on the EsPiFF have 512 times the RAM of an Industruino and 8192 times the RAM of the ATMega324 on the Pixtend.

Having the network stuff handled by its own processor unit, frees the PLC processor from these time consuming work. This is given on the EsPiFF-PLC by the ESP32, as well as on the Pixtend. But just the EsPiFF is build for reliability with external supervisior, external watchdog, and a real RTOS. 

Speed: we see, even with its enormous processing power of a Rasperry Pi, the Pixtend can maximal reach a cycle time of up to 800 per second. The Industruino is limited by its ADC to 240 cycles per second at low resolution ADC or 3.75 cycles per second at high resolution ADC. Here are the EsPiFF numbers:

- Digital inputs: up to 50.000  cycles per second.
- Digital outputs: up to 6.000 cycles per second.
- Analog inputs: up to 10.000 cycles per second. 
- Analog outputs: up to 50.000 cycles per second.

For the Pixtend, we know the limit of 800 cycles ([the Handbook suggest only 10Hz!](https://www.pixtend.de/files/manuals/AppNote_CodesysPiXtendProject_DE.pdf)), for the Industruino we [know the limit of 1380Hz](https://industruino.com/forum/help-1/question/speed-problem-on-i-o-board-100). Just in real world applications, the Industruino will not reach these limits, because as long as its answering network packages, the single core can not operate IOs at the same time. The unpredictable network processing load will also lead to unpredictable real time behavior. For the Arduino Octa, no data or experience are available.  

Finally, lets look at a typical PLC application: reading an 4-20mA transducer, performing a PID control algorithm, and set a new analog value on the 4-20mA output. I used to implement this exact scenario for a hydraulic pressure test bench a few years ago. An hydraulic pressure transducer transmitted a 4-20mA signal for the hydraulic pressure on the hose under test to the analog input, the pressure actor was commanded with an 4-20mA analog output and a PID control algorithm set the current test pressure. The Arduino Octa have no analog or digital outputs, so its out of the game here. Lets see, how many such control loops the EsPiFF-PLC and the 2 competitors can handle per second:  

![Compare_loops_perSecond](https://raw.githubusercontent.com/MDCservice/EsPiFF/main/images/EsPiFF_PLC_compare2.png)

The EsPiFF-PLC is more then 14 times faster in analog processing then the Industruino; the Pixtend is out of this game because it does not offer an analog 4-20mA output. 
In a pure digital loop (read digital in, compute, set digital out), the EsPiFF is still more then 4 times faster then the Industruino and more then 7 times faster then the Pixtend. 


We hope, this article shows, how powerful the EsPiFF is.
   

    

 
