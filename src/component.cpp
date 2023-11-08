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

#include <arpa/inet.h>
#include <netinet/in.h>

#include <libvpd-2/component.hpp>
#include <libvpd-2/lsvpd.hpp>
#include <libvpd-2/vpdexception.hpp>
#include <libvpd-2/debug.hpp>
#include <libvpd-2/logger.hpp>
#include <libvpd-2/helper_functions.hpp>

#include <cstring>

namespace lsvpd
{
	string const Component::CHILD_START   ( "::childrenStart::" );
	string const Component::CHILD_END     ( "::childrenEnd::" );
	string const Component::DEVICE_START  ( "::deviceSpecificStart::" );
	string const Component::DEVICE_END    ( "::deviceSpecificEnd::" );
	string const Component::USER_START    ( "::userStart::" );
	string const Component::USER_END      ( "::userEnd::" );
	string const Component::AX_START      ( "::axStart::" );
	string const Component::AX_END        ( "::axEnd::" );

	Component::Component()
	{
		idNode.ac = "None";
		idNode.humanName = "Main Device Node, equals sysFsNode or deviceTreeNode";
		deviceTreeNode.ac = "None";
		deviceTreeNode.humanName = "/proc/device-tree Device Node";
		sysFsNode.ac = "None";
		sysFsNode.humanName = "/sys Device Node";
		sysFsLinkTarget.ac = "None";
		sysFsLinkTarget.humanName = "/sys/bus Device Node";
		mDevClass.ac = "None";
		mDevClass.humanName = "/sys/class - Device Node";
		mFRU.ac = "FN";
		mFRU.humanName = "Field Replaceable Unit Number";
		mParent.ac = "Parent Node";
		mParent.humanName = "Parent Node";
		devSysName.humanName = "Device name from sysFS";
		devDevTreeName = "Device name from /proc/device-tree";

		scsiDetail.ac = "ZZ";
		scsiDetail.humanName = "Device Details";
		plantMfg.ac = "SE";
		plantMfg.humanName = "Plant of manufacture";
		mNetAddr.ac = "NA";
		mNetAddr.humanName = "Network Address";
		mDescription.ac = "DS";
		mDescription.humanName = "Displayable Message";
		mCDField.ac = "CD";
		mCDField.humanName = "Card ID";
		mSerialNumber.ac = "SN";
		mSerialNumber.humanName = "Serial Number";
		mPartNumber.ac = "PN";
		mPartNumber.humanName = "Part Number of assembly";
		// For RL: Special rule applies.  if devBus == "disk", is must be stored
		// as Hex. See device_scsi_append_field() in lsvpd-0.16
		mFirmwareLevel.ac = "RL";
		mFirmwareLevel.humanName = "Non-alterable ROM level";
		mFirmwareVersion.ac = "RM";
		mFirmwareVersion.humanName = "Alterable ROM Level";
		mManufacturer.ac = "MF";
		mManufacturer.humanName = "Manufacturer Name";
		mModel.ac = "TM";
		mModel.humanName = "Machine Type-Model";
		mManufacturerID.ac = "MN";
		mManufacturerID.humanName = "Manufacturer ID";
		mEngChangeLevel.ac = "EC";
		mEngChangeLevel.humanName = "Engineering Change Level";
		mFeatureCode.ac = "FC";
		mFeatureCode.humanName = "Feature Code or Request for Price Quotation (RPQ) number";
		devDriver.ac = "DD";
		devDriver.humanName = "Device Driver Level";
		mRecordType.ac = "RT";
		mRecordType.humanName = "Record Type";
		mKeywordVersion.ac = "VK";
		mKeywordVersion.humanName = "Keyword Version";
		mMicroCodeImage.ac = "MI";
		mMicroCodeImage.humanName = "Micro Code Image";
		mPhysicalLocation.ac = "YL";
		mPhysicalLocation.humanName = "Location Code";
		mSecondLocation.ac = "YL";
		mSecondLocation.humanName = "Location Code";
		n5.ac = "N5";
		n5.humanName = "Processor CoD Capacity Card Info";
		n6.ac = "N6";
		n6.humanName = "Memory CoD Capacity Card Info";

		devState = COMPONENT_STATE_LIVE;

		devBus.humanName = "Device Bus";

		mpParent = NULL;

		mDeviceSpecific = vector<DataItem*>( );
		mUserData = vector<DataItem*>( );
		mAIXNames = vector<DataItem*>( );
		mChildren = vector<string>( );
		mLeaves = vector<Component*>( );
	}

