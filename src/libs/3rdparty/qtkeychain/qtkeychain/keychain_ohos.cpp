/******************************************************************************
 *   Copyright (C) 2026 The Qt Company Ltd.                                   *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/
#include "keychain_p.h"

#include <asset/asset_api.h>

#include <QObject>
#include <QString>

#include <iterator>

using namespace QKeychain;

/*
 * Backed by the OHOS asset store, which keeps credentials encrypted per
 * application. Entries are addressed by a single alias, so service and key are
 * joined; the job mode is kept in a label attribute, as the store itself only
 * knows about byte arrays.
 */

static const char modeText = '0';
static const char modeBinary = '1';

static QByteArray entryAlias( const QString &service, const QString &key )
{
    return service.toUtf8() + '/' + key.toUtf8();
}

static Asset_Attr blobAttr( uint32_t tag, QByteArray &value )
{
    Asset_Attr attr = {};
    attr.tag = tag;
    attr.value.blob.size = uint32_t( value.size() );
    attr.value.blob.data = reinterpret_cast<uint8_t *>( value.data() );
    return attr;
}

static Asset_Attr numberAttr( uint32_t tag, uint32_t value )
{
    Asset_Attr attr = {};
    attr.tag = tag;
    attr.value.u32 = value;
    return attr;
}

static const Asset_Attr *findAttr( const Asset_Result &result, uint32_t tag )
{
    for ( uint32_t i = 0; i < result.count; ++i ) {
        if ( result.attrs[i].tag == tag )
            return &result.attrs[i];
    }
    return nullptr;
}

static QString errorStringFor( int32_t code )
{
    return QObject::tr( "Asset store error %1" ).arg( code );
}

static Error errorFor( int32_t code )
{
    switch ( code ) {
    case ASSET_NOT_FOUND:
        return EntryNotFound;
    case ASSET_ACCESS_DENIED:
        return AccessDenied;
    default:
        return OtherError;
    }
}

void ReadPasswordJobPrivate::scheduledStart()
{
    QByteArray alias = entryAlias( q->service(), q->key() );

    const Asset_Attr query[] = {
        blobAttr( ASSET_TAG_ALIAS, alias ),
        numberAttr( ASSET_TAG_RETURN_TYPE, ASSET_RETURN_ALL )
    };

    Asset_ResultSet resultSet = {};
    const int32_t result = OH_Asset_Query( query, uint32_t( std::size( query ) ), &resultSet );

    if ( result != ASSET_SUCCESS ) {
        q->emitFinishedWithError( errorFor( result ),
                                  result == ASSET_NOT_FOUND ? tr( "Password not found" )
                                                            : errorStringFor( result ) );
        return;
    }

    if ( resultSet.count == 0 ) {
        OH_Asset_FreeResultSet( &resultSet );
        q->emitFinishedWithError( EntryNotFound, tr( "Password not found" ) );
        return;
    }

    const Asset_Attr * const secret = findAttr( resultSet.results[0], ASSET_TAG_SECRET );
    if ( !secret ) {
        OH_Asset_FreeResultSet( &resultSet );
        q->emitFinishedWithError( OtherError, tr( "The entry carries no secret" ) );
        return;
    }

    data = QByteArray( reinterpret_cast<const char *>( secret->value.blob.data ),
                       int( secret->value.blob.size ) );

    const Asset_Attr * const label = findAttr( resultSet.results[0],
                                               ASSET_TAG_DATA_LABEL_NORMAL_1 );
    mode = ( label && label->value.blob.size == 1 && label->value.blob.data[0] == modeBinary )
               ? Binary : Text;

    OH_Asset_FreeResultSet( &resultSet );
    q->emitFinished();
}

void WritePasswordJobPrivate::scheduledStart()
{
    QByteArray alias = entryAlias( q->service(), q->key() );
    QByteArray secret = data;
    QByteArray label( 1, mode == Binary ? modeBinary : modeText );

    const Asset_Attr attributes[] = {
        blobAttr( ASSET_TAG_ALIAS, alias ),
        blobAttr( ASSET_TAG_SECRET, secret ),
        blobAttr( ASSET_TAG_DATA_LABEL_NORMAL_1, label ),
        numberAttr( ASSET_TAG_ACCESSIBILITY, ASSET_ACCESSIBILITY_DEVICE_FIRST_UNLOCKED )
    };

    int32_t result = OH_Asset_Add( attributes, uint32_t( std::size( attributes ) ) );

    if ( result == ASSET_DUPLICATED ) {
        // Adding refuses to overwrite, so update the existing entry in place.
        const Asset_Attr query[] = { blobAttr( ASSET_TAG_ALIAS, alias ) };
        const Asset_Attr updated[] = {
            blobAttr( ASSET_TAG_SECRET, secret ),
            blobAttr( ASSET_TAG_DATA_LABEL_NORMAL_1, label )
        };
        result = OH_Asset_Update( query, uint32_t( std::size( query ) ),
                                  updated, uint32_t( std::size( updated ) ) );
    }

    if ( result != ASSET_SUCCESS ) {
        q->emitFinishedWithError( errorFor( result ), errorStringFor( result ) );
        return;
    }

    q->emitFinished();
}

void DeletePasswordJobPrivate::scheduledStart()
{
    QByteArray alias = entryAlias( q->service(), q->key() );
    const Asset_Attr query[] = { blobAttr( ASSET_TAG_ALIAS, alias ) };

    const int32_t result = OH_Asset_Remove( query, uint32_t( std::size( query ) ) );

    if ( result == ASSET_NOT_FOUND ) {
        q->emitFinishedWithError( EntryNotFound, tr( "Password not found" ) );
        return;
    }

    if ( result != ASSET_SUCCESS ) {
        q->emitFinishedWithError( CouldNotDeleteEntry, errorStringFor( result ) );
        return;
    }

    q->emitFinished();
}

bool QKeychain::isAvailable()
{
    return true;
}
