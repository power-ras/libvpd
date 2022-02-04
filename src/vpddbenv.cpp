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
#include <libvpd-2/logger.hpp>
#include <libvpd-2/debug.hpp>

#include <sstream>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace::std;

namespace lsvpd
{
	const string VpdDbEnv::UpdateLock::UPDATE_LOCK_SUFFIX( "-updatelock" );
	const string VpdDbEnv::TABLE_NAME ( "components" );
	const string VpdDbEnv::ID         ( "comp_id" );
	const string VpdDbEnv::DATA       ( "comp_data" );

	pid_t VpdDbEnv::UpdateLock::lockFileUpdate( bool blocking )
	{
		const string fname( mDbPath + UPDATE_LOCK_SUFFIX );
		int rc;
		struct flock fl = {
			.l_type = F_WRLCK,
			.l_whence = SEEK_SET,
			.l_start = 0,
			.l_len = 1,
			.l_pid = -1,
		};

		if ( lockfd >= 0 )
			return -EBUSY;

		lockfd = open( fname.c_str( ), O_RDWR|O_CREAT, 0644 );
		if ( lockfd < 0 )
			return -errno;

		rc = fcntl( lockfd, blocking ? F_SETLKW : F_SETLK, &fl );
		if ( rc ) {
			if ( ! blocking && ( errno == EACCES || errno == EAGAIN ) ) {
				if ( ! fcntl ( lockfd, F_GETLK, &fl ) )
					rc = fl.l_pid;
			} else {
				rc = -errno;
			}
			lockfd = -1;
		}
		return rc;
	}

	void VpdDbEnv::UpdateLock::unlockFileUpdate( void )
	{
		if ( lockfd > 0 ) {
			close( lockfd );
			lockfd = -1;
		}
	}

	VpdDbEnv::UpdateLock::~UpdateLock()
	{
		unlockFileUpdate();
	}

	VpdDbEnv::UpdateLock::UpdateLock( const string& envDir, const string& dbFileName,
			bool readOnly = false ):
		mReadOnly( readOnly ),
		mDbFileName( dbFileName ),
		mEnvDir( envDir ),
		lockfd( -1 )
	{
		struct stat st;
		bool dbExists;
		pid_t rc;
		ostringstream message;
		Logger l;

		mDbPath = mEnvDir + "/" + mDbFileName;
		dbExists = (stat( mDbPath.c_str( ), &st )) == 0;
		if (readOnly) {
			if (!dbExists) {
				message << mDbPath << ": DB requested for reading does not "
					"exist (" << errno << ":" <<
					strerror(errno) << ")." << endl;
				goto CON_ERR;
			}
		}
		rc = lockFileUpdate( false );
		while ( rc > 0 ) {
			ostringstream message;

			message << mDbPath << ": locked by another process (" << rc << "), waiting ..." << endl;
			rc = lockFileUpdate( true );
			l.log( message.str( ), LOG_INFO );
		}
		if ( rc < 0 ) {
			ostringstream message;

			message << mDbPath << ": cannot lock (" << strerror( -rc ) << ")." << endl;
			l.log( message.str( ), LOG_ERR );
		}
		return;
CON_ERR:
		l.log( message.str( ), LOG_ERR );
		VpdException ve( message.str( ) );
		throw ve;
	}

	static int SQLiteBusyHandler( void * user_data , int number_of_calls )
	{
		sqlite3 *mpVpdDb = (sqlite3 *)user_data;
		ostringstream message;
		Logger l;

		message << "SQLite database busy, waiting... " <<
			sqlite3_errmsg( mpVpdDb ) << endl;
		l.log( message.str( ), LOG_WARNING );
		sleep( number_of_calls );

		return 1; /* keep trying */
	}

	VpdDbEnv::VpdDbEnv( const string& envDir, const string& dbFileName,
				bool readOnly = false ) :
		mUpdateLock( *new UpdateLock(envDir, dbFileName, readOnly )),
		mDbFileName( dbFileName ),
		mEnvDir( envDir ),
		mpVpdDb( NULL )
	{
		initFromLock();
	}

