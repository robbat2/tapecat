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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mtio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "tapecat.h"
#include "debug.h"

char * filename = NULL;
int file = 0;
int dorewind = 1;
int dodumpdata = 0;
int dumpsize = MAX_BYTES_SHOWN;
int dodebug = 0;
int dodebugblk = 0;
int dofast = 0;
 
/* big buffer for trying to read any tape block in the world. */
unsigned char buf[READ_BUFFER_SIZE];
int blksize = sizeof(buf);
unsigned char bufftype[BUFFER_FILE_TYPE];
int szbufftype = sizeof(bufftype) - 1;
  
void exit_with_error()
{
  exit(8);
}

void exit_with_warning()
{
  exit(4);
}

void exit_ok()
{
  exit(0);
}

/*
 * Get tape statistics with ioctl call
 */
void get_ioctl_statistics(const int file, struct mtget * mt_status)
{
	int ferror = 0;
	struct mtop mt_cmd;
  
	/* nop tape. */
	mt_cmd.mt_op = MTNOP;
	mt_cmd.mt_count = 1;
	ferror = ioctl(file, MTIOCTOP, &mt_cmd);
	if (ferror == -1) {
		ferror = errno;
		printf("Error-ioctli/nop: %i %s\n", ferror, strerror(ferror));
		exit_with_error();
	}
	/* tape stat by ioctl. */
	ferror = ioctl(file, MTIOCGET, mt_status);
	if (ferror == -1) {
		ferror = errno;
		printf("Error-ioctli/get: %i %s\n", ferror, strerror(ferror));
		exit_with_error();
	}
} /* get_ioctl_statistics. */
  
/*
 * Get info from amanda tapestart file, ie, the label.
 */
void get_amanda_tapestart_info(const unsigned char * buf, const ssize_t totread, struct amdata * aminfo)
{
	int i = 0,
		j = 0,
		state = 0;
    
	aminfo->isamtapestart = 1;
	aminfo->isamtapeend = 0;
	aminfo->isamfile = 0;
	for (i = strlen(AMANDA_TAPESTART); i < totread; i++) {
		if (buf[i] == 0x0A) {
			break;
		}
		if (buf[i] == ' ') {
			state++;
			j = 0;
			continue;
		}
		/* skip until next blank. */
		switch (state) {
			case 0:  /* constant DATE. */
				break;
			case 1:  /* value of date. */
				if (j < (sizeof(aminfo->start_date) - 1)) {
					aminfo->start_date[j++] = buf[i];
					aminfo->start_date[j] = '\0';
				}
				break;
			case 2:  /* constant TAPE */
				break;
			case 3:  /* value of tape. */
				if (j < (sizeof(aminfo->start_label) - 1)){
					aminfo->start_label[j++] = buf[i];
					aminfo->start_label[j] = '\0';
				}
			break;
		} /* switch state. */
	} /* for buffer. */
} /* get_amanda_tapestart_info. */

/*
 * Get info from amanda tapeend file, ie, the end of the amanda backup.
 */
void get_amanda_tapeend_info(const unsigned char * buf, const ssize_t totread, struct amdata * aminfo)
{
	int i = 0,
		j = 0,
		state = 0;
  
	aminfo->isamtapestart = 0;
	aminfo->isamtapeend = 1;
	aminfo->isamfile = 0;
	for (i = strlen(AMANDA_TAPEEND); i < totread; i++) {
		if (buf[i] == 0x0A){
			break;
		}
		if (buf[i] == ' '){
			state++;
			j = 0;
			continue;
		}
		/* skip until next blank. */
		switch (state) {
			case 0:  /* constant DATE. */
				break;
			case 1:  /* value of date. */
				if (j < (sizeof(aminfo->end_date) - 1)){
					aminfo->end_date[j++] = buf[i];
					aminfo->end_date[j] = '\0';
				}
				break;
		} /* switch state. */
	} /* for buffer. */
} /* get_amanda_tapeend_info. */

/*
 * Get info from amanda backup file, ie, a disk on the configuration.
 */
