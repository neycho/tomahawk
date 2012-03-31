/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "RelatedArtistsContext.h"

#include <QHeaderView>

#include "playlist/artistview.h"
#include "playlist/treemodel.h"

using namespace Tomahawk;


RelatedArtistsContext::RelatedArtistsContext()
    : ContextPage()
    , m_infoId( uuid() )
{
    m_relatedView = new ArtistView();
    m_relatedView->setGuid( "RelatedArtistsContext" );
    m_relatedView->setUpdatesContextView( false );
    m_relatedModel = new TreeModel( m_relatedView );
    m_relatedModel->setColumnStyle( TreeModel::TrackOnly );
    m_relatedView->setTreeModel( m_relatedModel );
    m_relatedView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_relatedView->setSortingEnabled( false );
    m_relatedView->proxyModel()->sort( -1 );

    QPalette pal = m_relatedView->palette();
    pal.setColor( QPalette::Window, QColor( 0, 0, 0, 0 ) );
    m_relatedView->setPalette( pal );

    m_proxy = new QGraphicsProxyWidget();
    m_proxy->setWidget( m_relatedView );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );
}


RelatedArtistsContext::~RelatedArtistsContext()
{
}


void
RelatedArtistsContext::setArtist( const Tomahawk::artist_ptr& artist )
{
    if ( artist.isNull() )
        return;
    if ( !m_artist.isNull() && m_artist->name() == artist->name() )
        return;

    m_artist = artist;

    Tomahawk::InfoSystem::InfoStringHash artistInfo;
    artistInfo["artist"] = artist->name();

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = m_infoId;
    requestData.customData = QVariantMap();
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );

    requestData.type = Tomahawk::InfoSystem::InfoArtistSimilars;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
}


void
RelatedArtistsContext::setQuery( const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    setArtist( Artist::get( query->artist(), false ) );
}


void
RelatedArtistsContext::setAlbum( const Tomahawk::album_ptr& album )
{
    if ( album.isNull() )
        return;

    setArtist( album->artist() );
}


void
RelatedArtistsContext::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != m_infoId )
        return;

    InfoSystem::InfoStringHash trackInfo;
    trackInfo = requestData.input.value< InfoSystem::InfoStringHash >();

    if ( output.canConvert< QVariantMap >() )
    {
        if ( trackInfo["artist"] != m_artist->name() )
        {
            qDebug() << "Returned info was for:" << trackInfo["artist"] << "- was looking for:" << m_artist->name();
            return;
        }
    }

    QVariantMap returnedData = output.value< QVariantMap >();
    switch ( requestData.type )
    {
        case InfoSystem::InfoArtistSimilars:
        {
            m_relatedModel->clear();
            const QStringList artists = returnedData["artists"].toStringList();
            foreach ( const QString& artist, artists )
            {
                m_relatedModel->addArtists( Artist::get( artist ) );
            }
            break;
        }

        default:
            return;
    }
}


void
RelatedArtistsContext::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
}
