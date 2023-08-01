/**
 * tx - disk-send
 * 
 * main routines
 *
 * Thomas Cherryhomes <thom.cherryhomes@gmail.com>
 *
 * Modified for DEC Rainbow by Upi Tamminen <desaster@gmail.com>
 *
 * Licensed under GPL Version 3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include "rbcomms.h"
#include "rbdisk.h"
#include "xmodem-send.h"

#define BUFFER_SIZE 512

void print_hom_info(char *buf) {
  hom_t *hom = (hom_t *) buf;

  printf("HOM ID: [%.3s] Volume: [%.8s]\r\n", hom->id, hom->volume_id);
  printf("Disk type: %d  Autoboot: %02X\r\n", hom->typecode, hom->autoboot);
  printf("C:%d H:%d S:%d\r\n", hom->cylinders, hom->surfaces, hom->sectors_per_track);
}

int main(int argc, char* argv[])
{
  char hombuf[512];

  rbcomms_init();
  printf("Serial port initialized.\r\n");

  if (!rbdisk_init()) {
    printf("Hard disk initialization failed!\r\n");
    return 1;
  }

  // Read the HOM sector. This is only available if the disk has been
  // previously initialized. If there's need to dump a disk without the HOM
  // block, this code needs to be changed.
  if (rbdisk_read_sector(0, 0, 2, hombuf) != 0) {
    printf("Error reading HOM block\r\n");
    return 2;
  }

  // Print out some details from the HOM block to reassure the user that the
  // disk is being read properly
  print_hom_info(hombuf);

  // Read the disk geometry from the HOM block. If there's need to dump a disk
  // without the HOM block, the code needs to be changed to read the geometry
  // from somewhere else.
  if (rbdisk_geometry(&geometry, hombuf) == 1) {
    printf("Could not retrieve disk geometry from HOM block. Aborting.\n");
    return 3;
  }

  xmodem_send();
    
  return 0;
}
