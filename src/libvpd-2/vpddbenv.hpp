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
#ifndef LSVPDVPDDBENV_HPP
#define LSVPDVPDDBENV_HPP

#include <string>
#include <vector>
#include <sqlite3.h>

#include <libvpd-2/component.hpp>
#include <libvpd-2/system.hpp>
#include <libvpd-2/vpdexception.hpp>

#if HAVE_SQLITE3_PREPARE_V2
#define SQLITE3_PREPARE sqlite3_prepare_v2
#else
#define SQLITE3_PREPARE sqlite3_prepare
#endif

using namespace std;

namespace lsvpd
{

	/**
	 * VpdDbEnv provides the interface for all of lsvpd to interact with the
	 * Berkeley DB that is storing all the VPD information.  This class
	 * enforces the use of an BDB environment to give us multi-process saftey
	 * and transaction support.
	 */
	class VpdDbEnv
	{
		public:
			class UpdateLock {
				protected:
					friend VpdDbEnv;
					bool mReadOnly;
					string mDbFileName;
					string mEnvDir;
					string mDbPath;
				private:
					int lockfd;

					UpdateLock( const UpdateLock& copyMe ) = delete;
					UpdateLock& operator=( const UpdateLock& rhs ) = delete;

					pid_t lockFileUpdate( bool blocking );
					void unlockFileUpdate( void );
				public:
					static const string UPDATE_LOCK_SUFFIX;

					UpdateLock( const string& envDir, const string& dbFileName,
						bool readOnly );
					~UpdateLock();
			};
		private:
			VpdDbEnv& operator=( const VpdDbEnv& rhs ) = delete;
			VpdDbEnv( const VpdDbEnv& copyMe ) = delete;
			void initFromLock( void );

			const UpdateLock &mUpdateLock;
			string mDbFileName;
			string mEnvDir;
			string mDbPath;
			sqlite3* mpVpdDb;

		public:
			// Table name for the components
			static const string TABLE_NAME;
			static const string ID;
			static const string DATA;

			VpdDbEnv( const string& envDir, const string& dbFileName,
						bool readOnly );
			VpdDbEnv( const VpdDbEnv::UpdateLock&);
			~VpdDbEnv();

			/**
			 * Fetch attempts to load the specified Component from the VPD
			 * database.  If the Component is not in the database, the returned
			 * object will have empty data fields.  A VpdException will only be
			 * thrown if there is an unrecoverable problem communicating with
			 * the database.
			 *
			 * @param deviceID
			 *   The ID for the device information to retrieve from the db
			 * @returns
			 *   The collection of device information
			 * @throws VpdException
			 *   In the event of an unrecoverable error (e.g. new returns NULL,
			 * database is corrupt, etc.) a VpdException will be thrown.
			 */
			Component* fetch( const string& deviceID );

			/**
			 * Fetch attempts to load the root System from the VPD database.
			 * If the System is not in the database, the returned object will
			 * have empty data fields.  A VpdException will only be thrown if
			 * there is an unrecoverable problem communicating with the
			 * database.
			 *
			 * @returns
			 *   The collection of system information
			 * @throws VpdException
			 *   In the event of an unrecoverable error (e.g. new returns NULL,
			 * database is corrupt, etc.) a VpdException will be thrown.
			 */
			System* fetch( );

			/**
			 * Store attempts to save the specified Component to the VPD
			 * database.  If the save is not successfull, store will return
			 * false.  A VpdException will only be thrown if there is an
			 * unrecoverable problem communicating with the database.
			 *
			 * @param storeMe
			 *   The device information to store into the db
			 * @returns
			 *  true is everything succeeded, fales otherwise
			 * @throws VpdException
			 *   In the event of an unrecoverable error (e.g. new returns NULL,
			 * database is corrupt, etc.) a VpdException will be thrown.
			 */
			bool store( Component *storeMe );

			/**
			 * Store attempts to save the specified System to the VPD database.
			 * If the save is not successfull, store will return false.  A
			 * VpdException will only be thrown if there is an unrecoverable
			 * problem communicating with the database.
			 *
			 * @param storeMe
			 *   The system information to store into the db
			 * @returns
			 *  true is everything succeeded, fales otherwise
			 * @throws VpdException
			 *   In the event of an unrecoverable error (e.g. new returns NULL,
			 * database is corrupt, etc.) a VpdException will be thrown.
			 */
			bool store( System *storeMe );

			/**
			 * Remove deletes the specified Component from the VPD database, if
			 * the delete is unsuccessfull remove will return false.  A
			 * VpdException will only be thrown if there is an unrecoverable
			 * problem communicating with the database.
			 *
			 * @param deviceID
			 *   The ID for the information to remove from the db
			 * @returns
			 *   true if deletion succeeded, otherwise false
			 * @throws VpdException
			 *   In the event of an unrecoverable error (e.g. new returns NULL,
			 * database is corrupt, etc.) a VpdException will be thrown.
			 */
			bool remove( const string& deviceID );

			/**
			 * getKeys returns all of the valid keys (device ID's) present in
			 * the VPD database, if there is a problem retrieving the keys the
			 * vector will be empty or incomplete.
			 *
			 * @returns
			 *   A vector containing all of the device ID's in the db
			 * @throws VpdException
			 *   In the event of an unrecoverable error (e.g. new returns NULL,
			 * database is corrupt, etc.) a VpdException will be thrown.
			 */
			vector<string> getKeys( );
	};
}
#endif
