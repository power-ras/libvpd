/***************************************************************************
 *   Copyright (C) 2007, IBM                                               *
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

#include <libvpd-2/vpddbenv.h>
#include <stdio.h>

#define DATA_BUFFER_SIZE	12288
#define FULL_PATH_SIZE		(MAX_NAME_LENGTH * 2 + 2)

struct vpddbenv * new_vpddbenv( const char *dir, const char *file )
{
	struct vpddbenv *ret;
	int rc = 0;
	
	ret = calloc( 1, sizeof( struct vpddbenv ) );
	if( !ret )
		return ret;
	
	if( dir == NULL || strncmp( dir, "", MAX_NAME_LENGTH ) == 0 )
		strncpy( ret->envDir, DEFAULT_ENV, MAX_NAME_LENGTH );
	else {
		strncpy( ret->envDir, dir, MAX_NAME_LENGTH );
		ret->envDir[MAX_NAME_LENGTH] = '\0';
	}
	
	if( file == NULL || strncmp( file, "", MAX_NAME_LENGTH ) == 0 )
		strncpy( ret->dbFileName, DEFAULT_DB, MAX_NAME_LENGTH );
	else {
		strncpy( ret->dbFileName, file, MAX_NAME_LENGTH );
		ret->dbFileName[MAX_NAME_LENGTH] = '\0';
	}

	strncpy( ret->fullPath, ret->envDir , FULL_PATH_SIZE - 1);

	if (strlen(ret->fullPath) + 1 < FULL_PATH_SIZE)
		strcat( ret->fullPath, "/" );
	if (strlen(ret->fullPath) + strlen(ret->dbFileName) < FULL_PATH_SIZE)
		strcat( ret->fullPath, ret->dbFileName );

	ret->fullPath[FULL_PATH_SIZE - 1] = '\0';
	
	rc = sqlite3_open( ret->fullPath, &(ret->db) );
	if( rc != SQLITE_OK )
		goto newerr;
	
	return ret;
	
newerr:
	fprintf( stderr, "sqlite db error '%s'\n", sqlite3_errmsg( ret->db ) );
	free_vpddbenv( ret );
	return NULL;
}

void free_vpddbenv( struct vpddbenv *freeme )
{
	if( !freeme )
		return;
	
	if( freeme->db )
		sqlite3_close( freeme->db );
	
	free( freeme );
}

struct component* fetch_component( struct vpddbenv *db, const char *deviceID )
{
	struct component* ret = NULL;
	sqlite3_stmt *pstmt = NULL;
	int rc;
	const char *out;
	char sql[] = {"SELECT " DATA " FROM " TABLE_NAME " WHERE " ID "=?"};

	rc = SQLITE3_PREPARE( db->db, sql, sizeof( sql ), &pstmt, &out );
	if( rc != SQLITE_OK )
		goto FETCH_COMP_ERR;

	rc = sqlite3_bind_text(pstmt, 1, deviceID,
			       deviceID ? strlen(deviceID):0, SQLITE_STATIC);
	if (rc != SQLITE_OK)
		goto FETCH_COMP_ERR;

	rc = sqlite3_step( pstmt );
	if( rc != SQLITE_ROW && rc != SQLITE_DONE )
		goto FETCH_COMP_ERR;
	if( rc == SQLITE_ROW )
	{
		ret = unpack_component( (void *) sqlite3_column_blob( pstmt, 0 ) );
	}
	
	sqlite3_finalize( pstmt );
	return ret;
	
FETCH_COMP_ERR:
	fprintf( stderr, "Error fetching '%s': %s\n", deviceID,
			sqlite3_errmsg( db->db ) );
	if( pstmt )
		sqlite3_finalize( pstmt );
	return ret;
}

struct system* fetch_system( struct vpddbenv *db )
{
	struct system* ret = NULL;
	sqlite3_stmt *pstmt = NULL;
	int rc;
	const char *out;
	char sql[] = "SELECT " DATA " FROM " TABLE_NAME " WHERE " ID "='" SYS_ID "';";

	rc = SQLITE3_PREPARE( db->db, sql, sizeof( sql ), &pstmt, &out );
	if( rc != SQLITE_OK )
		goto FETCH_SYS_ERR;
	
	rc = sqlite3_step( pstmt );
	if( rc != SQLITE_ROW && rc != SQLITE_DONE )
		goto FETCH_SYS_ERR;
	
	if( rc == SQLITE_ROW )
	{
		ret = unpack_system( (void *) sqlite3_column_blob( pstmt, 0 ) );
	}
	
	sqlite3_finalize( pstmt );
	return ret;

FETCH_SYS_ERR:
	fprintf( stderr, "Error fetching '%s': %s\n", SYS_ID,
			sqlite3_errmsg( db->db ) );
	if( pstmt )
		sqlite3_finalize( pstmt );
	return ret;
}