	VpdDbEnv::VpdDbEnv( const VpdDbEnv::UpdateLock& lock ):
		mUpdateLock( lock ),
		mDbFileName( mUpdateLock.mDbFileName ),
		mEnvDir( mUpdateLock.mEnvDir ),
		mpVpdDb( NULL )
	{
		initFromLock();
	}

	void VpdDbEnv::initFromLock( void )
	{
		int rc;
		Logger l;
		ostringstream message;
		struct stat st;
		bool dbExists;
		bool readOnly = mUpdateLock.mReadOnly;
		sqlite3_stmt *pstmt;
		const char *out;
		string async = "PRAGMA synchronous = OFF";

		mDbPath = mUpdateLock.mDbPath;
		dbExists = (stat( mDbPath.c_str( ), &st )) == 0;

		for( unsigned int seconds = 1;; seconds++ ) {
			rc = sqlite3_open_v2( mDbPath.c_str( ), &mpVpdDb,
					readOnly ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
					NULL );
			if (rc == SQLITE_BUSY) {
				ostringstream message;

				message << "SQLite database busy, waiting... " <<
					sqlite3_errmsg( mpVpdDb ) << endl;
				l.log( message.str( ), LOG_WARNING );
				sleep( seconds );
			} else {
				break;
			}
		}
		if( rc != SQLITE_OK )
		{
			message << "SQLite Error " << rc << ": " <<
				sqlite3_errmsg( mpVpdDb ) << endl;
			goto CON_ERR;
		}
		rc = sqlite3_busy_handler( mpVpdDb, SQLiteBusyHandler, (void *)mpVpdDb );
		if( rc != SQLITE_OK )
		{
			message << "SQLITE Error " << rc << ": " <<
				sqlite3_errmsg( mpVpdDb ) << endl;
			goto CON_ERR;
		}

		if( !dbExists )
		{
			ostringstream sql;
			sql << "CREATE TABLE " << TABLE_NAME << " ( " << ID <<
				" TEXT NOT NULL UNIQUE, " << DATA << " BLOB NOT NULL );";
			string stmt = sql.str( );
			rc = SQLITE3_PREPARE( mpVpdDb, stmt.c_str( ),
						stmt.length( ) + 1, &pstmt, &out );
			if( rc != SQLITE_OK )
			{
				message << "SQLITE Error " << rc << ": " <<
					sqlite3_errmsg( mpVpdDb ) << endl;
				goto CON_ERR;
			}

			rc = sqlite3_step( pstmt );
			if( rc != SQLITE_DONE )
			{
				message << "SQLITE Error " << rc << ": " <<
					sqlite3_errmsg( mpVpdDb ) << endl;
				goto CON_ERR;
			}
			sqlite3_finalize( pstmt );
		}

		SQLITE3_PREPARE( mpVpdDb, async.c_str( ), async.length( ) + 1,
				&pstmt, &out );
		sqlite3_step( pstmt );
		sqlite3_finalize( pstmt );

		return;
CON_ERR:
		l.log( message.str( ), LOG_ERR );
		VpdException ve( message.str( ) );
		if( mpVpdDb != NULL )
			sqlite3_close( mpVpdDb );
		throw ve;
	}

	VpdDbEnv::~VpdDbEnv()
	{
		int rc;
		rc = sqlite3_close( mpVpdDb );
		if( rc != SQLITE_OK )
		{
			Logger l;
			ostringstream message;
			message << "SQLITE Error " << rc << ": " <<
				sqlite3_errmsg( mpVpdDb ) << endl;
			l.log( message.str( ), LOG_ERR );
		}
		delete &mUpdateLock;
	}

