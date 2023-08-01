/**
 * tx - disk-send
 *
 * DEC Rainbow low-level disk access routines
 *
 * Instead of calling Int 13h, on DEC Rainbow we have to
 * call function 44h of Int 21h.  Except that for some reason this doesn't
 * seem to work, and we'll have to call 44h on the drivers manually.
 *
 * Disk Control functions are described in:
 * QV068-GZ_Rainbow_MS-DOS_V2.05_Technical_Documentation_Nov84.pdf
 * Page 4-24 (PDF page 108)
 *
 * Location of disk drivers is described in WUTIL:
 * https://github.com/bsdimp/rainbow100/blob/master/wutil/WUSUBH.PAS#L151
 *
 * The general DOS device driver structure is fairly normal:
 * http://www.techhelpmanual.com/322-device_header_layout.html
 *
 * Although finding the disk driver on Rainbow ends up being quite a hack.
 *
 * Upi Tamminen <desaster@gmail.com>
 *
 * Licensed under GPL Version 3.0
 */

#include <i86.h>
#include <stdio.h>
#include "rbdisk.h"

union REGS regs;
struct SREGS sregs;

__segment biosseg;

DiskGeometry geometry;

char dos_version;

void (__far * hdinterrupt)(void);
void (__far * hdstrategy)(void);

/**
 * Check DOS version and figure out the BIOS segment
 */
void check_dos_version(void)
{
  char major, minor, extra;

  regs.h.ah = 0x30;
  int86(0x21, &regs, &regs);

  major = regs.h.al;
  minor = regs.h.ah;
  extra = regs.h.cl;
  dos_version = major;

  printf("DOS Version: %d.%d%c\r\n", major, minor, extra);

  biosseg = 0x0040;
  if (major > 0x03) {
    biosseg = 0x0070;
  } else if (major == 0x03 && (minor > 0x0a || (minor == 0x0a && extra > 0x61))) {
    biosseg = 0x0070;
  }
}

/**
 * For debugging, print out the driver entry
 */
void print_driver_details(ddentry_t driver)
{
  printf("Next=%04X:%04X Stg=%04X Int=%04X Flg=%04X NU=%d [%.8s]\r\n",
      driver.next_segment,
      driver.next_offset,
      driver.stg_offset,
      driver.int_offset,
      driver.flags,
      (driver.flags & 0x8000) ? 0 : driver.devname[0],
      (driver.flags & 0x8000) ? driver.devname : "        ");
}

/**
 * Pick the driver from specified memory location
 */
ddentry_t get_driver(uint16_t segment, uint16_t offset)
{
  return *((ddentry_t far *) (((long) segment << 16) | (offset)));
}

/**
 * Set the specified driver as the hard disk driver
 */
int set_hd_driver(ddentry_t driver)
{
  // Check that it doesn't look like something else
  if ((driver.flags & 0xe00f) != 0x6000 || driver.devname[0] > 2) {
    printf("Found something that's not a hard disk driver\r\n");
    return 0;
  }

  // Set far pointers to be called
  hdstrategy = (void __far *) (((long) biosseg << 16) | (driver.stg_offset));
  hdinterrupt = (void __far *) (((long) biosseg << 16) | (driver.int_offset));

  // Print out for debugging, address can be inspected with DEBUG.COM
  printf("HD Driver Strategy: [%04X:%04X] Interrupt: [%04X:%04X]\r\n",
    FP_SEG(hdstrategy),
    FP_OFF(hdstrategy),
    FP_SEG(hdinterrupt),
    FP_OFF(hdinterrupt));

  return 1;
}

/**
 * Try to locate the hard disk driver
 */
