# nordic_snesUSBD
Snes Classic Controller and 8Bit Doe to USBD-HID 

A simple weekend based project to allow for the use of either the wired or wireless snes controllers designed for use on the snes classic on any PC who supports USBD-HID. This project was created due to the several issues i had with the SNES classic, many key functionalities and features i expected were not present. Many games including those supported were often glitchy and sometimes unplayable. Therefore this project was created out a need of a need to convert the I2C wireless controllers from 8bit do into a "pc" compatible format. Since i wanted to avoid modifying any controllers and i wanted them to be functional/compatabile with my SNES Mini in the future this seemed like the best method.

Tools & Devices Needed: 
  -NRF52840 Dongle - With Segger Jlink Programmer and Cable 
  -2 SNES Classic Extension Cables 
  -1x 5v -> 3v3 Regulator
  
TODO:   - Add interrupt service routine for insertion detection, rather than a simple pin read. 
        - Add sleep mode and LED status.
        - Minor code cleanup perhaps place everything in a freeRTOS environment. 
