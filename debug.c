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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mtio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "debug.h"

extern int dodebug;
extern int dodebugblk;

void print_file_statistics(const struct stat *filestat)
{
	if (!dodebug) {
		return;
	}
	printf("file statistics:\n");
	printf("  device=%04llX\n", filestat->st_dev);
	printf("  i-node=%li\n", filestat->st_ino);
	printf("  protection=%i\n", filestat->st_mode);
	printf("  hlinks=%i\n", filestat->st_nlink);
	printf("  uid=%i\n", filestat->st_uid);
	printf("  gid=%i\n", filestat->st_gid);
	printf("  devtype=%04llX\n", filestat->st_rdev);
	printf("  totsize=%li\n", filestat->st_size);
	printf("  bklsize=%li\n", filestat->st_blksize);
	printf("  allocblks=%li\n", filestat->st_blocks);
	printf("  lastacc=%li\n", filestat->st_atime);
	printf("  lastmod=%li\n", filestat->st_mtime);
	printf("  laststchg=%li\n", filestat->st_ctime);
} /* print_file_statistics */

void print_ioctl_statistics(const struct mtget *mt_status)
{
	if (!dodebug) {
		return;
	}
	printf("ioctl statistics:\n");	
	printf("  type=%li\n", mt_status->mt_type);
	printf("  partition=%li\n", mt_status->mt_resid);
	printf("  blksize=%li\n", mt_status->mt_dsreg & 0x0fff);
	printf("  density=%li\n", mt_status->mt_dsreg >> 24);
	printf("  status=%li\n", mt_status->mt_gstat);
	printf("    eof is %li\n", GMT_EOF(mt_status->mt_gstat));
	printf("    bot is %li\n", GMT_BOT(mt_status->mt_gstat));
	printf("    eot is %li\n", GMT_EOT(mt_status->mt_gstat));
	printf("    sma is %li\n", GMT_SM(mt_status->mt_gstat));
	printf("    eod is %li\n", GMT_EOD(mt_status->mt_gstat));
	printf("    wpr is %li\n", GMT_WR_PROT(mt_status->mt_gstat));
	printf("    onl is %li\n", GMT_ONLINE(mt_status->mt_gstat));
	printf("    dro is %li\n", GMT_DR_OPEN(mt_status->mt_gstat));
	printf("    imr is %li\n", GMT_IM_REP_EN(mt_status->mt_gstat));
	printf("  recerror=%li\n", mt_status->mt_erreg);
	printf("  fileno=%i\n", mt_status->mt_fileno);
	printf("  blkno=%i\n", mt_status->mt_blkno);
} /* print_ioctl_statistics */

void print_options(const int dofast, const int dodebug, const int dorewind, const int dumpsize, 
	const int dodumpdata, const char * filename)
{
	if (!dodebug) {
		return;
	}
	printf("Options value:\n");
	printf("   dofast=%i\n", dofast);
	printf("   dodebug=%i\n", dodebug);
	printf("   dorewind=%i\n", dorewind);
	printf("   dumpsize=%i\n", dumpsize);
	printf("   dumpdata=%i\n", dodumpdata);
	printf("   filename=%s\n", filename);
} /* print_options */

void print_blksize(const int blksize)
{
	if (!dodebug) {
		return;
	}
	printf("Detected blksize: %i\n", blksize);
} /* print_blksize */

void print_blkinfo(const int blkonfile, const long int filesize, const int totread)
{
	if (!dodebug || !dodebugblk) {
		return;
	}
	printf("Block on file: %i - size is %i - total file size is %.2fK\n", blkonfile, totread, (double) (filesize / 1024));
} /* print_blkinfo */
