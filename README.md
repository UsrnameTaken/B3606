# B3606 based on B3603

This project is about reverse engineering the B3606 control board and figuring
out how it works, then it should be possible to create an alternative firmware.
Either by driving it with another board on the same control points or by
replacing the original firmware with one of my own.
The difference between B3603 and B3606 is only in converter part.

**Current state**: Testing //Working, it is functioning and serially controllable. Code size 7231 bytes (original is 7930).

Components needed:
* [B3606](https://www.banggood.com/B3606-NC-Digital-DC-DC-Step-Down-Buck-Module-Constant-Voltage-Current-p-947293.html?rmmds=search) -- The unit being reprogrammed
* [CP2102](http://www.banggood.com/Wholesale-USB-To-TTL-or-COM-Converter-Module-buildin-in-CP2102-New-p-27989.html?p=PA11121233669201502E) -- A usb-to-serial TTL-level
* [STLink V2](http://www.aliexpress.com/item/FREE-SHIPPING-ST-Link-V2-stlink-mini-STM8STM32-STLINK-simulator-download-programming-With-Cover/1766455290.html) or [STLink/V2 mini](https://www.waveshare.com/st-link-v2-mini-stm32.htm) -- programmer for the STM8S microcontroller

Software needed:
* [SDCC v 3.5.0](https://sourceforge.net/projects/sdcc/files/sdcc-linux-x86/3.5.0/) -- source code compiler
* [stm8flash](https://github.com/vdudouyt/stm8flash) -- STM8 flasher

## Schematics

Bottom board schematics:

![B3606 Bottom Board Schematics](docs/B3606.pdf)

Top board schematics:

![B3603 Top Board Schematics](docs/B3603_TopBoardSchematics.png)

## Regulator Board (bottom)

![Bottom Board Side 1](docs/BottomBoardSide1.jpg)

![Bottom Board Side 2](docs/BottomBoardSide2.jpg)

## Control Board (top)

![Top Board Side 1](docs/TopBoardSide1.jpg)

![Top Board Side 2](docs/TopBoardSide2.jpg)

## Differences between B3603 and B3606

Mainly the units are the same. Top board and bottom part under top board is the same (in my unit the exception is in R8, R19 and R26 values). There is a high chance, that the program will work on every simmilar board, but without guarantee.
Differences in values:

Resistor | B3603 value | B3606 value
--- | --- | ---
R8 | 1k | 1k5
R19 | 10k | 20k
R26 | 15k | 20k

Those differences probably make the board to not precise values of output voltage and current, but it's about 0,3 V (B3606 measure) or 0,6 V (cheap multimeter measure) at 12 V VIN. I'll look after this in version 1.0.3.

### MCU

The MCU is an [STM8S003F3](http://www.st.com/web/catalog/mmc/FM141/SC1244/SS1010/LN2/PF251792). It is the TSSOP-20 package.

### Compiling and flashing
To compile source code use command: make SHELL='sh -x'

To flash .ihx hex file into STM8 use command: ./stm8flash -c stlinkv2 -p stm8s003?3 -s flash -w b3606.ihx

### Pinouts

Lets name the different pinout components, left and right are as seen looking at the top board with the 7-segment display up:

* MCU
* Left connector -- 8 pins left side
* Right connector -- 8 pins right side
* Serial connector -- 4 pins at left most side
* SWIM connector -- 4 pins at the bottom, just left of the buttons
* 74HC595 #1 -- The one closest to the MCU
* 74HC595 #2 -- The one furthest from the MCU

#### Pinout from MCU

![STM8S003F3 TSSOP20 pins](docs/STM8S003F3 pinout.png)

| MCU pin | MCU Function | Board Connector | Board Connector Pin | Board Connector Name
| ------- | -------------|-----------------|---------------------|-----
| Pin 1 | UART1\_CK/TIM2\_CH1/BEEP/(HS) PD4 | 74HC595 | Pin 3 | DS
| Pin 2 | UART1\_TX | Serial connector | Pin 2 | TX
| Pin 3 | UART1\_RX | Serial connector | Pin 4 | RX
| Pin 4 | NRST | SWIM | Pin 1 | SWIM NRST
| Pin 5 | OSCIN/PA1 | 74HC595 | Pin 11 | SHCP
| Pin 6 | OSCOUT/PA2 | 74HC595 | Pin 12 | STCP
| Pin 7 | Vss (GND) | | |
| Pin 8 | Vcap | | |
| Pin 9 | Vdd | | |
| Pin 10 | SPI\_NSS / TIM2\_CH3 / PA3 (HS) | CV/CC leds |  | CV/CC leds
| Pin 11 | PB5 (T) / I2C\_SDA / TIM1\_BKIN | Left connector | Pin 7 | CV/CC status
| Pin 12 | PB4 (T) / I2C\_SCL / ADC\_ETR | Left connector | Pin 6 | Enable Output + Red (ON) led
| Pin 13 | PC3 (HS) / TIM1\_CH3 [TLI] [TIM1_CH1N]| Left Connector | Pin 8 | Not connected
| Pin 14 | PC4 (HS) / TIM1\_CH4 / CLK\_CCO / AIN2 / TIM1\_CH2N | Left connector | Pin 1 | Iout sense 16\*(0.01V + Iout\*0.05)
| Pin 15 | PC5 (HS) / SPI\_SCK / TIM2\_CH1 | Left connector | Pin 5 | Vout set
| Pin 16 | PC6 (HS) / SPI\_MOSI / TIM1\_CH1 | Left connector | Pin 4 | Iout set
| Pin 17 | PC7 (HS) / SPI\_MISO / TIM1\_CH2 | Button |  | Buttons
| Pin 18 | PD1 (HS) / SWIM | SWIM | Pin 3 | SWIM & Buttons
| Pin 19 | PD2 (HS) / AIN3 / TIM2\_CH3 | Left connector | Pin 2 | Vout sense
| Pin 20 | PD3 (HS) / AIN4 / TIM2\_CH2 / ADC\_ETR | Left connector | Pin 3 | Vin sense (Vin/16)


The buttons are connected in a strange setup where all four are on two pins. You can see oscillograms in [this directory](https://github.com/UsrnameTaken/B3606/tree/master/docs/b3606_uswitch).

The CV/CC leds are in serial with a lead between them throuh a 10K resistor to pin PA3, by changing the pin between Output HIGH, Output LOW and Input it is possible to make one of them on or both off.

#### Bottom Board Interface

The below was decoded by [bal00](http://www.reddit.com/r/arduino/comments/2so02f/can_anyone_recommend_a_cheap_cheerful_bench_power/cnrjdxo).

![Control pinouts](docs/control_pinouts.png)

Right side:

* Top four (1-4) pins are GND
* Next two (5-6) are Vcc +5V (seems wrong)
* 7 is connected to MCU UART RX
* 8 is connected to MCU UART TX

Left side (Top to bottom):

* Pin 1: Iout sense, 970mV/A + 140mV
* Pin 2: Vout sense, 72mV/V + 42mV
* Pin 3: Vin sense, 62mV/V
* Pin 4: Iout control, 970mV/A + 140mV (PWM controlled, off when output off)
* Pin 5: Vout control, 72mV/V + 42mV (PWM controlled, off when output off)
* Pin 6: Enable control, 0V = output on, 5V = output off (Digitally controlled)
* Pin 7: CC/CV sense, CV = 0.47V, CC = 2.5V
* Pin 8: Connected to MCU pin 13 (PC3), unknown function

#### Pinouts of 74HC595 chips

There are two 74HC595 TSSOP16, these control the 4 digit 7 segment display, and possibly the leds as well. The 7 segment display has 12 pins and is controlled constantly to create a persistence-of-vision effect.

![74HC595 pinout](docs/74HC595_TSSOP16.png)

## Links

* [Manufacturer product page](http://www.mhinstek.com/product/html/?106.html) (Chinese) ([English translation](https://translate.google.com/translate?sl=auto&tl=en&js=y&prev=_t&hl=en&ie=UTF-8&u=http%3A%2F%2Fwww.mhinstek.com%2Fproduct%2Fhtml%2F%3F106.html&edit-text=))
* [EEVBlog forum discussion](http://www.eevblog.com/forum/reviews/b3603-dcdc-buck-converter-mini-review-and-how-the-set-key-could-be-fatal/)
