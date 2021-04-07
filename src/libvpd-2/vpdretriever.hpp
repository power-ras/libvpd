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

#ifndef LSVPDVPDRETRIEVER_HPP
#define LSVPDVPDRETRIEVER_HPP

#include <libvpd-2/component.hpp>
#include <libvpd-2/system.hpp>
#include <libvpd-2/vpddbenv.hpp>
#include <libvpd-2/vpdexception.hpp>

namespace lsvpd
{

	class VpdRetriever
	{
		private:
			VpdDbEnv* db;
			void buildSubTree( System* root );
			void buildSubTree( Component* root );

		public:
			static const string DEFAULT_DIR;
			static const string DEFAULT_FILE;
			static const string UDEV_NOTIFY_FILE;
			
			/**
			 * Builds A VpdRetriever object that can be used for reading the
			 * vpd stored in the specified database.  VpdExceptions
			 * mean that there was a problem with the necessary resources
			 * to access the database (e.g. The file was not present or was
			 * not readable by the user).  If any other exceptions come from
			 * this constructor, there were serious underlying issues that
			 * are not recoverable.
			 *
			 * @param envDir
			 *   The directory where the VPD database is stored.
			 * @param dbFileName
			 *   The file name for the VPD database.
			 */
			VpdRetriever( string envDir, string dbFileName );
			
			/**
			 * Builds A VpdRetriever object that can be used for reading the
			 * vpd stored in the specified database.  VpdExceptions
			 * mean that there was a problem with the necessary resources
			 * to access the database (e.g. The file was not present or was
			 * not readable by the user).  If any other exceptions come from
			 * this constructor, there were serious underlying issues that
			 * are not recoverable.  Uses the default dir and filename
			 */
			VpdRetriever( );
			~VpdRetriever( );

			/**
			 * Retrieves, builds, and returns a tree structure representing
			 * the VPD available in the target VPD database.  Each entry
			 * corresponds to a single device available on a system and all
			 * of the VPD for that device will be contained in the
			 * corresponding entry only.
			 *
			 * @return
			 *   The root of the tree of device VPD.
			 */
			System* getComponentTree( );

			/**
			 * Gets a specified Component from the database.  A Component is
			 * the collection of VPD about a single device on the system.
			 *
			 * NOTE: The pointer returned is "newed" by this method but the
			 * caller will be responsible for deleting the pointer that is
			 * returned.
			 *
			 * @param id
			 *   The string ID for the requested component.
			 * @return
			 *   A pointer to the requested information or NULL on failure.
			 */
			inline Component* getComponent( const string& id )
				{ return db->fetch( id ); }

			/**
			 * Gets the root or System Component from the database.  A
			 * System is the collection of VPD about the System.
			 *
			 * NOTE: The pointer returned is "newed" by this method but the
			 * caller will be responsible for deleting the pointer that is
			 * returned.
			 *
			 * @return
			 *   A pointer to the System VPD collection or NULL on failure.
			 */
			inline System* getComponent( ) { return db->fetch( ); }

	};

}

#endif /*LSVPDVPDRETRIEVER_HPP*/