	Component* VpdDbEnv::fetch( const string& deviceID )
	{
		Component* ret = NULL;
		sqlite3_stmt *pstmt = NULL;
		int rc;
		const char *out;
		ostringstream message;
		string sql = "SELECT " + DATA + " FROM " + TABLE_NAME + " WHERE " +
				ID + "=?";

		rc = SQLITE3_PREPARE( mpVpdDb, sql.c_str( ), sql.length( ) + 1,
					&pstmt, &out );
		if( rc != SQLITE_OK )
			goto FETCH_COMP_ERR;

		rc = sqlite3_bind_text(pstmt, 1, deviceID.c_str(),
				       deviceID.length(), SQLITE_STATIC);
		if (rc != SQLITE_OK)
			goto FETCH_COMP_ERR;

		rc = sqlite3_step( pstmt );
		if( rc != SQLITE_ROW && rc != SQLITE_DONE )
			goto FETCH_COMP_ERR;

		if( rc == SQLITE_ROW ) {
			try {
				ret = new Component( sqlite3_column_blob( pstmt, 0 ) );
			}
			catch (std::bad_alloc& ba) {
				message << "SQLITE Error: call to new() failed " << endl;
				goto FETCH_NEW_FAILED;
			}
		}

		sqlite3_finalize( pstmt );
		return ret;

FETCH_COMP_ERR:
		message << "SQLITE Error " << rc << ": " <<
			sqlite3_errmsg( mpVpdDb ) << endl;
FETCH_NEW_FAILED:
		Logger().log( message.str( ), LOG_ERR );
		if( pstmt )
			sqlite3_finalize( pstmt );
		return ret;
	}

	/*
	 * Fetches the system root (the base of the component tree that
	 * stores all the system-wide vpd.
	 */
	System* VpdDbEnv::fetch( )
	{
		System* ret = NULL;
		sqlite3_stmt *pstmt = NULL;
		int rc;
		const char *out;
		ostringstream message;

		string sql = "SELECT " + DATA + " FROM " + TABLE_NAME + " WHERE " +
				ID + "='" + System::ID + "';";
		rc = SQLITE3_PREPARE( mpVpdDb, sql.c_str( ), sql.length( ) + 1,
					&pstmt, &out );
		if( rc != SQLITE_OK )
			goto FETCH_SYS_ERR;

		rc = sqlite3_step( pstmt );
		if( rc != SQLITE_ROW && rc != SQLITE_DONE )
			goto FETCH_SYS_ERR;

		if( rc == SQLITE_ROW ) {
			try {
				ret = new System( sqlite3_column_blob( pstmt, 0 ) );
			}
			catch (std::bad_alloc& ba) {
				message << "SQLITE Error: call to new() failed " << endl;
				goto FETCH_NEW_FAILED;
			}
		}

		sqlite3_finalize( pstmt );
		return ret;

FETCH_SYS_ERR:
		message << "SQLITE Error " << rc << ": " <<
			sqlite3_errmsg( mpVpdDb ) << endl;
FETCH_NEW_FAILED:
		Logger().log( message.str( ), LOG_ERR );
		if( pstmt )
			sqlite3_finalize( pstmt );
		return ret;
	}

	bool VpdDbEnv::store( Component *storeMe )
	{
		void * buffer = NULL;
		unsigned int dataSize;
		int rc;
		const char *out;
		sqlite3_stmt *pstmt = NULL;

		string sql = "INSERT INTO " + TABLE_NAME + " (" + ID + ", " + DATA +
				") VALUES ('" + storeMe->getID( ) + "', ?);";
		rc = SQLITE3_PREPARE( mpVpdDb, sql.c_str( ), sql.length( ) + 1,
					&pstmt, &out );
		if( rc != SQLITE_OK )
			goto STORE_COMP_ERR;

		dataSize = storeMe->pack( &buffer );
		rc = sqlite3_bind_blob( pstmt, 1, buffer, dataSize, SQLITE_TRANSIENT );
		if( rc != SQLITE_OK )
			goto STORE_COMP_ERR;

		rc = sqlite3_step( pstmt );
		if( rc != SQLITE_DONE )
			goto STORE_COMP_ERR;

		if( buffer != NULL )
			delete [] (char*)buffer;
		sqlite3_finalize( pstmt );
		return true;

STORE_COMP_ERR:
		if( buffer != NULL )
			delete [] (char*)buffer;
		Logger l;
		ostringstream message;
		message << "SQLITE Error " << rc << ": " <<
			sqlite3_errmsg( mpVpdDb ) << endl;
		l.log( message.str( ), LOG_ERR );

		if ( rc == SQLITE_BUSY )
			l.log( "Another instance of vpdupdate running." );

		sqlite3_finalize( pstmt );
		return false;
	}