	Component::Component( const Component& copyMe )
	{
		copyToMe( copyMe );
	}

	Component::Component( const void* packedData )
	{
		unpack( packedData );
		mLeaves = vector<Component*>( );
	}

	Component::~Component( )
	{
		vector<Component*>::iterator i, end = mLeaves.end( );
		for( i = mLeaves.begin( ); i != end; ++i )
		{
			delete (*i);
		}

		vector<DataItem*>::iterator j, dEnd;
		for( j = mDeviceSpecific.begin( ), dEnd = mDeviceSpecific.end( ); j != dEnd; ++j )
		{
			delete (*j);
		}

		for( j = mUserData.begin( ), dEnd = mUserData.end( ); j != dEnd; ++j )
		{
			delete (*j);
		}

		for( j = mAIXNames.begin( ), dEnd = mAIXNames.end( ); j != dEnd; ++j )
		{
			delete (*j);
		}
	}

	Component& Component::operator=( const Component& rhs )
	{
		copyToMe( rhs );
		return (*this);
	}

	unsigned int Component::getPackedSize( )
	{
		unsigned int ret = INIT_BUF_SIZE;

		ret += idNode.getPackedLength( );
		ret += deviceTreeNode.getPackedLength( );
		ret += sysFsNode.getPackedLength( );
		ret += sysFsLinkTarget.getPackedLength( );
		ret += halUDI.getPackedLength( );
		ret += mNetAddr.getPackedLength( );
		ret += mDevClass.getPackedLength( );
		ret += mDescription.getPackedLength( );
		ret += mCDField.getPackedLength( );
		ret += mSerialNumber.getPackedLength( );
		ret += mPartNumber.getPackedLength( );
		ret += mFirmwareLevel.getPackedLength( );
		ret += mFirmwareVersion.getPackedLength( );
		ret += mFRU.getPackedLength( );
		ret += mManufacturer.getPackedLength( );
		ret += mModel.getPackedLength( );
		ret += mManufacturerID.getPackedLength( );
		ret += mEngChangeLevel.getPackedLength( );
		ret += mParent.getPackedLength( );
		ret += devSubsystem.getPackedLength( );
		ret += devDriver.getPackedLength( );
		ret += devKernel.getPackedLength( );
		ret += devKernelNumber.getPackedLength( );
		ret += devSysName.getPackedLength( );
		ret += devDevTreeName.getPackedLength( );
		ret += devBus.getPackedLength( );
		ret += devBusAddr.getPackedLength( );
		ret += mRecordType.getPackedLength( );
		ret += scsiDetail.getPackedLength( );
		ret += plantMfg.getPackedLength( );
		ret += mFeatureCode.getPackedLength( );
		ret += mKeywordVersion.getPackedLength( );
		ret += mMicroCodeImage.getPackedLength( );
		ret += mSecondLocation.getPackedLength( );
		ret += n5.getPackedLength( );
		ret += n6.getPackedLength( );

		/* Must be last single item packed */
		ret += mPhysicalLocation.getPackedLength( );

		ret += mDeviceSpecific.size( );
		ret += mUserData.size( );
		ret += mChildren.size( );
		ret += mAIXNames.size( );
		vector<string>::iterator child, cEnd;
		vector<DataItem*>::iterator item, dEnd;
		for( child = mChildren.begin( ), cEnd = mChildren.end( ); child != cEnd;
					++child )
		{
			ret += (*child).length( );
		}
		for( item = mDeviceSpecific.begin( ), dEnd = mDeviceSpecific.end( );
					item != dEnd;
				   ++item )
		{
			ret += (*item)->getPackedLength( );
		}

		for( item = mUserData.begin( ), dEnd = mUserData.end( ); item != dEnd;
				   ++item )
		{
			ret += (*item)->getPackedLength( );
		}

		for( item = mAIXNames.begin( ), dEnd = mAIXNames.end( ); item != dEnd;
				   ++item )
		{
			ret += (*item)->getPackedLength( );
		}

		ret += sizeof( u32 );

		return ret;
	}

