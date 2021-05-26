# STM32 DS2480 emulation project.
DS2480 is a 1-Wire line driver IC that works with OneWireLinkLayer library and several evaluation kit softwares from Maxim. In this project i have demostrated how to emulate DS2480 chip on STM32. You can use this usb stick to access your 1-Wire IC's with above softwares or with your own software using the OneWireLinkLayer library.

# How to upload firmware
1) Download and install the DfuSe application from ST.
2) Solder "DFU" jumper and plug it into USB port.
3) Install driver if needed.
4) Run DfuSe and press "choose" button under "Update and verify action" section.
5) Locate to *.dfu file (software/stm32_ds2480_emu.dfu)
5) Press "Upgrade" button
6) Press "Leave DFU mode" button.
8) Remove solder from "DFU" jumper

# Licenses
- Software license: MIT
- Hardware license: CERN v1.2

[![OSHW-TR000001](oshwa_tr000001.svg)](https://certification.oshwa.org/tr000001.html)

