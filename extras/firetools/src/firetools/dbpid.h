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
#ifndef DBPID_H
#define DBPID_H
#include <sys/types.h>
#include <unistd.h>
#include "dbstorage.h"

class DbPid {
public:
	static const int MAXCYCLE = 48;
	DbStorage data_[MAXCYCLE];

	DbPid(pid_t pid);
	~DbPid();
	void setCmd(const char *cmd);
	const char *getCmd() {
		return cmd_;
	}

	void add(DbPid *dbpid);
	void remove(DbPid *dbpid);
	DbPid *find(pid_t pid);
	void dbgprint();
	DbPid *getNext() {
		return next_;
	}
	void resetNext() {
		next_ = 0;
	}
	pid_t getPid() {
		return pid_;
	}
	bool networkDisabled() {
		return network_disabled_;
	}
	void setNetworkDisabled(bool val) {
		network_disabled_ = val;
	}
	uid_t getUid() {
		return uid_;
	}
	void setUid(uid_t val) {
		uid_ = val;
	}

private:	
	DbPid *next_;
	pid_t pid_;
	char *cmd_;
	bool network_disabled_;
	uid_t uid_;
};

#endif