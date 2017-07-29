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

void print_file_statistics(const struct stat *);
void print_ioctl_statistics(const struct mtget *);
void print_options(const int, const int, const int, const int, const int, const char *);
void print_blksize(const int);
void print_blkinfo(const int, const long int, const int);

