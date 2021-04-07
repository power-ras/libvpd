/***************************************************************************
 *   Copyright (C) 2006, IBM                                               *
 *                                                                         *
 *   Maintained By:                                                        *
 *   Eric Munson and Brad Peters                                           *
 *   munsone@us.ibm.com, bpeters@us.ibm.com                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the Lesser GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2.1 of the  *
 *   License, or at your option) any later version.                        *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesser General Public License for more details.                   *
 *                                                                         *
 *   You should have received a copy of the Lesser GNU General Public      *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <libvpd-2/vpddbenv.hpp>
#include <libvpd-2/vpdretriever.hpp>
#include <libvpd-2/logger.hpp>

#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

using namespace std;

namespace lsvpd
{
	const string VpdRetriever::DEFAULT_DIR  ( "/var/lib/lsvpd/" );
	const string VpdRetriever::DEFAULT_FILE ( "vpd.db" );
	const string VpdRetriever::UDEV_NOTIFY_FILE ( "/run/run.vpdupdate" );

	VpdRetriever::VpdRetriever( string envDir,
		string dbFileName )
	{
		try {
			db = new VpdDbEnv( envDir, dbFileName, true );
		}
		catch (std::bad_alloc& ba) {
			log_err("Out of memory, failed to build VpdEnv.");
			VpdException ve("Out of memory, failed to build VpdEnv.");
			throw ve;
		}
	}
	
	VpdRetriever::VpdRetriever( )
	{
		struct stat vpd_stat,udev_stat;
		const string vpddb = VpdRetriever::DEFAULT_DIR + VpdRetriever::DEFAULT_FILE;
		const string udev_file = VpdRetriever::UDEV_NOTIFY_FILE;
		Logger logger;
		int flag = 1;

		/* Check if stat is successful for UDEV_NOTIFY_FILE. */
		if ( stat ( udev_file.c_str(), &udev_stat ) != 0 )
		{
			logger.log( "libvpd: Unable to stat udev rule file, run.vpdupdate.", LOG_INFO );
		}
		else
		{
			/* Find the modification time for default DB. */
			if ( stat ( vpddb.c_str(), &vpd_stat ) != 0 )
			{
				logger.log( "libvpd: Unable to stat vpd.db file.", LOG_INFO );
				if ( errno == ENOENT )
					vpd_stat.st_mtime = 0;
				else
					flag = 0;
			}

			if ( flag && ( udev_stat.st_mtime > vpd_stat.st_mtime ) )
			{
				/*
				 * This implies there were changes to devices on the system
				 * after the VPD db was created/modified. So
				 * run vpdupdate.
				 */
				pid_t cpid;	/* Pid of child */
				int rc;		/* Holds return value */
				int status;	/* exit value of child */

				logger.log( "libvpd: Running vpdupdate to update the default db.", LOG_INFO );
				cpid = fork();
				if (cpid == -1) {	/* fork failed */
					logger.log("libvpd: Fork failed, while running vpdupdate\n", LOG_INFO);
					VpdException ve( "libvpd: Fork failed, while running vpdupdate\n" );
					throw ve;
				} else if (cpid == 0) {	/* child */
					char *system_arg[2];
					int nullfd;

					system_arg[0] = (char *)"/usr/sbin/vpdupdate";
					system_arg[1] = NULL;

					nullfd = open("/dev/null", O_WRONLY);
					if (nullfd == -1) {
						log_notice("Failed to open /dev/null "
							   "(%d:%s)", errno,
							   strerror(errno));
						exit(-2);
					}

					if (dup2(nullfd, STDOUT_FILENO) == -1) {
						log_notice("Failed to redirect stderr "
							   "to /dev/null (%d:%s)",
							   errno, strerror(errno));
						 close(nullfd);
						 exit(-2);
					}

					if (dup2(nullfd, STDERR_FILENO) == -1) {
						log_notice("Failed to redirect stdout "
							   "to /dev/null (%d:%s)",
							   errno, strerror(errno));
						 close(nullfd);
						 exit(-2);
					}

					execv(system_arg[0], system_arg);
					exit(-2);
				} else {	/* parent */
					rc = waitpid(cpid, &status, 0);
					if (rc == -1)
						logger.log("libvpd: wait failed, while running vpdupdate\n", LOG_INFO);
					if (WIFEXITED(status) &&
					    (signed char)WEXITSTATUS(status) == -2)
						logger.log("libvpd: execv failed\n", LOG_INFO);
				}
			}
		}
		try {
			db = new VpdDbEnv( VpdRetriever::DEFAULT_DIR,
					   VpdRetriever::DEFAULT_FILE, true );
		}
		catch (std::bad_alloc& ba) {
			log_err("Out of memory, failed to build VpdEnv.");
			VpdException ve("Out of memory, failed to build VpdEnv.");
			throw ve;
		}
	}

	VpdRetriever::~VpdRetriever( )
	{
		if( db != NULL )
		{
			delete db;
		}
	}

	System* VpdRetriever::getComponentTree( )
	{
		System *root = db->fetch( );
		if (root)
			buildSubTree( root );
		else
		{
			Logger logger;
			logger.log( "Failed to fetch VPD DB, it may be corrupt.", LOG_ERR );
			VpdException ve( "Failed to fetch VPD DB, it may be corrupt." );
			throw ve;
		}

		return root;
	}

	void VpdRetriever::buildSubTree( System* root )
	{
		Component* leaf;
		const vector<string> children = root->getChildren( );
		vector<string>::const_iterator i, end;

		end = children.end( );

		for( i = children.begin( ); i != end; ++i )
		{
			const string next = *i;
			leaf = db->fetch( next );
			if( leaf == NULL )
			{
				Logger logger;
				logger.log( "Failed to fetch requested item.", LOG_ERR );
				VpdException ve( "Failed to fetch requested item." );
				throw ve;
			}
			buildSubTree( leaf );
			root->addLeaf( leaf );
		}
	}

	void VpdRetriever::buildSubTree( Component* root )
	{
		Component* leaf;
		const vector<string> children = root->getChildren( );
		vector<string>::const_iterator i, end;

		end = children.end( );

		for( i = children.begin( ); i != end; ++i )
		{
			const string next = *i;
			leaf = db->fetch( next );
			if( leaf == NULL )
			{
				Logger logger;
				logger.log( "Failed to fetch requested item.", LOG_ERR );
				VpdException ve( "Failed to fetch requested item." );
				throw ve;
			}
			buildSubTree( leaf );
			root->addLeaf( leaf );
		}
	}
}