void get_amanda_file_info(const unsigned char * buf, const ssize_t totread, struct amdata * aminfo)
{
	int i = 0,
		j = 0,
		state = 0;
  
	aminfo->isamtapestart = 0;
	aminfo->isamtapeend = 0;
	aminfo->isamfile = 1;
	for (i = strlen(AMANDA_FILE); i < totread; i++) {
		if (buf[i] == 0x0A) {
			break;
		}
		if (buf[i] == ' ') {
			state++;
			j = 0;
			continue;
		}
		/* skip until next blank. */
		switch (state) {
			case 0:  /* value of date. */
				if (j < (sizeof(aminfo->file_date) - 1)) {
					aminfo->file_date[j++] = buf[i];
					aminfo->file_date[j] = '\0';
				}
				break;
			case 1:  /* value of host. */
				if (j < (sizeof(aminfo->file_host) - 1)) {
					aminfo->file_host[j++] = buf[i];
					aminfo->file_host[j] = '\0';
				}
				break;
			case 2:  /* value of disk. */
				if (j < (sizeof(aminfo->file_disk) - 1)) {
					aminfo->file_disk[j++] = buf[i];
					aminfo->file_disk[j] = '\0';
				}
				break;
		} /* switch state. */
	} /* for buffer. */
} /* get_amanda_file_info. */

/*
 * Print the dumped data to stdin.
 */
void print_dump_data(const unsigned char * buf, const ssize_t totread)
{
	ssize_t totshown = 0;
	int i = 0,
		j = 0;

	/* loop on read data to dump the values. */
	totshown = (totread < dumpsize ? totread : dumpsize);
	if (totshown <= 0) {
		return;
	}
	for (i = 0; i < totshown; i++) {
		/* end of hex data, start text output. */
		if (i != 0 && !(i % MAX_CHARS_LINE)) {
			printf("  ");
			for (j = i - MAX_CHARS_LINE; j < i; j++) {
				if (buf[j] < MIN_DISPLAYABLE || buf[j] > MAX_DISPLAYABLE) {
					printf(".");
				} else {
					printf("%c", buf[j]);
				}
			} /* for text chars. */
			printf("\n");
		} /* if eol. */
		/* each hex block has 4 bytes. Then a whitespace. */
		if (!(i % MAX_DATA_BLOCK)) {
			printf(" ");
		}
		printf("%02X", buf[i]);
	} /* for */
	/* for the last line it's necessary to write the text output.  */
	/* write the necessary white spaces. */
	if (totshown % MAX_CHARS_LINE) {
		for (i = totshown; i < ((totshown - 1) / MAX_CHARS_LINE + 1) * MAX_CHARS_LINE; i++) {
			if (!(i % MAX_DATA_BLOCK)) {
				printf(" ");
			}
			printf("  ");
		}
	} /* if totread. */
	/* write the separator between hex and text output. */
	printf("  ");
  
	/* write the text output. */
	for (j = i - MAX_CHARS_LINE; j < totshown; j++) {
		if (buf[j] < MIN_DISPLAYABLE || buf[j] > MAX_DISPLAYABLE) {
			printf(".");
		} else {
			printf("%c", buf[j]);
		}
	} /* for text chars. */
	/* new line at the end of dumpdata. */
	printf("\n");
} /* print_dump_data. */

/* 
 * Fork a new process to call file /system command) to check the file type.
 */
