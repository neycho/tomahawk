/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RecentlyPlayedModel.h"

#include <QMimeData>
#include <QTreeView>

#include "source.h"
#include "sourcelist.h"
#include "database/database.h"
#include "database/databasecommand_playbackhistory.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#define HISTORY_TRACK_ITEMS 25

using namespace Tomahawk;


RecentlyPlayedModel::RecentlyPlayedModel( const source_ptr& source, QObject* parent )
    : PlaylistModel( parent )
    , m_source( source )
    , m_limit( HISTORY_TRACK_ITEMS )
{
    if ( source.isNull() )
    {
        onSourcesReady();

        connect( SourceList::instance(), SIGNAL( ready() ), SLOT( onSourcesReady() ) );
        connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );
    }
    else
    {
        onSourceAdded( source );
        loadHistory();
    }
}


RecentlyPlayedModel::~RecentlyPlayedModel()
{
}


void
RecentlyPlayedModel::loadHistory()
{
    if ( rowCount( QModelIndex() ) )
    {
        clear();
    }

    DatabaseCommand_PlaybackHistory* cmd = new DatabaseCommand_PlaybackHistory( m_source );
    cmd->setLimit( m_limit );

    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ),
                    SLOT( append( QList<Tomahawk::query_ptr> ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
RecentlyPlayedModel::onSourcesReady()
{
    Q_ASSERT( m_source.isNull() );

    loadHistory();

    foreach ( const source_ptr& source, SourceList::instance()->sources() )
        onSourceAdded( source );
}


void
RecentlyPlayedModel::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source.data(), SIGNAL( playbackFinished( Tomahawk::query_ptr ) ), SLOT( onPlaybackFinished( Tomahawk::query_ptr ) ), Qt::UniqueConnection );
}


void
RecentlyPlayedModel::onPlaybackFinished( const Tomahawk::query_ptr& query )
{
    int count = trackCount();
    unsigned int playtime = query->playedBy().second;

    if ( count )
    {
        TrackModelItem* oldestItem = itemFromIndex( index( count - 1, 0, QModelIndex() ) );
        if ( oldestItem->query()->playedBy().second >= playtime )
            return;

        TrackModelItem* youngestItem = itemFromIndex( index( 0, 0, QModelIndex() ) );
        if ( youngestItem->query()->playedBy().second <= playtime )
            insert( query, 0 );
        else
        {
            for ( int i = 0; i < count - 1; i++ )
            {
                TrackModelItem* item1 = itemFromIndex( index( i, 0, QModelIndex() ) );
                TrackModelItem* item2 = itemFromIndex( index( i + 1, 0, QModelIndex() ) );

                if ( item1->query()->playedBy().second >= playtime && item2->query()->playedBy().second <= playtime )
                {
                    insert( query, i + 1 );
                    break;
                }
            }
        }
    }
    else
        insert( query, 0 );

    if ( trackCount() > (int)m_limit )
        remove( m_limit );

    ensureResolved();
}


bool
RecentlyPlayedModel::isTemporary() const
{
    return true;
}
