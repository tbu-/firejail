/*
 * Copyright (C) 2015 netblue30 (netblue30@yahoo.com)
 *
 * This file is part of firetools project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef DBSTORAGE_H
#define DBSTORAGE_H
#include <stdio.h>
#include <assert.h>

struct DbStorage {
	float cpu_;
	float rss_;
	float shared_;
	float rx_;
	float tx_;
	
	DbStorage(): cpu_(0), rss_(0), shared_(0), rx_(0), tx_(0) {}
	
	void dbgprint(int cycle) {
		printf("%d: %.2f, %.2f, %.2f, %.2f, %.2f\n",
			cycle, cpu_, rss_, shared_, rx_, tx_);
	}
	
	float get(int id) {
		switch (id) {
			case 0:
				return cpu_;
			case 1:
				return rss_ + shared_;
			case 2:
				return rx_;
			case 3:
				return tx_;
			default:
				assert(0);
		}

		return 0;
	}
}; 

#endif