int find_hd_driver(void)
{
  uint16_t segment;
  uint16_t offset;
  ddentry_t driver;
  ddentry_t prevdriver;

  // Initially set these to 0xFFFF so we know if we found anything
  hdstrategy = (void __far *) 0xFFFFFFFF;
  hdinterrupt = (void __far *) 0xFFFFFFFF;

  // http://www.techhelpmanual.com/514-dos_fn_52h__get_dos_variables.html
  regs.x.ax = 0x5200;
  int86x(0x21, &regs, &regs, &sregs);
  segment = sregs.es;

  // NUL device (first driver in the chain) is found at an offset depending
  // on DOS version
  offset = (regs.x.bx + (dos_version < 3 ? 0x17 : 0x22)) ;

  // Pick the first driver
  driver = get_driver(segment, offset);
  printf("%04X:%04X ", segment, offset);
  print_driver_details(driver);

  // Keep going through drivers until we hit the end, or find the hard disk
  // driver
  while (driver.next_offset != 0xFFFF) {
    prevdriver = driver;
    printf("%Fp ", driver); // somehow this displays the address for the next
                            // driver instead of the current one,
                            // so let's do it before the next driver ¯\_(ツ)_/¯

    // PIck the next driver
    driver = get_driver(driver.next_segment, driver.next_offset);
    print_driver_details(driver);

    // Does it look like a Rainbow RX50 floppy drive driver with 4 units?
    if (driver.devname[0] == 4) {
      // then next one should be the hard disk driver
      if (driver.next_offset != 0xFFFF) {
        // Normally it's just there, and we can cleanly pick it
        printf("Encountered a floppy drive, assuming next one is hard drive\r\n");
        printf("%Fp ", driver);
        driver = get_driver(driver.next_segment, driver.next_offset);
      } else {
        // if the disk is not initialized, or has no MS-DOS partitions, we
        // need to manually pick it by offset of 18
        printf("At the end of chain [%04X], picking next driver by offset anyway\r\n", driver.next_offset);
        printf("%Fp ", driver);
        driver = get_driver(prevdriver.next_segment, prevdriver.next_offset + 18);
      }

      print_driver_details(driver);

      // Try to set it as the hard disk driver
      if (!set_hd_driver(driver)) {
        return 0;
      }
      break;
    }
  }

  // We got this far, did we successfully find and set the hard disk driver?
  if (FP_OFF(hdstrategy) == 0xFFFF || FP_OFF(hdinterrupt) == 0xFFFF) {
    return 0;
  }

  return 1;
}

/**
 * Prepare for disk operations by finding the hard disk driver
 */
int rbdisk_init(void) {
  // Dos version is needed, since finding the device drivers are at a
  // different offset in version <3
  check_dos_version();

  // Find the hard disk driver in DOS, which will be called for the actual
  // low level reading of sectors
  if (!find_hd_driver()) {
    return 0;
  }
  return 1;
}

/**
 * Read sector given CHS
 */
int rbdisk_read_sector(short c, unsigned char h, unsigned char s, char* buf)
{
  driverreq_t driverreq;
  driverreq_t *driverreq_p = &driverreq;
  wctlbl_t wctlbl;

  // TODO: If I knew how to do the far calls here in assembly, the push/pop
  // could be handled by compiler
  extern void pre_driver_call();
  #pragma aux pre_driver_call = \
    "push ax" \
    "push es" \
    "push bx" \
    "mov ax, ds" \
    "mov es, ax" \
    "mov bx, driverreq_p" \
    ;

  extern void post_driver_call();
  #pragma aux post_driver_call = \
    "pop bx" \
    "pop es" \
    "pop ax" \
    ;

  // Prepare the Winchester Control Block
  wctlbl.func = 0x00;
  wctlbl.drive = 0xff; // physical unit
  wctlbl.sector = s; // starting from 1
  wctlbl.unit = h | 0x40;
  wctlbl.track = c;
  wctlbl.sectors = 0x0001;
  wctlbl.bufoffs = FP_OFF(buf);
  wctlbl.bufseg = FP_SEG(buf);
  wctlbl.status = 0xff;
  wctlbl.error = 0xff;

  // Prepare the driver request
  driverreq.len = 13;
  driverreq.unit = 5;
  driverreq.cmd = 0x03;
  driverreq.addroffs = FP_OFF(&wctlbl);
  driverreq.addrseg = FP_SEG(&wctlbl);

  pre_driver_call();
  hdstrategy();
  hdinterrupt();
  post_driver_call();

  if (wctlbl.status != 0x00) {
    printf("Error %02X %02X\r\n", wctlbl.status, wctlbl.status);
  }

  return wctlbl.status != 0x00;
}

/**
 * Get disk Geometry
 */
int rbdisk_geometry(DiskGeometry* geometry, char *buf)
{
  // We should have been given the HOM block (sector 2), which contains the
  // drive geometry. We can also double check here that it really looks like a
  // HOM block.
  hom_t *hom = (hom_t *) buf;
  if (hom->id[0] != 'H' || hom->id[1] != 'O' || hom->id[2] != 'M') {
    return 1;
  }

  geometry->c = hom->cylinders;
  geometry->h = hom->surfaces;
  geometry->s = hom->sectors_per_track;

  return 0;
}
