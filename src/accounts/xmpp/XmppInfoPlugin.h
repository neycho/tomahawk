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

#ifndef XMPPINFOPLUGIN_H
#define XMPPINFOPLUGIN_H

#include "infosystem/NowPlayingInfoPlugin.h"

namespace Jreen {
    namespace PubSub {
        class Manager;
    }
    class Client;
class Client;
}

namespace Tomahawk {

    namespace InfoSystem {

        class XmppInfoPlugin  : public NowPlayingInfoPlugin
        {
            Q_OBJECT

        public:
            XmppInfoPlugin();
            virtual ~XmppInfoPlugin();

            // actually this should be set in the ctor
            // but this way we uncouple the order of initialization
            // of SipPlugin and InfoSystemPlugin
            // maybe we should add the possibility to emit new InfoPlugins
            // from accounts
            void setJreenClient(Jreen::Client* client);

        protected:
            void audioStarted();
            void audioStopped();
            void audioPaused();

        private:
            Jreen::PubSub::Manager *m_pubSubManager;
            Jreen::Client* m_client;
        };

    }

}

#endif // XMPPINFOPLUGIN_H
