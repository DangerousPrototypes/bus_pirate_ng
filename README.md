# bus_pirate_ng
Bus Pirate NG firmware and hardware

Check the blog at http://dangerousprototypes.com/blog/2018/09/13/prototype-bus-pirate-next-gen-v1/

### Building
- Install toolchain:
```
sudo apt-get update -y
sudo apt-get install -y gcc-arm-none-eabi
```
- Clone repository:
```
git clone --recursive https://github.com/DangerousPrototypes/bus_pirate_ng
cd bus_pirate_ng
```
- Compile modules in this order:
```
make -C libopencm3 lib/stm32/f1
make -C bootloader images
```
- Compile Bus Pirate:
```
make -C source images
```
- Output images:
```
bootloader/usbdfu.bin
source/buspirateNG.bin
```

### Bootloader
The firmware is compiled expecting a bootloader offset of 0x2000.<br>
The generated BusPirate image doesn't contain the bootloader, if your stm32 doesn't have it you must first flash:
```
bootloader/usbdfu.bin
```
Then enter DFU mode and flash:
```
source/buspirateNG.bin
```

### Bootloader-less
You can skip the bootloader entirely by modifying [`source/buspirateNG.ld`](source/buspirateNG.ld),<br>
Replace:
```
ORIGIN = 0x08002000, LENGTH = 120K
```
With:
```
ORIGIN = 0x08000000, LENGTH = 128K
```
Now simply flash ```source/buspirateNG.bin``` into the stm32 using your favourite programmer.
