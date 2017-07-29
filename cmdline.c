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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mtio.h>
#include <sys/types.h>
#include <errno.h>
#define _GNU_SOURCE
#include <getopt.h>
#include "tapecat.h"
#include "debug.h"

void print_help_info();
  
extern char * filename;
extern int dorewind;
extern int dodumpdata;
extern int dumpsize;
extern int dodebug;
extern int dodebugblk;
extern int dofast;

extern char * optarg;
extern int optind,
           opterr,
           optopt;

/*
 * Get command parameters.
 * -d / --debug - debug mode on.
 *-f / --fast - fast run - reads only first block and then do a fsf 1.
 * -k / --debugblk - debug of every block on.
 * -r / --norewind - don't rewind tape.
 * -s / --dumpsize - value for MAX_BYTES_SHOWN
 *  -u / --dumpdata - show first MAX_BYTES_SHOWN of data for each file.
 */
void get_cmdline(const int argc, char **argv)
{
 	int i = 0,
		error = 0,
		locerrno = 0,
		opt = 0;
	struct option longopts[] = {
		{"debug", 0, NULL, 'd'},
		{"debugblk", 0, NULL, 'k'}, 
		{"fast", 0, NULL, 'f'}, 
		{"norewind", 0, NULL, 'r'}, 
		{"dumpsize", 1, NULL, 's'}, 
		{"dumpdata", 0, NULL, 'u'}, 
		{"help", 0, NULL, 'h'}
	};
	
	while ((opt = getopt_long(argc, argv, "dfkrs:u", longopts, NULL)) != -1) {
		switch(opt) {
			case ':':
			case '?':
				error = 1;
				break;
			case 'd':
				dodebug = 1;
				break;
			case 'f':
				dofast = 1;
				break;
			case 'h':
				print_help_info();
				exit_ok();
				break;
		      case 'k':
			        dodebug = 1;
			        dodebugblk = 1;
			        break;
			case 'r':
				dorewind = 0;
				break;
			case 's':
				dodumpdata = 1;
				dumpsize = strtol(optarg, (char **)NULL, 10);
				if (errno != 0 || dumpsize == 0) {
					locerrno = errno;
					printf("Invalid numeric value for --dumpsize: %s\n", optarg);
					error = 1;
				}
				break;
			case 'u':
				dodumpdata = 1;
				break;
		} /* switch */
	} /* while */

	for (i = optind; i < argc; i++) {
		if (filename == NULL) {
			filename = argv[i];
		} else {
			printf("Filename alreay specified: %s\n", argv[i]);
			error = 1;
		}
	}
 
	if (filename == NULL) {
		filename = DEFAULT_TAPE_DEVICE;
	}

  	print_options(dofast, dodebug, dorewind, dumpsize, dodumpdata, filename);
  
	if (error) {
		printf("Exiting on errors...\n");
		exit_with_error();
	}
} /* get_cmdline */

/*
 * Print help information for the program.
 * Remember to change the manpage, too.
 */
void print_help_info()
{
	printf("tapecat version %s\n", TAPECAT_VERSION);
	printf("\n");
	printf("Usage: tapecat [option]... [tape_device]\n");
	printf("When tape_device is not specified uses /dev/nst0 by default.\n");
	printf("Options are:\n");
	printf("  -d, --debug        turn on debugging messages.\n");
	printf("  -f, --fast         turn on fast scan of the tape. It reads only the first block\n");
	printf("                     of each file on tape and then skips to the next file.\n");
	printf("  -k, --debugblk     turn on block debugging. That means to print a message for\n");
	printf("                     each block read. Implies --debug.\n");
	printf("  -r, --norewind     don't do a rewind prior to reading the tape. Be carefull if\n");
	printf("                     you're using an auto-rewind device: as the name implies it \n");
	printf("                     will make a rewind by itself.\n");
	printf("  -s N, --dumpsize=N set the number of bytes to dump to N. Implies --dumpdata.\n");
	printf("  -u, --dumpdata     dump bytes from the beginning of each file on tape to stdin.\n");
	printf("                     The number of bytes dumped can be set using --dumpsize.\n");
	printf("                     Default is 512.\n");
	printf("  --help             guess what...\n");
	printf("\n");
} /* print_help_info */