	/**
	 * pack serializes this object into the provided buffer (pack will
	 *	allocate the buffer) storing only the data fields (at the moment)
	 *	and ignoring all the meta-data within the DataItem object.
	 *	The format for the packed object will be direct strings seperated
	 *	by '\0' charaters.  If a given string field is empty then there will
	 *	be consectutive '\0' characters.  Each of the vector fields will be
	 *	preceeded by ::listNameStart:: and followed by ::listNameEnd::
	 *	(e.g. the children vector will be
	 *		::childrenStart::\0data\0data\0::childrenEnd::\0).
	 */
	unsigned int Component::pack( void** buffer )
	{
		u32 ret = getPackedSize( ), length;
		char* buf;

		buf = new char[ ret ];
		if( buf == NULL )
		{
			string message( "Component.pack( ): new call failed, out of memory." );
			Logger l;
			l.log( message, LOG_ERR );
			VpdException ve( message );
			throw ve;
		}

		memset( buf, '\0', ret );

		*buffer = (void*)buf;

		/* -----------------------------------------------------*/
		/* ------------ Load up the buffer with data -----------*/
		/* -----------------------------------------------------*/

		// The first entry in our buffer is our length in network byte order
		u32 netOrder = htonl( ret );
		memcpy( buf, &netOrder, sizeof( u32 ) );
		buf += sizeof( u32 );
		// Pack the individual data items.
		buf += idNode.pack( buf );
		buf += deviceTreeNode.pack( buf );
		buf += sysFsNode.pack( buf );
		buf += sysFsLinkTarget.pack( buf );
		buf += halUDI.pack( buf );
		buf += mNetAddr.pack( buf );
		buf += mDevClass.pack( buf );
		buf += mDescription.pack( buf );
		buf += mCDField.pack( buf );
		buf += mSerialNumber.pack( buf );
		buf += mPartNumber.pack( buf );
		buf += mFirmwareLevel.pack( buf );
		buf += mFirmwareVersion.pack( buf );
		buf += mFRU.pack( buf );
		buf += mManufacturer.pack( buf );
		buf += mModel.pack( buf );
		buf += mManufacturerID.pack( buf );
		buf += mEngChangeLevel.pack( buf );
		buf += mParent.pack( buf );
		buf += devSubsystem.pack( buf );
		buf += devDriver.pack( buf );
		buf += devKernel.pack( buf );
		buf += devKernelNumber.pack( buf );
		buf += devSysName.pack( buf );
		buf += devDevTreeName.pack( buf );
		buf += devBus.pack( buf );
		buf += devBusAddr.pack( buf );
		buf += mRecordType.pack( buf );
		buf += scsiDetail.pack( buf );
		buf += n5.pack( buf );
		buf += n6.pack( buf );

		// Added - SCSI fill
		buf += plantMfg.pack( buf );
		buf += mFeatureCode.pack( buf );

		buf += mKeywordVersion.pack( buf );
		buf += mMicroCodeImage.pack( buf );
		buf += mSecondLocation.pack( buf );

		// Must be last single item to be packed
		buf += mPhysicalLocation.pack( buf );

		// Pack the child vector.
		memcpy( buf, CHILD_START.c_str( ), CHILD_START.length( ) );
		buf += CHILD_START.length( );
		*buf = '\0';
		buf++;

		vector<string>::iterator child, cEnd;
		vector<DataItem*>::iterator item, dEnd;
		for( child = mChildren.begin( ), cEnd = mChildren.end( ); child != cEnd;
					++child )
		{
			const char* str = (*child).c_str( );
			int length = (*child).length( );
			memcpy( buf, str, length );
			buf+= length;
			*buf = '\0';
			buf++;
		}
		memcpy( buf, CHILD_END.c_str( ), CHILD_END.length( ) );
		buf += CHILD_END.length( );
		*buf = '\0';
		buf++;

		memcpy( buf, DEVICE_START.c_str( ), DEVICE_START.length( ) );
		buf += DEVICE_START.length( );
		*buf = '\0';
		buf++;
		for( item = mDeviceSpecific.begin( ), dEnd = mDeviceSpecific.end( );
					item != dEnd;
				   ++item )
		{
			length = (*item)->pack( buf );
			buf+= length;
		}
		memcpy( buf, DEVICE_END.c_str( ), DEVICE_END.length( ) );
		buf += DEVICE_END.length( );
		*buf = '\0';
		buf++;

		memcpy( buf, USER_START.c_str( ), USER_START.length( ) );
		buf += USER_START.length( );
		*buf = '\0';
		buf++;
		for( item = mUserData.begin( ), dEnd = mUserData.end( ); item != dEnd;
				   ++item )
		{
			length = (*item)->pack( buf );
			buf+= length;
		}
		memcpy( buf, USER_END.c_str( ), USER_END.length( ) );
		buf += USER_END.length( );
		*buf = '\0';
		buf++;

		memcpy( buf, AX_START.c_str( ), AX_START.length( ) );
		buf += AX_START.length( );
		*buf = '\0';
		buf++;
		for( item = mAIXNames.begin( ), dEnd = mAIXNames.end( ); item != dEnd;
				   ++item )
		{
			length = (*item)->pack( buf );
			buf+= length;
		}
		memcpy( buf, AX_END.c_str( ), AX_END.length( ) );
		buf += AX_END.length( );
		*buf = '\0';

		return ret;
	}