void check_file_type(const unsigned char * inbuf, const ssize_t intotread)
{
	/* in and out here are relative to the parent. */
	int ferror = 0;
	ssize_t totread = 0;
	int fdpipein[2] = {-1, -1};
	int fdpipeout[2] = {-1, -1};
	pid_t forkresult;
	if (pipe(fdpipein) != 0) {
		ferror = errno;
		printf("Error-pipein: %i %s\n", ferror, strerror(ferror));
		goto _exitpoint;
	}
	if (pipe(fdpipeout) != 0) {
		ferror = errno;
		printf("Error-pipeout: %i %s\n", ferror, strerror(ferror));
		goto _exitpoint;
	}
	forkresult = fork();
	if (forkresult == (pid_t)-1) {
		ferror = errno;
		printf("Error-fork: %i %s\n", ferror, strerror(ferror));
		goto _exitpoint;
	} 
	/* child process. */
	if (forkresult == (pid_t)0) {
		/* redirecting stdin. */
		if (close(0) == -1) {
	    		ferror = errno;
	    		printf("Error-child-closestdin: %i %s\n", ferror, strerror(ferror));
			goto _exitpoint;
		} 
		if (dup(fdpipeout[0]) == -1) {
			ferror = errno;
   		    	printf("Error-child-dupstdin: %i %s\n", ferror, strerror(ferror));
			goto _exitpoint;
		}
		/* redirecting stdout. */
		if (close(1) == -1) {
			ferror = errno;
		    	printf("Error-child-closestdout: %i %s\n", ferror, strerror(ferror));
			goto _exitpoint;
		}
		if (dup(fdpipein[1]) == -1) {
			ferror = errno;
   	    		printf("Error-child-dupstdout: %i %s\n", ferror, strerror(ferror));
			goto _exitpoint;
		}
		if (close(fdpipein[0]) == -1) {
			ferror = errno;
			printf("Error-child-closepipein0: %i %s\n", ferror, strerror(ferror));
			goto _exitpoint;
		}
		if (close(fdpipeout[1]) == -1) {
			ferror = errno;
			printf("Error-child-closepipeout1: %i %s\n", ferror, strerror(ferror));
			goto _exitpoint;
		} 
		/* call file. */
		execlp("file", "file", "-", (char *) 0);
		exit(0);
	} /* if child process */
	
	/* parent process. */
	bufftype[0] = '\0';
	/* write data to stdin. */
	if (write(fdpipeout[1], inbuf, intotread) == -1) {
		ferror = errno;
		printf("Error-write-pipeout1: %i %s\n", ferror, strerror(ferror));
		goto _exitpoint;
	}
	/* close output so child process can read it. */
	if (close(fdpipeout[0]) == -1) {
		ferror = errno;
		printf("Error-close1pipeout0: %i %s\n", ferror, strerror(ferror));
		goto _exitpoint;
	}
	fdpipeout[0] = -1;
	if (close(fdpipeout[1]) == -1) {
		ferror = errno;
		printf("Error-close1pipeout1: %i %s\n", ferror, strerror(ferror));
		goto _exitpoint;
	}
	fdpipeout[1] = -1;
			
	/* wait for file to finish. */
	waitpid(forkresult, NULL, 0);
	if (fcntl(fdpipein[0], F_SETFL, fcntl(fdpipein[0], F_GETFL) | O_NONBLOCK) == -1) {
		ferror = errno;
		printf("Error-fcntl-setflpipeout0: %i %s\n", ferror, strerror(ferror));
		goto _exitpoint;
	}
	while ((totread = read(fdpipein[0], bufftype, szbufftype))) {
		if (totread == -1) {
			if (errno == 11) {
				break;
			} else {
	        		ferror = errno;
				printf("Error-readpipein0: %i %s\n", ferror, strerror(ferror));
			}
		}
		bufftype[totread] = '\0';
		/* read just one record. */
		break;
	} /* while reading stdout of file. */
	
_exitpoint:
	if (fdpipein[0] != -1 && close(fdpipein[0]) == -1) {
		ferror = errno;
		printf("Error-closepipein0: %i %s\n", ferror, strerror(ferror));
	}
	if (fdpipein[1] != -1 && close(fdpipein[1]) == -1) {
		ferror = errno;
		printf("Error-closepipein1: %i %s\n", ferror, strerror(ferror));
	}
	if (fdpipeout[0] != -1 && close(fdpipeout[0]) == -1) {
		ferror = errno;
		printf("Error-closepipeout0: %i %s\n", ferror, strerror(ferror));
	}
	if (fdpipeout[1] != -1 && close(fdpipeout[1]) == -1) {
		ferror = errno;
		printf("Error-closepipeout1: %i %s\n", ferror, strerror(ferror));
	}
} /* check_file_type */

