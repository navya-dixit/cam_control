# cam_control
INTODUCTION
-----------

This project is about controlling camera using dip switch in imx6 SBC board.

The camer aused is ov7725 in imx6 SBC board. 
In dip switch driver, in ioctl, added the controls to get the status of all the 4 dip switches in the board. Added polling function to get the status change of dip switch whenever user changes the status of the switch.
test_app.c is the file which contains the application code.
Makefile is used to compile the application.
Before compiling cross compiler should  be set as
  export CROSS_COMPILE=/opt/poky/1.8/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/
  export SDKTARGETSYSROOT=/opt/poky/1.8/sysroots/cortexa9hf-vfp-neon-poky-linux-gnueabi/
Patch file for the dip switch driver should be applied after the basic customization patch.
