/*
 * Copyright 2006 Inventive Technology GmbH
 * 
 * This file is part of tapecat.
 *
 * tapecat is free software; you can redistribute it and/or modify it under the terms of the GNU 
 * General Public License as published by the Free Software Foundation; version 2 of the License.
 *
 * tapecat is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without 
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with tapecat; if not,
 * write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301, USA 
*/

#include <stdint.h>
/* defines. */
#define TAPECAT_VERSION "1.0.0"
#define READ_BUFFER_SIZE (1024*1024)
#define BUFFER_FILE_TYPE (1*1024)
#define MAX_CHARS_LINE    32
#define MAX_DATA_BLOCK     4
#define MIN_DISPLAYABLE 0x20
#define MAX_DISPLAYABLE 0xef
#define MAX_BYTES_SHOWN  512
#define DEFAULT_TAPE_DEVICE "/dev/nst0"
#define AMANDA_TAPESTART "AMANDA: TAPESTART "
#define AMANDA_TAPEEND "AMANDA: TAPEEND "
#define AMANDA_FILE "AMANDA: FILE "

struct amdata {
	int isamtapestart;
	int isamtapeend;
	int isamfile;
	char start_date[11];
	char start_label[256];
	char end_date[11];
	char file_date[11];
	char file_host[256];
	char file_disk[256];
};

/* prototypes. */
void exit_with_error();
void exit_with_warning();
void exit_ok();
void get_cmdline(const int, char **);
void get_ioctl_statistics(const int, struct mtget *);
void get_amanda_tapestart_info(const unsigned char *, const ssize_t, struct amdata *);
void get_amanda_tapeend_info(const unsigned char *, const ssize_t, struct amdata *);
void get_amanda_file_info(const unsigned char *, const ssize_t, struct amdata *);
void print_dump_data(const unsigned char *, const ssize_t);
void check_file_type(const unsigned char *, const ssize_t);
