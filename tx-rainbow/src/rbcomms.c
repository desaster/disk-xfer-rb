/**
 * tx-rainbow - disk-send
 *
 * DEC Rainbow Serial Comms (AUX) routines
 *
 * Instead of calling Int 14h, on DEC Rainbow we have to
 * call function 44h of Int 21h.
 *
 * Information based on:
 * QV068-GZ_Rainbow_MS-DOS_V2.05_Technical_Documentation_Nov84.pdf
 * Chapter 4-1 (PDF page 85)
 *
 * Thomas Cherryhomes <thom.cherryhomes@gmail.com>
 *
 * Modified for DEC Rainbow by Upi Tamminen <desaster@gmail.com>
 *
 * Licensed under GPL Version 3.0
 */

#include <i86.h>
#include <stdio.h>
#include <stdint.h>
#include "rbcomms.h"

static union REGS regs;

ioctl_t ioctl;

/**
 * Initialize port
 */
unsigned char rbcomms_init(void)
{
  // Nothing to do here.
  // User may initialize port settings using SETPORT in DOS.
  return 1;
}

/**
 * Send byte
 */
void rbcomms_send_byte(unsigned char b)
{
  ioctl.func = 11; // Func 11 - Put character, return when successful
  ioctl.character = b;
  regs.h.ah = 0x44;
  regs.h.al = 0x03;
  regs.x.bx = 0x03;
  regs.x.dx = FP_OFF(&ioctl);
  int86(0x21,&regs,&regs);
}

/**
 * Get Port Status
 */
short rbcomms_get_status(void)
{
  ioctl.func = 6; // Func 6 - Read input device status
  regs.h.ah = 0x44;
  regs.h.al = 0x03;
  regs.x.bx = 0x03;
  regs.x.dx = FP_OFF(&ioctl);
  int86(0x21,&regs,&regs);
  return ioctl.ret;
}

/**
 * Is data waiting?
 * Return 0 if nothing, 1 if data waiting.
 */
unsigned short rbcomms_data_waiting(void)
{
  return (rbcomms_get_status() == 0xFF); // FF = character available
}

/**
 * Read byte
 */
unsigned char rbcomms_read_byte(void)
{
  ioctl.func = 7; // Func 7 - Read input character, return if none available
  regs.h.ah = 0x44;
  regs.h.al = 0x03;
  regs.x.bx = 0x03;
  regs.x.dx = FP_OFF(&ioctl);
  int86(0x21,&regs,&regs);

  // There should be a character, if the user has checked the status before.
  // But if we still get "no character" here, just return a 0x00.
  if (ioctl.ret != 0xFF) {
    return 0;
  }

  // Just dump any unexpected situations on screen
  if (ioctl.char_stat & 0x80) {
    printf("\r\nBREAK\r\n");
  } else if (ioctl.char_stat & 0x40) {
    printf("\r\nFraming Error\r\n");
  } else if (ioctl.char_stat & 0x20) {
    printf("\r\nOverrun Error\r\n");
  } else if (ioctl.char_stat & 0x10) {
    printf("\r\nParity Error\r\n");
  }

  return ioctl.character;
}