	/**
	 * unpack does exactly the opposite from pack, it will take a data buffer
	 * and load this object with the contents.  If we need more data than the
	 * size claims that we have, then we have a corrupt buffer and we will
	 * throw an exception.
	 */
	void Component::unpack( const void* payload )
	{
		u32 size = 0, netOrder;
		char* packed = (char*) payload;
		char* next;
		string item;

		if( payload == NULL )
		{
			/* Rather than throwing an exception, this will just return,
			 if NULL is handed in this far there is nothing that we can do
			 to stop program instability other than refuse to mess with it.
			 */
			return;
		}

		// Load the size of the payload. (It is packed in network order)
		memcpy( &netOrder, payload, sizeof( u32 ) );
		size = ntohl( netOrder );

		next = packed + sizeof( u32 );
		mChildren = vector<string>( );
		mDeviceSpecific = vector<DataItem*>( );

		idNode = DataItem( next );
		next += idNode.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		deviceTreeNode = DataItem( next );
		next += deviceTreeNode.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		sysFsNode = DataItem( next );
		next += sysFsNode.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		sysFsLinkTarget = DataItem( next );
		next += sysFsLinkTarget.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		halUDI = DataItem( next );
		next += halUDI.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}

		mNetAddr = DataItem( next );
		next += mNetAddr.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mDevClass = DataItem( next );
		next += mDevClass.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mDescription = DataItem( next );
		next += mDescription.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mCDField = DataItem( next );
		next += mCDField.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}

		mSerialNumber = DataItem( next );
		next += mSerialNumber.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mPartNumber = DataItem( next );
		next += mPartNumber.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mFirmwareLevel = DataItem( next );
		next += mFirmwareLevel.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mFirmwareVersion = DataItem( next );
		next += mFirmwareVersion.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mFRU = DataItem( next );
		next += mFRU.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mManufacturer = DataItem( next );
		next += mManufacturer.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mModel = DataItem( next );
		next += mModel.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mManufacturerID = DataItem( next );
		next += mManufacturerID.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mEngChangeLevel = DataItem( next );
		next += mEngChangeLevel.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mParent = DataItem( next );
		next += mParent.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		devSubsystem = DataItem( next );
		next += devSubsystem.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		devDriver = DataItem( next );
		next += devDriver.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		devKernel = DataItem( next );
		next += devKernel.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		devKernelNumber = DataItem( next );
		next += devKernelNumber.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		devSysName = DataItem( next );
		next += devSysName.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		devDevTreeName = DataItem( next );
		next += devDevTreeName.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		devBus = DataItem( next );
		next += devBus.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		devBusAddr = DataItem( next );
		next += devBusAddr.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mRecordType = DataItem( next );
		next += mRecordType.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		scsiDetail = DataItem( next );
		next += scsiDetail.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		n5 = DataItem( next );
		next += n5.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		n6 = DataItem( next );
		next += n6.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		/* Added - From SCSI fill */
		plantMfg = DataItem( next );
		next += plantMfg.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mFeatureCode = DataItem( next );
		next += mFeatureCode.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mKeywordVersion = DataItem( next );
		next += mKeywordVersion.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mMicroCodeImage = DataItem( next );
		next += mMicroCodeImage.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mSecondLocation = DataItem( next );
		next += mSecondLocation.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}
		mPhysicalLocation = DataItem( next );
		next += mPhysicalLocation.getPackedLength( );
		if( next > packed + size )
		{
			goto lderr;
		}

		// Now onto the vectors, these should go much the same way that the
		//  packing did.
		while( next != CHILD_START )
		{
			next++;
			if( next > packed + size )
			{
				goto lderr;
			}
		}

		if( next == CHILD_START )
		{
			next += CHILD_START.length( ) + 1;
			if( next > packed + size )
			{
				goto lderr;
			}
			item = string( (char*)next );
			while( CHILD_END != item )
			{
				if( item != string( "" ) )
				{
					string child = string( (char*)next );
					mChildren.push_back( child );
					next += item.length( );
				}
				next++;
				item = string( (char*)next );
			}
			next += CHILD_END.length( ) + 1;
		}

		if( DEVICE_START == next )
		{
			next += DEVICE_START.length( ) + 1;
			if( next > packed + size )
			{
				goto lderr;
			}
			item = string( (char*) next );
			while( DEVICE_END != item )
			{
				DataItem* d = new DataItem( next );
				if ( d == NULL ) {
					VpdException ve( "Out of memory." );
					throw ve;
				}
				next += d->getPackedLength( );
				if( next > packed + size )
				{
					delete ( d );
					goto lderr;
				}
				mDeviceSpecific.push_back( d );
				item = string( (char*)next );
			}

			next += DEVICE_END.length( ) + 1;
		}

		if( USER_START == next )
		{
			next += USER_START.length( ) + 1;
			if( next > packed + size )
			{
				goto lderr;
			}
			item = string( (char*) next );
			while( USER_END != item )
			{
				DataItem* d = new DataItem( next );
				if ( d == NULL ) {
					VpdException ve( "Out of memory." );
					throw ve;
				}
				next += d->getPackedLength( );
				if( next > packed + size )
				{
					delete ( d );
					goto lderr;
				}
				mUserData.push_back( d );
				item = string( (char*)next );
			}
			next += USER_END.length( ) + 1;
		}

		if( AX_START == next )
		{
			next += AX_START.length( ) + 1;
			if( next > packed + size )
			{
				goto lderr;
			}
			item = string( (char*) next );
			while( AX_END != item )
			{
				DataItem* d = new DataItem( next );
				if ( d == NULL ) {
					VpdException ve( "Out of memory." );
					throw ve;
                                }
				next += d->getPackedLength( );
				if( next > packed + size )
				{
					delete ( d );
					goto lderr;
				}
				mAIXNames.push_back( d );
				item = string( (char*)next );
			}
		}

		return;

