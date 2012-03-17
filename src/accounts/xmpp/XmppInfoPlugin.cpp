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


#include "XmppInfoPlugin.h"

#include <jreen/tune.h>
#include <jreen/pubsubmanager.h>
#include <jreen/jid.h>

#include "utils/logger.h"
#include <jreen/client.h>

Tomahawk::InfoSystem::XmppInfoPlugin::XmppInfoPlugin()
    : NowPlayingInfoPlugin()
    , m_pubSubManager(0)
{
}


Tomahawk::InfoSystem::XmppInfoPlugin::~XmppInfoPlugin()
{
    delete m_pubSubManager;
}

void
Tomahawk::InfoSystem::XmppInfoPlugin::setJreenClient(Jreen::Client* client)
{
    if( !client )
        return;

    m_client = client;

    delete m_pubSubManager;
    m_pubSubManager = new Jreen::PubSub::Manager(client);
    m_pubSubManager->addEntityType<Jreen::Tune>();
}


void Tomahawk::InfoSystem::XmppInfoPlugin::audioStarted()
{
    if( !m_pubSubManager )
        return;

    tLog() << "UPDATING JABBER STATUS NOW!" << m_client->jid();

    Jreen::Tune::Ptr tune(new Jreen::Tune());

    tune->setTitle(m_currentTrack.value("title"));
    tune->setArtist(m_currentTrack.value("artist"));
    tune->setLength(360); // m_currentTrack.value("duration").toInt()
    tune->setTrack("track"); //m_currentTrack.value("albumpos").toInt()
    tune->setUri(QUrl("http://narf/")); // m_currentShortUrl

    //TODO: provide a rating once available in Tomahawk
    tune->setRating(10);

    //TODO: it would be nice to set Spotify, Dilandau etc here, but not the jabber ids of friends
    tune->setSource("Tomahawk");

    tLog() << tune->artist() << tune->title() << tune->length() << tune->track() << tune->uri() << tune->rating() << tune->source();

    m_pubSubManager->publishItems(QList<Jreen::Payload::Ptr>() << tune, Jreen::JID());
}

void Tomahawk::InfoSystem::XmppInfoPlugin::audioPaused()
{
}

void Tomahawk::InfoSystem::XmppInfoPlugin::audioStopped()
{
    if( !m_pubSubManager )
        return;

    Jreen::Tune::Ptr tune( new Jreen::Tune() );
    m_pubSubManager->publishItems(QList<Jreen::Payload::Ptr>() << tune, Jreen::JID());
}


