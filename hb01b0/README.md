############################################
### install updated Sparkfun EDGE driver ###

## Versioning
# version 1.1 / paulvha / March 2025
- update libmbed-os.a from library V 2.0.6 to V 2.2.1

The original libmbed-os.a is about 60MB, the one in this package is about 7.5MB as I have removed the debug info that is not used anyway.
I have removed the complete HB01BO driver from mbed-os pre-compiled library, so we can use (nearly) the standard library provided by Sparkfun.
The .includes-file has the MBED-PS HB01BO driver reference removed (2 lines)

Go to the Sparkfun Artemis/Apollo3 V2.2.1 boards library (referenced as  {Vlib} in the rest of this document)
cd {vlib}/variants/SFE_EDGE/mbed

rename libmbed-os.a to libmbed-os.org   (safety copy)
rename .includes to ./includes.org      (safety copy)

copy libmbed-os.a and .includes that are part of this package in this mbed folder.

#####################################
### install update HB01BO library ###

This library can be used on BOTH V1x and V2x Sparkfun Artemis boards library

In the sketch has a number of changes the enablement AM_BSP_GPIO_CAMERA_HM01B0_DVDDEN has been changed as D10 is NOT defined for EDGE.
OPTIONAL : You can enable that if you want so you can use the default pinMode (see below)

Two changes have been applied in the Sparkfun library that is part of this package, else it is the standard library.

In hm01b0.c, in routine hm01b0_blocking_read_oneframe() around the while loop I have added:
AM_CRITICAL_BEGIN and AM_CRITICAL_END

so it looks like
```
    AM_CRITICAL_BEGIN
    while((ui32HsyncCnt < HM01B0_PIXEL_Y_NUM))
    {

        while (0x00 == HM01B0_READ_HSYNC);

        // read one row
        while(HM01B0_READ_HSYNC)
        {
            while(0x00 == HM01B0_READ_PCLK);

            *(pui8Buffer + ui32Idx++) = HM01B0_READ_BYTE;

            if (ui32Idx == ui32BufferLen) {
                goto end;
            }

            while(HM01B0_READ_PCLK);
        }

        ui32HsyncCnt++;
    }
end:
    AM_CRITICAL_END
    return ui32Err;
```

In the file hm01b0_arduino.cpp, the routine begin(), I have added a software reset.

```
After :
  status = hm01b0_init_if(&cfg);
  if(status != HM01B0_ERR_OK){ goto fail; }

added :

  hm01b0_reset_sw(&cfg);
  delay(100)
```

#####################################
### optional : enable D10 on EDGE ###

If you want to use D10 on Edge (e.g. with pinMode) you need to make the following 3 changes :
```
In {vlib}/cores/mbed-os/targets/TARGET_Ambiq_Micro/TARGET_Apollo3/TARGET_SFE_EDGE/PinNames.h

after line 41 : D3 = 3,
add a line    : D10 = 10,

In {vlib}/variants/SFE_EDGE/config/pins.cpp

change :  const pin_size_t variantPinCount = 4;
to     :  const pin_size_t variantPinCount = 5;

After line :  {D3, 3, NULL, /*NULL, NULL, NULL,*/ NULL},
add a line :  {D10, 10, NULL, /*NULL, NULL, NULL,*/ NULL},  // <<< missing for camera
```

#####################################