lderr:
		string message(
			"Component.unpack( ): Attempting to unpack corrupt buffer." );
		Logger l;
		l.log( message, LOG_ERR );
		VpdException ve( message );
		throw ve;
	}

	/**
	 * Internal use method only.  This method houses the logic that would
	 * otherwise be repeated in the copy constructor and the operator=
	 * methods.
	 */
	void Component::copyToMe( const Component& copyMe )
	{
		mChildren = vector<string>( copyMe.mChildren );

		vector<DataItem*>::const_iterator i, dEnd;
		vector<Component*>::const_iterator j, cEnd;

		for( i = copyMe.mDeviceSpecific.begin( ),
			dEnd = copyMe.mDeviceSpecific.end( ); i != dEnd; ++i )
		{
			mDeviceSpecific.push_back( (*i) );
		}

		for( i = copyMe.mUserData.begin( ),
			dEnd = copyMe.mUserData.end( ); i != dEnd; ++i )
		{
			mUserData.push_back( (*i) );
		}

		for( i = copyMe.mAIXNames.begin( ),
			dEnd = copyMe.mAIXNames.end( ); i != dEnd; ++i )
		{
			mAIXNames.push_back( (*i) );
		}

		for( j = copyMe.mLeaves.begin( ),
			cEnd = copyMe.mLeaves.end( ); j != cEnd; ++j )
		{
			mLeaves.push_back( (*j) );
		}

		mDescription = DataItem( copyMe.mDescription );
		mSerialNumber = DataItem( copyMe.mSerialNumber );
		mPartNumber = DataItem( copyMe.mPartNumber );
		mFRU = DataItem( copyMe.mFRU );
		mFirmwareLevel = DataItem( copyMe.mFirmwareLevel );
		mFirmwareVersion = DataItem( copyMe.mFirmwareVersion );
		mManufacturer = DataItem( copyMe.mManufacturer );
		mModel = DataItem( copyMe.mModel );
		mEngChangeLevel = DataItem( copyMe.mEngChangeLevel );
		mParent = DataItem( copyMe.mParent );
		idNode = DataItem( copyMe.idNode );
		mPhysicalLocation = DataItem( copyMe.mPhysicalLocation );
		mSecondLocation = DataItem( copyMe.mSecondLocation );
		mFeatureCode = DataItem( copyMe.mFeatureCode );
		/*
		 * plantMfg = DataItem( copyMe.plantMfg );
		 */
	}

	/* Children Manipulation Section */

	void Component::addChild(const string& in )
	{
		mChildren.push_back(in);
	}

	/**
	 * @brief
	 *   Walk thru list of children of this component, checking to see
	 *   if the passed-in component is a child of this
	 * @param child
	 *   The component which may or may not be a child of this
	 */
	bool Component::isChild(Component *child)
	{
		for (unsigned int i = 0; i < mChildren.size(); i++)
			if (mChildren[i] == child->idNode.getValue())
				return true;
		return false;
	}

	/**
	 * @brief
	 *   Walk thru list of children of this component, checking to see
	 *   if the passed-in component is a child of this
	 * @param child
	 *   The component which may or may not be a child of this
	 */
	bool Component::isChild(const string& child)
	{
		for (unsigned int i = 0; i < mChildren.size(); i++)
			if (mChildren[i] == child)
				return true;
		return false;
	}

	/**
	 * @param idNode
	 *   idNode of the child you are searching for
	 */
	const string& Component::getChild( int i ) const
	{
		return mChildren[i];
	}

	int Component::numChildren()
	{
		return mChildren.size();
	}

	void Component::removeChild( const string& id )
	{
		vector<string>::iterator i, end;

		for( i = mChildren.begin( ), end = mChildren.end( ); i != end; ++i )
		{
			if( id == *i )
			{
				mChildren.erase(i);
				return;
			}
		}
	}

	/**
	 * getDevClass
	 * @brief Returns the class node name - ie /sys/class/net/eth0
	 * would return net
	 */
	const string Component::getDevClass( )
	{
		return HelperFunctions::parsePath(mDevClass.getValue(), 2);
	}

	/* Finds and returns the DataItem specified by the acronym field, AC */
	const DataItem * Component::getDeviceSpecific(const string& itemAC )
	{
		vector<DataItem*>::iterator i, end;

		for( i = mDeviceSpecific.begin( ), end = mDeviceSpecific.end( ); i != end; ++i )
		{
			if( (*i)->ac == itemAC ) {
				return *i;
			}
		}

		return NULL;
	}

	void Component::addDeviceSpecific( const string& ac,
		const string& humanName, const string& val, int lvl = 0 )
	{
		DataItem *d = new DataItem( );
		vector<DataItem*>::iterator i, end;

		if( d == NULL )
		{
			VpdException ve( "Out of memory." );
			throw ve;
		}
		d->ac = ac;
		d->humanName = humanName;
		d->setValue( val, lvl, __FILE__, __LINE__ );

		/*
		 * There can be multiple records with the 'CL' key.
		 * e.g., system firmware has different values for indicating
		 * the levels of Phyp, FSP etc.
		 * So do not check if we already have the record.
		 */

		for( i = mDeviceSpecific.begin( ), end = mDeviceSpecific.end( );
			i != end; ++i )
		{
			if( (*i)->ac == ac )
				return; //Failed to add - already present
		}

		mDeviceSpecific.push_back( d );
	}

	void Component::updateDeviceSpecific( const string& ac,
		const string& humanName, const string& val, int lvl = 0 )
	{
		vector<DataItem*>::iterator i, end;

		for ( i = mDeviceSpecific.begin( ), end = mDeviceSpecific.end( );
			i != end; ++i )
		{
			if ( (*i)->ac == ac ) {
				(*i)->setValue(val, lvl, __FILE__, __LINE__ );
				return;
			}
		}

		/* We didn't find the entry, so add it */
		addDeviceSpecific(ac, humanName, val, lvl);
	}

	const string* Component::getMicroCodeLevel( )
	{
		const DataItem *d = getDeviceSpecific("ML");

		if ( d != NULL )
			return &((*d).getValue());
		else
			return NULL;
	}

	void Component::addUserData( const string& ac, const string& humanName,
		const string& val, int prefLvl = 1, bool clobber = false )
	{
		vector<DataItem*>::iterator i, end;
		for( i = mUserData.begin( ), end = mUserData.end( ); i != end; ++i )
		{
			if( (*i)->ac == ac )
			{
				if( clobber )
				{
					(*i)->dataValue = val;
				}
				return;
			}
		}
		DataItem *d = new DataItem( );
		if( d == NULL )
		{
			VpdException ve( "Out of memory." );
			throw ve;
		}

		d->ac = ac;
		d->humanName = humanName;
		d->setValue( val, prefLvl, __FILE__, __LINE__ );
		mUserData.push_back( d );
	}

	void Component::addAIXName( const string& val, int prefLvl = 1 )
	{
		vector<DataItem*>::const_iterator j, stop;
		j = getAIXNames().begin();
		stop =getAIXNames().end();

		/* Already present - remove */
		for (;j != stop; j++) {
			if (val == (*j)->getValue())
				return;
		}

		DataItem *d = new DataItem( );
		if( d == NULL )
		{
			VpdException ve( "Out of memory." );
			throw ve;
		}

		d->ac = "AX";
		d->humanName = "AIX Name";
		d->setValue( val, prefLvl, __FILE__, __LINE__ );
		mAIXNames.push_back( d );
	}

	/*
	 * In-memory tree representation methods
	 */
	bool Component::removeLeaf( int index )
	{
		vector<Component*>::iterator i = mLeaves.begin( );
		if( index > ( int )mLeaves.size( ) )
		{
			return false;
		}

		for( int j = 0; j < index; j++ )
		{
			i++;
		}
		mLeaves.erase( i );
		return true;
	}

	bool Component::removeLeaf( const string& in )
	{
		vector<Component*>::iterator i = mLeaves.begin( );

		for( ; i != mLeaves.end( ); ++i )
		{
			if( (*i)->getID( ) == in )
			{
				mLeaves.erase( i );
				return true;
			}
		}
		return false;
	}

}
