# Parts



## General

Note: most sparkfun parts are also available via DigiKey (search for sparkfun product number) in case that's a preferred vendor.

 - super glue (e.g. Amazon [B003Y49R7G](https://www.amazon.com/gp/product/B003Y49R7G))
 - straight female single row PCB headers with 2.54 mm spacing (e.g. Amazon [B09BFN1JBX](https://www.amazon.com/Mardatt-Straight-Headers-Assortment-Connector/dp/B09BFN1JBX))
 - breakaway straight male single row PCB header pins with 2.54 mm spacing (e.g. Amazon [B07PKKY8BX](https://www.amazon.com/MCIGICM-Header-2-45mm-Arduino-Connector/dp/B07PKKY8BX))
 - breakaway right angle male single row PCB header pins with 2.54 mm spacing (e.g. Amazon [B01461DQ6S](https://www.amazon.com/Uxcell-a15062500ux0349-Single-40-pin-Breadboard/dp/B01461DQ6S))

## PCB

 - 1x 1N4001 rectifier diode soldered on top of PCB (e.g. Amazon [B07Q3HBM63](https://www.amazon.com/gp/product/B07Q3HBM63)). **The direction matters, watch the stripe!**
 - 1x 1/4W 10K Ohm resistor soldered on top of PCB named `HV` (e.g. Amazon [B07HDGX5LM](https://www.amazon.com/EDGELEC-Resistor-Tolerance-Resistance-Optional/dp/B07HDGX5LM))
 - 1x 1/4W 1.5K Ohm resistor soldered on top of PCB named `GND` (e.g. Amazon [B07HDGVJTH](https://www.amazon.com/EDGELEC-Resistor-Tolerance-Resistance-Optional/dp/B07HDGVJTH))
 - 2x 4 pin **male** headers soldered on top of PCB for serial and LCD connection (alternatively Qwiic connector for LC but that's less sturdy)
 - 1x 16 pin and 1x 12 pin **female** headers soldered on top of PCB for microcontroller (buy either correct lengths or break into correct length with pliars)
 - 2x 4 pin **female** headers soldered on top of PCB for I2C data logger and (optional) I2C temp / humidity sensor (alternatively Qwiic connectors but those are less sturdy)
 - 1x 2 position molex connector (Molex #1722861102, Digikey [WM11570-ND](https://www.digikey.com/en/products/detail/molex/1722861102/5344267)) soldered on **bottom** of PCB (power, not MOSFET!)
 - 2x 6 position molex connector (Molex #1722861306, Digikey [WM11677-ND](https://www.digikey.com/en/products/detail/molex/1722861306/5360142)) soldered on **bottom** of PCB
 - 1x DC DC converter (step down from 8-28V to 5V) soldered on after all headers are complete (Recom R-78E5.0-1.0 available at Digikey [945-2201-ND](https://www.digikey.com/en/products/detail/recom-power/R-78E5-0-1-0/4930585)) soldered on top of PCB

## Relays

 - 2x opto-isolated 4-channel relays with 10A current rating and 2.5-5V logic control (e.g. Amazon [B08PP8HXVD](https://www.amazon.com/gp/product/B08PP8HXVD))
 - 1x 6 wire 60cm molex cable (Molex #2174651063, Digikey [900-2174651063-ND](https://www.digikey.com/en/products/detail/molex/2174651063/13966724)). Cut cable in half, strip ends of wires and attach securely to screw terminals on the 2 relay boards, **make sure to get the order right: with the connector tab facing up, the far left wire should connect to VCC on the relay, 2nd wire to GND on the relay, 3rd through 6th wire to relay controls 1-4**. The relay linked above has the screw terminal in the correct order (DC+, DC-, IN1-4 from left to right) but this may not be true for all relay boards. Note that you can also buy shorter cables but using a long one cut in half is most cost effective (alternatively buy just the molex terminal and make your own cable from scratch).


## Power
 
  - 1x 2 wire 30cm molex cable (Molex #2174651022, Digikey [900-2174651022-ND](https://www.digikey.com/en/products/detail/molex/2174651022/13966733)). Cut one end off and connect to your power supply (e.g. car battery or wall wart, idealy 12V but can be 8-28V).

## Controller Box
 
### top

 - 1x 20x4 Sparkfun serial LCD screen ([LCD-16398](https://www.sparkfun.com/products/16398))
 - 4x 12mm long M2.5 flat head machine screws to attach LCD screen (e.g. McMaster [92010A022](https://www.mcmaster.com/92010A022/))
 - 4x M2.5 hex nuts to attach LCD screen (e.g. McMaster-Carr [91828A113](https://www.mcmaster.com/91828A113/))

### base

 - 4x 10mm long M3 nylon or aluminum hex standoffs to attach PCB board (e.g. Amazon [B016ENW3YC](https://www.amazon.com/Uxcell-a15062200ux0544-Spacer-Standoff-Pillar/dp/B016ENW3YC)) OR aluminum version for higher T applications (e.g. McMaster[95947A036](https://www.mcmaster.com/95947A036))
 - 4x 6mm long M3 rounded head screws to attach PCB board to standoffs (e.g. McMaster-Carr [92000A116](https://www.mcmaster.com/92000A116/))
 - 4x 8mm long M3 flat head screws to attach PCB standoffs to back panel (e.g. McMaster-Carr [92010A118](https://www.mcmaster.com/92010A118/))
 - 1x serial RS232 to TTL converter with male (i.e. pins) DB9 port ([Nulsom-RS232](http://www.nulsom.com/datasheet/NS-RS232_en.pdf), available e.g. from Amazon [B00OPU2QJ](https://www.amazon.com/NulSom-Inc-Ultra-Compact-Converter/dp/B00OPU2QJ4)). Female DB9 port works as well but would need a matching DB9 cable. The important pins on the DB9 connector are 2 (RX), 3 (TX) and 5 (GND). **Important: solder on right angle headers and install port oriented such that VCC is on the left and GND is on the right side (looking from the back), i.e. lined up with the serial pins on the PCB (to avoid cable twisting)**
 - 2x male-female threaded jack screws to attach DB9 port (McMaster [92710A220](https://www.mcmaster.com/92710A220/))
 - 2x 4-40 hex nuts to tighten jack screws (McMaster [91841A005](https://www.mcmaster.com/91841A005/))
 - OPTIONAL (for easier USB connection to microcontroller): panel mount Micro USB extension cable (Sparkfun [CAB-15464](https://www.sparkfun.com/products/15464))

 ## Assembly

  - 2x 10cm 4 pin jumper wire to attache LCD and serial to PCB (Sparkfun [PRT-10364](https://www.sparkfun.com/products/10364))
  - 4x 8mm long M3 flat head machine screws to attach top to base (e.g. McMaster-Carr [92010A118](https://www.mcmaster.com/92010A118/))
  - 4x M3 hex nuts to attach top to base (2.4mm thick, 5.5mm across, e.g. McMaster-Carr [91828A211](https://www.mcmaster.com/91828A211/)), **Glue these into the slots with a drop of super glue.**
  - 1x I2C data logger (Sparkfun [DEV-15164]https://www.sparkfun.com/products/15164)) with micro SD card (e.g. Sparkfun [COM-14832](https://www.sparkfun.com/products/14832) but doesn't have to be this big, any card will do). **Note: can connect this with Qwiic cable but it attaches to the PCB more sturdily with male right angle headers soldered on (recommended)**
  - OPTIONAL (for a few hours of backup power, highly recommended when field deployed): 2Ah lithium ion battery (Sparkfun [PRT-13855](https://www.sparkfun.com/products/13855)). This is the biggest that fits in the battery holder in the controller box but smaller down to 400mAh works too (shorter backup power).
  
