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

#ifndef TOMAHAWK_NOWPLAYINGINFOSYSTEMPLUGIN_H
#define TOMAHAWK_NOWPLAYINGINFOSYSTEMPLUGIN_H

#include "infosystem/infosystem.h"

#include <QVariant>

#include "dllmacro.h"

class QTimer;

namespace Tomahawk {

    namespace InfoSystem {

        class DLLEXPORT NowPlayingInfoPlugin : public InfoPlugin
        {
            Q_OBJECT

        public:
            NowPlayingInfoPlugin();
            virtual ~NowPlayingInfoPlugin();

        protected slots:
            void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData );

            void pushInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input );

        protected:
            virtual void audioStarted();
            virtual void audioStopped();
            virtual void audioPaused();

        public slots:
            virtual void notInCacheSlot( const Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData );

        private slots:
            void shortLinkReady( QUrl longUrl, QUrl shortUrl );

        private:
            void requestShortenedLink( const QVariant& input );
            QUrl openLinkFromHash( const InfoStringHash& hash ) const;


        protected:
            Tomahawk::InfoSystem::InfoStringHash m_currentTrack;

            QUrl m_currentLongUrl;
            QUrl m_currentShortUrl;

        private:
            QTimer* m_pauseTimer;

        };


    }

}

#endif // TOMAHAWK_NOWPLAYINGINFOSYSTEMPLUGIN_H