	bool VpdDbEnv::store( System *storeMe )
	{
		void * buffer = NULL;
		unsigned int dataSize;
		int rc;
		const char *out;
		sqlite3_stmt *pstmt = NULL;

		string sql = "INSERT INTO " + TABLE_NAME + " (" + ID + ", " + DATA +
				") VALUES ('" + storeMe->getID( ) + "', ?);";
		rc = SQLITE3_PREPARE( mpVpdDb, sql.c_str( ), sql.length( ) + 1,
					&pstmt, &out );
		if( rc != SQLITE_OK )
			goto STORE_SYS_ERR;

		dataSize = storeMe->pack( &buffer );
		rc = sqlite3_bind_blob( pstmt, 1, buffer, dataSize, SQLITE_TRANSIENT );
		if( rc != SQLITE_OK )
			goto STORE_SYS_ERR;

		rc = sqlite3_step( pstmt );
		if( rc != SQLITE_DONE )
			goto STORE_SYS_ERR;

		if( buffer != NULL )
			delete [] (char*)buffer;
		sqlite3_finalize( pstmt );
		return true;

STORE_SYS_ERR:
		if( buffer != NULL )
			delete [] (char*)buffer;
		Logger l;
		ostringstream message;
		message << "SQLITE Error " << rc << ": " <<
			sqlite3_errmsg( mpVpdDb ) << endl;
		l.log( message.str( ), LOG_ERR );

		if ( rc == SQLITE_BUSY )
			l.log( "Another instance of vpdupdate running." );

		sqlite3_finalize( pstmt );
		return false;
	}

	bool VpdDbEnv::remove( const string& deviceID )
	{
		int rc;
		const char *out;
		sqlite3_stmt *pstmt = NULL;

		string sql = "DELETE FROM " + TABLE_NAME + " WHERE " + ID + "='" +
			deviceID + "';";
		rc = SQLITE3_PREPARE( mpVpdDb, sql.c_str( ), sql.length( ) + 1,
					&pstmt, &out );
		if( rc != SQLITE_OK )
			goto REMOVE_ERR;

		rc = sqlite3_step( pstmt );
		if( rc != SQLITE_DONE )
			goto REMOVE_ERR;
		sqlite3_finalize( pstmt );
		return true;

REMOVE_ERR:
		Logger l;
		ostringstream message;
		message << "SQLITE Error " << rc << ": " <<
			sqlite3_errmsg( mpVpdDb ) << endl;
		l.log( message.str( ), LOG_ERR );
		sqlite3_finalize( pstmt );
		return false;
	}

	vector<string> VpdDbEnv::getKeys( )
	{
		vector<string> ret;
		sqlite3_stmt *pstmt = NULL;
		int rc;
		const char *out;

		string sql = "SELECT " + ID + " FROM " + TABLE_NAME + ";";
		rc = SQLITE3_PREPARE( mpVpdDb, sql.c_str( ), sql.length( ) + 1,
					&pstmt, &out );
		if( rc != SQLITE_OK )
		{
			Logger l;
			ostringstream message;
			message << "SQLITE Error " << rc << ": " <<
				sqlite3_errmsg( mpVpdDb ) << endl;
			l.log( message.str( ), LOG_ERR );
			return ret;
		}

		rc = sqlite3_step( pstmt );
		while( rc == SQLITE_ROW )
		{
			const char *row = (const char*)sqlite3_column_text( pstmt, 0 );
			if( row )
				ret.push_back( string( row ) );
			rc = sqlite3_step( pstmt );
		}
		sqlite3_finalize( pstmt );
		return ret;
	}
}