int main(int argc, char **argv)
{
	int ferror = 0,
		warning = 0;
	int fileontape = 0,
		blkonfile = 0;
	long int filesize = 0;

	ssize_t totread = 0;
	struct stat filestat;
	struct mtop mt_cmd;
	struct mtget mt_status;
	struct amdata aminfo;

	get_cmdline(argc, argv);
 
	/* check file statistics. */
	ferror = stat(filename, &filestat);
	if (ferror == -1) {
		ferror = errno;
		printf("Error-stat1: %i %s\n", ferror, strerror(ferror));
		exit_with_error();
	}
  
	/* checks if it's a st device. */
	if (((filestat.st_rdev & 0xff00) >> 8) != 9) {
		printf("Device is not a tape device (major=9).\n");
		exit_with_error();
	}

	/* print file statistics. */
	print_file_statistics(&filestat);
 
	/* open file. */
	file = open(filename, O_RDONLY);
	if (file == -1) {
		ferror = errno;
		printf("Error-open: %i %s\n", ferror, strerror(ferror));
		exit_with_error();
	}

	/* rewind tape. */
	if (dorewind) {
		printf("Rewinding tape...\n");
		mt_cmd.mt_op = MTREW;
		mt_cmd.mt_count = 1;
		ferror = ioctl(file, MTIOCTOP, &mt_cmd);
		if (ferror == -1) {
			ferror = errno;
			printf("Error-ioctli/rew: %i %s\n", ferror, strerror(ferror));
			exit_with_error();
		}
	} /* if dorewind. */

	/* loop on tape. */
	mt_status.mt_gstat = 0;
	while (!GMT_EOD(mt_status.mt_gstat)) {
		blkonfile = 0;
		filesize = 0;
		aminfo.isamtapestart = 0;
		aminfo.isamtapeend = 0;
		aminfo.isamfile = 0;
		/* loop on file. */
		while ((totread = read(file, buf, blksize))) {
			if (totread == -1) {
				ferror = errno;
				printf("Error-read: %i %s\n", ferror, strerror(ferror));
				exit_with_error();
			}
			/* first record read - check if amanda info. */
			if (!filesize) {
				if (strlen(AMANDA_TAPESTART) <= totread && 
					!strncmp(buf, AMANDA_TAPESTART, strlen(AMANDA_TAPESTART))) {
					get_amanda_tapestart_info(buf, totread, &aminfo);
				} else if (strlen(AMANDA_TAPEEND) <= totread && 
					!strncmp(buf, AMANDA_TAPEEND, strlen(AMANDA_TAPEEND))) {
					get_amanda_tapeend_info(buf, totread, &aminfo);
				} else if (strlen(AMANDA_FILE) <= totread && 
					!strncmp(buf, AMANDA_FILE, strlen(AMANDA_FILE))) {
					get_amanda_file_info(buf, totread, &aminfo);
				} else { /* other files. */
					check_file_type(buf, totread);
				} /* if/else file type. */
			} /* if !filesize. */
			/* dump data. */
			if (dodumpdata && !filesize) {
				if (fileontape) {
					printf("\n");
				}
				print_dump_data(buf, totread);
			}
			filesize += totread;
			print_blkinfo(blkonfile++, filesize, totread);
			/* fast scanning. */
			if (dofast) {
				mt_cmd.mt_op = MTFSF;
				mt_cmd.mt_count = 1;
				ferror = ioctl(file, MTIOCTOP, &mt_cmd);
				if (ferror == -1) {
					ferror = errno;
					printf("Error-ioctli/fsf: %i %s\n", ferror, strerror(ferror));
					exit_with_error();
				}
				/* exit loop on file. */
				break;
			}
		} /* while loop on file. */
	
		/* get ioctl statistics to check tape status. */
		get_ioctl_statistics(file, &mt_status);
		print_ioctl_statistics(&mt_status);
		if (GMT_EOT(mt_status.mt_gstat)) {
			printf("Physical end of tape before end of data found.\n");
			warning = 1;
			break;
		}
		if (blkonfile) {
			if (dofast) {
				printf("First block of file %i has %.2fK.\n", fileontape, (double) (filesize / 1024));
			} else {
				printf("Blocks for file %i are %i. File size is %.2fK.\n", fileontape, blkonfile, (double) (filesize / 1024));
			}
			if (aminfo.isamtapestart) {
				printf("   File is Amanda TAPESTART. Date is %s, label is %s.\n", aminfo.start_date, aminfo.start_label);
			} else if (aminfo.isamtapeend) {
				printf("   File is Amanda TAPEEND. Date is %s, label is %s.\n", aminfo.end_date,
				aminfo.start_label);
			} else if (aminfo.isamfile) {
				printf("   File is Amanda FILE. Date is %s, label is %s, host is %s, disk is %s.\n", 
				aminfo.file_date, aminfo.start_label, aminfo.file_host, aminfo.file_disk);
			} else {
				printf("   File guessed:%s\n", strchr(bufftype, ' '));
			}
		} else {
			printf("Tape mark.\n");
		} /* if blkonfile. */
		fileontape++;
	} /* while loop on tape. */
  
	printf("\n");

	/* close file. */
	ferror = close(file);
	if (ferror == -1){
		ferror = errno;
		printf("Error-close: %i %s\n", ferror, strerror(ferror));
		exit_with_error();
	}
	if (warning) {
		printf("Program terminated with warnings!\n");
		exit_with_warning();
	} else {
		printf("Program terminated normally\n");
		exit_ok();
	}
	exit(0);
} /* main */
