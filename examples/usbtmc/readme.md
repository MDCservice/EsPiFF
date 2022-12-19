Convert every HAT to an USBTMC-Measurement device with the EsPiFF. 

** work in progress, might not compile on all platforms. We use Ubuntu 22 for development **


Instructions:
1. Download the Tinyusb github repo from https://github.com/hathach/tinyusb
2. Add a folder "espiff_usbtmc" under ./tinyusb/examples/device/
3. copy the files from EsPiFF/examples/usbtmc/ into the folder just created.
4. make sure, your PICO SDK is setup correctly.
5. follow the instructions from the TinyUSB repo to build.
