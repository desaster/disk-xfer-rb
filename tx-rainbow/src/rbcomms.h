#include <stdint.h>

#ifndef RBCOMMS_H
#define RBCOMMS_H

/* IOCTL packet described in:
 * QV068-GZ_Rainbow_MS-DOS_V2.05_Technical_Documentation_Nov84.pdf
 * Chapter 4-10 (PDF page 94) */
typedef _Packed struct ioctl_s {
  uint8_t func;           // Sub-function number
  uint8_t ret;            // Function return code: FFH = successful, 0 = unsuccessful
  uint8_t character;      // Character Input or Output
  uint8_t char_stat;      // Character Status
} ioctl_t;

/* Communications Control Block described in:
 * QV068-GZ_Rainbow_MS-DOS_V2.05_Technical_Documentation_Nov84.pdf
 * Page 4-7 (PDF page 91) */
typedef _Packed struct ccb_s {
  uint8_t device;         // Device number: 1=communications, 2=printer, 3=extended comm
  uint8_t modem_control;  // Modem Control: 1=data only, 2=limited modem control
  uint8_t stop_bits;      // Stop bits: 1=one, 2=one and one half, 3=two
  uint8_t data_bits;      // Data bits: 5, 6, 7, 8, 7S, 7M
  uint8_t parity;         // Parity: Even, Odd, None
  uint8_t rcv_baud;       // Receive baud
  uint8_t xmt_baud;       // Transmit baud
  uint8_t xon_char;       // If alt XON character to be used
  uint8_t xoff_char;      // If alt XOFF character to be used
  uint8_t rcv_xonxoff;    // On or Off
  uint8_t xmt_xonxoff;    // On or Off
  uint16_t alt_buf_size;  // Alternate buffer size
  uint16_t alt_buf_offs;  // Start address of buffer in offset, segment format
  uint16_t alt_buf_seg;   // Start address of buffer in offset, segment format
} ccb_t;

/**
 * Initialize port
 */
unsigned char rbcomms_init(void);

/**
 * Send byte
 */
void rbcomms_send_byte(unsigned char b);

/**
 * Get Port Status
 */
short rbcomms_get_status(void);

/**
 * Is data waiting?
 * Return 0 if nothing, 1 if data waiting.
 */
unsigned short rbcomms_data_waiting(void);

/**
 * Read byte
 */
unsigned char rbcomms_read_byte(void);

#endif /* RBCOMMS_H */
