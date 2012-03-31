/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef TOMAHAWKAPP_H
#define TOMAHAWKAPP_H

#define APP TomahawkApp::instance()

#include "headlesscheck.h"
#include "config.h"

#include <QtCore/QRegExp>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QPersistentModelIndex>

#include "QxtHttpServerConnector"
#include "QxtHttpSessionManager"

#include "mac/tomahawkapp_mac.h" // for PlatforInterface
#include "typedefs.h"
#include "utils/tomahawkutils.h"
#include "thirdparty/kdsingleapplicationguard/kdsingleapplicationguard.h"

class AudioEngine;
class Database;
class ScanManager;
class Servent;
class SipHandler;
class TomahawkSettings;
class XMPPBot;

namespace Tomahawk
{
    class ShortcutHandler;
    namespace InfoSystem
    {
        class InfoSystem;
    }

    namespace Accounts
    {
        class AccountManager;
    }
}

#ifdef LIBLASTFM_FOUND
#include <lastfm/NetworkAccessManager>
#include "scrobbler.h"
#endif

#ifndef TOMAHAWK_HEADLESS
class TomahawkWindow;
#endif


// this also acts as a a container for important top-level objects
// that other parts of the app need to find
// (eg, library, pipeline, friends list)
class TomahawkApp : public TOMAHAWK_APPLICATION, public Tomahawk::PlatformInterface
{
Q_OBJECT

public:
    TomahawkApp( int& argc, char *argv[] );
    virtual ~TomahawkApp();

    void init();
    static TomahawkApp* instance();

    XMPPBot* xmppBot() { return m_xmppBot.data(); }

#ifndef ENABLE_HEADLESS
    AudioControls* audioControls();
    TomahawkWindow* mainWindow() const { return m_mainwindow; }
#endif

    // PlatformInterface
    virtual bool loadUrl( const QString& url );

    bool isTomahawkLoaded() const { return m_loaded; }

signals:
    void tomahawkLoaded();

public slots:
    virtual void activate();
    void instanceStarted( KDSingleApplicationGuard::Instance );

private slots:
    void initServent();
    void initSIP();
    void initHTTP();

    void spotifyApiCheckFinished();

private:
    void registerMetaTypes();

    void printHelp();

    // Start-up order: database, collection, pipeline, servent, http
    void initDatabase();
    void initLocalCollection();
    void initPipeline();

    QWeakPointer<Database> m_database;
    QWeakPointer<ScanManager> m_scanManager;
    QWeakPointer<AudioEngine> m_audioEngine;
    QWeakPointer<Servent> m_servent;
    QWeakPointer<Tomahawk::InfoSystem::InfoSystem> m_infoSystem;
    QWeakPointer<XMPPBot> m_xmppBot;
    QWeakPointer<Tomahawk::ShortcutHandler> m_shortcutHandler;
    QWeakPointer< Tomahawk::Accounts::AccountManager > m_accountManager;
    bool m_scrubFriendlyName;

#ifdef LIBLASTFM_FOUND
    Scrobbler* m_scrobbler;
#endif

#ifndef TOMAHAWK_HEADLESS
    TomahawkWindow* m_mainwindow;
#endif

    bool m_headless, m_loaded;

    QWeakPointer< QxtHttpServerConnector > m_connector;
    QWeakPointer< QxtHttpSessionManager > m_session;
};

Q_DECLARE_METATYPE( QPersistentModelIndex )
Q_DECLARE_METATYPE( PairList )

#endif // TOMAHAWKAPP_H

