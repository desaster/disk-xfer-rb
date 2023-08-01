# disk-xfer-rb

A simple pair of programs to image copy a DEC Rainbow 100A/100B hard drive to
an image file on a remote computer over serial port.

This software is based on [disk-xfer](https://github.com/tschak909/disk-xfer), a disk imaging tool for IBM PC
compatibles. Since the DEC Rainbow is quite incompatible with the IBM PC, this
version replaces the disk and communications routines with something that
works on the Rainbow. Otherwise the software should work identically.

## Usage

To use:

On the source machine, use SETPORT to change communications parameters to 19200,8,N,1 (no CTS or XON/XOFF).

Start TXRB:

```
A> TXRB
```

Start rx on destination machine:

```
$ rx /dev/ttyUSB0 diskimage.img
```

If everything is connected correctly, tx will send the data over the AUX
serial port to the destination machine in the requested image file.

## Notes

This has been tested on a real Rainbow 100A with a RD50 (ST-506) drive, as well as on
the Rainbow 100B emulation on MAME.

On my RD50 drive, the read seems to fail at cylinder 152, even though that cylinder
should be there based on the geometry. I don't know this is normal, but the
resulting image seems good anyway.
