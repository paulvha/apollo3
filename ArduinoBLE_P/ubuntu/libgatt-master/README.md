Orignal Source code is taken from Gattlib https://github.com/labapart/gattlib. It had too many issues and bugs. I have tried to resolve them and also added missing functionlaities like disconnection handler, pairing/unpairing info, RSSI info, removing device, getting list of paired/connected devices etc. 

libgatt is a library used to access Generic Attribute Profile (GATT) protocol of BLE (Bluetooth Low Energy) devices.
It has been introduced to allow to build applications that could easily communicate with BLE devices.

It supports Bluez v4 and v5. On Bluez versions prior to v5.42, gattlib used Bluez source code while it uses D-Bus API 
from v5.42. D-Bus API can be used on version prior to Bluez v5.42 by using the CMake flag `-DGATTLIB_FORCE_DBUS=TRUE`:

```
mkdir build && cd build
cmake -DGATTLIB_FORCE_DBUS=TRUE ..
make
```
Build libgatt
=============

* libgatt requires the following packages: `libbluetooth-dev`, `libreadline-dev`.  
On Debian based system (such as Ubuntu), you can installed these packages with the
following command: `sudo apt-install libbluetooth-dev libreadline-dev`

```
cd <libgatt-src-root>
mkdir build && cd build
cmake ..
make
```
### Cross-Compilation

To cross-compile libgatt, you must provide the following environment variables:

- `CROSS_COMPILE`: prefix of your cross-compile toolchain
- `SYSROOT`: an existing system root that contains the libraries and include files required by your application

Example:

```
cd <gattlib-src-root>
mkdir build && cd build
export CROSS_COMPILE=~/Toolchains/gcc-linaro-4.9-2015.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
export SYSROOT=~/Distributions/debian-wheezy
cmake ..
make
```

Package libgatt
===============

From the build directory: `cpack ..`

**Note:** It generates DEB, RPM and ZIP packages. Ensure you have the expected dependencies
 installed on your system (eg: to generate RPM package on Debian-based Linux distribution
  you must have `rpm` package installed).

Default install directory is defined as /usr by CPack variable `CPACK_PACKAGE_INSTALL_DIRECTORY`.  
To change the install directory to `/usr/local` run: `cpack -DCPACK_PACKAGE_INSTALL_DIRECTORY=/usr/local ..`

TODO List
=========

- Alot of cleanup is required i.e. commented out code, extra spaces and unnecessary functions.
- adjust return values of all functions 
- Undo changes made to examples for testing
- change compile options to use name libgatt

