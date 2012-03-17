/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "NowPlayingInfoPlugin.h"


#include "infosystem/infosystemworker.h"
#include "artist.h"
#include "result.h"
#include "globalactionmanager.h"
#include "utils/logger.h"

#include <QTimer>


using namespace Tomahawk::InfoSystem;

void Tomahawk::InfoSystem::NowPlayingInfoPlugin::getInfo(Tomahawk::InfoSystem::InfoRequestData requestData)
{
    Q_UNUSED( requestData );
}

void Tomahawk::InfoSystem::NowPlayingInfoPlugin::notInCacheSlot(const Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData)
{
    Q_UNUSED( criteria );
    Q_UNUSED( requestData );
}


NowPlayingInfoPlugin::NowPlayingInfoPlugin()
: InfoPlugin()
{
    tDebug() << Q_FUNC_INFO;

    m_supportedPushTypes << InfoNowPlaying << InfoNowPaused << InfoNowResumed << InfoNowStopped;

    m_pauseTimer = new QTimer( this );
    m_pauseTimer->setSingleShot( true );
    connect( m_pauseTimer, SIGNAL( timeout() ),
            this, SLOT( audioStopped() ) );

    connect( GlobalActionManager::instance(), SIGNAL( shortLinkReady( QUrl, QUrl ) ),
             SLOT( shortLinkReady( QUrl, QUrl ) ) );
}


NowPlayingInfoPlugin::~NowPlayingInfoPlugin()
{
}

void
NowPlayingInfoPlugin::shortLinkReady( QUrl longUrl, QUrl shortUrl )
{
    // The URL we received is either from a previous track, or not requested by us
    if( longUrl != m_currentLongUrl )
        return;

    tLog() << Q_FUNC_INFO << "got link:" <<  longUrl << shortUrl;
    audioStarted();
}

void
NowPlayingInfoPlugin::pushInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input )
{
    switch ( type )
    {
        case InfoNowPlaying:
            tDebug()<< "NAAAAAAAAAAAAAAAAAAARF" << Q_FUNC_INFO << "NowPlaying" << (qint64)this;
        case InfoNowResumed:
            tDebug()<< "NAAAAAAAAAAAAAAAAAAARF" << Q_FUNC_INFO << "NowResumed" << (qint64)this;

            requestShortenedLink( input );
            break;
        case InfoNowPaused:
            m_pauseTimer->start( 60 * 1000 );
            audioPaused();
            return;
        case InfoNowStopped:
            m_currentTrack.clear();
            audioStopped();
            break;

        default:
            return;
    }

    // Stop the pause timer always, unless pausing of course
    m_pauseTimer->stop();
}

void
NowPlayingInfoPlugin::requestShortenedLink( const QVariant& input )
{
    if ( !input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
        return;

    m_currentTrack = input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !m_currentTrack.contains( "title" ) || !m_currentTrack.contains( "artist" ) )
        return;

    // Request a short URL
        m_currentLongUrl = openLinkFromHash( m_currentTrack );
    GlobalActionManager::instance()->shortenLink( m_currentLongUrl );
}


QUrl
NowPlayingInfoPlugin::openLinkFromHash( const Tomahawk::InfoSystem::InfoStringHash& hash ) const
{
    QString title, artist, album;

    if( !hash.isEmpty() && hash.contains( "title" ) && hash.contains( "artist" ) )
    {
        title = hash["title"];
        artist = hash["artist"];
        if( hash.contains( "album" ) )
            album = hash["album"];
    }

    return GlobalActionManager::instance()->openLink( title, artist, album );
}


/** Audio state slots */
void
NowPlayingInfoPlugin::audioStarted()
{
    tLog() << Q_FUNC_INFO << "PLAYBACK STARTED!!!!";
}


void
NowPlayingInfoPlugin::audioStopped()
{
    tLog() << "PLAYBACK STOPPED!!!!!";
}

void
NowPlayingInfoPlugin::audioPaused()
{
    tLog() << "PLAYBACK PAUSED!!!!";
}
