#include <stdint.h>

#ifndef RBDISK_H
#define RBDISK_H

extern char dos_version;
extern __segment biosseg;

typedef struct {
  short c; /* Cylinders */
  short h; /* Heads */
  short s; /* Sectors per Track */
} DiskGeometry;

extern DiskGeometry geometry;

/**
 * Device Header Layout
 * http://www.techhelpmanual.com/322-device_header_layout.html
 */
typedef _Packed struct ddentry_s {
  int next_offset;
  int next_segment;
  int flags;
  int stg_offset;
  int int_offset;
  char devname[8];
  /* and probably some more stuff */
} ddentry_t;

/**
 * Device Request Header
 * http://www.techhelpmanual.com/324-device_request_header.html
 */
typedef _Packed struct driverreq_s {
  unsigned char len;
  unsigned char unit;
  unsigned char cmd;
  unsigned int status;
  unsigned char res[8];
  unsigned char mediadescr;
  unsigned int addroffs;
  __segment addrseg;
  unsigned int count;
  unsigned int start;
} driverreq_t;

/**
 * Winchester conrol block, described in
 * QV068-GZ_Rainbow_MS-DOS_V2.05_Technical_Documentation_Nov84.pdf
 * Page 4-25 (PDF page 109)
 */
typedef _Packed struct wctlbl_s {
  unsigned char func;
  unsigned char drive;
  unsigned char sector;
  unsigned char unit;
  unsigned int track;
  unsigned int sectors;
  unsigned int bufoffs;
  __segment bufseg;
  unsigned char status;
  unsigned char error;
} wctlbl_t;

/**
 * Home Block (HOM)
 *
 * Described in:
 * QV068-GZ_Rainbow_MS-DOS_V2.05_Technical_Documentation_Nov84.pdf
 * Page A-4 (PDF page 127)
 *
 * If the drive was properly initialized, we can find the disk geometry here.
 */
typedef _Packed struct hom_s {
  uint8_t id[3];
  int8_t part_flag;
  uint16_t checksum;
  uint8_t volume_id[8];
  uint16_t system_id[2];
  uint8_t bat_stuff[5];
  uint8_t dpd_stuff[5];
  uint8_t osn_stuff[5];
  uint8_t boot_stuff[5];
  uint8_t ast_stuff[5];
  uint16_t firstalt_track;
  uint8_t num_alt_tracks;
  uint8_t autoboot;
  uint16_t boot_track;
  uint8_t mbz[15];
  uint16_t cylinders;
  uint8_t sectors_per_track;
  uint16_t sector_size;
  uint8_t surfaces;
  uint16_t maint_cylinder;
  uint16_t mfg_cylinder;
  uint16_t precomp_value;
  uint8_t steprate;
  uint8_t typecode;
  uint8_t this_block_number;
  uint16_t mbz2[192];
} hom_t;

int rbdisk_init(void);
void dump_sector(char *buf);
int rbdisk_read_sector(short c, unsigned char h, unsigned char s, char* buf);
int rbdisk_geometry(DiskGeometry* geometry, char *buf);

#endif /* RBDISK_H */
