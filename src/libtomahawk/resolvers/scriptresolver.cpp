/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi            <lfranchi@kde.org>
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

#include "scriptresolver.h"

#include <QtEndian>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>

#include "artist.h"
#include "album.h"
#include "pipeline.h"
#include "sourcelist.h"

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#ifdef Q_OS_WIN
#include <shlwapi.h>
#endif

ScriptResolver::ScriptResolver( const QString& exe )
    : Tomahawk::ExternalResolverGui( exe )
    , m_num_restarts( 0 )
    , m_msgsize( 0 )
    , m_ready( false )
    , m_stopped( true )
    , m_configSent( false )
    , m_deleting( false )
    , m_error( Tomahawk::ExternalResolver::NoError )
{
    tLog() << Q_FUNC_INFO << "Created script resolver:" << exe;
    connect( &m_proc, SIGNAL( readyReadStandardError() ), SLOT( readStderr() ) );
    connect( &m_proc, SIGNAL( readyReadStandardOutput() ), SLOT( readStdout() ) );
    connect( &m_proc, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( cmdExited( int, QProcess::ExitStatus ) ) );

    startProcess();

    if ( !TomahawkUtils::nam() )
        return;

    // set the name to the binary, if we launch properly we'll get the name the resolver reports
    m_name = QFileInfo( filePath() ).baseName();
}


ScriptResolver::~ScriptResolver()
{
    disconnect( &m_proc, SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( cmdExited( int, QProcess::ExitStatus ) ) );
    m_deleting = true;

    m_proc.kill();
    m_proc.waitForFinished(); // might call handleMsg

    Tomahawk::Pipeline::instance()->removeResolver( this );

    if ( !m_configWidget.isNull() )
        delete m_configWidget.data();
}


Tomahawk::ExternalResolver*
ScriptResolver::factory( const QString& exe )
{
    ExternalResolver* res = 0;

    const QFileInfo fi( exe );
    if ( fi.suffix() != "js" && fi.suffix() != "script" )
    {
        res = new ScriptResolver( exe );
        tLog() << Q_FUNC_INFO << exe << "Loaded.";
    }

    return res;
}


void
ScriptResolver::start()
{
    m_stopped = false;
    if ( m_ready )
        Tomahawk::Pipeline::instance()->addResolver( this );
    else if ( !m_configSent )
        sendConfig();
    // else, we've sent our config msg so are waiting for the resolver to react
}


void
ScriptResolver::sendConfig()
{
    // Send a configutaion message with any information the resolver might need
    // For now, only the proxy information is sent
    QVariantMap m;
    m.insert( "_msgtype", "config" );

    m_configSent = true;

    TomahawkUtils::NetworkProxyFactory* factory = dynamic_cast<TomahawkUtils::NetworkProxyFactory*>( TomahawkUtils::nam()->proxyFactory() );
    QNetworkProxy proxy = factory->proxy();
    QString proxyType = ( proxy.type() == QNetworkProxy::Socks5Proxy ? "socks5" : "none" );
    m.insert( "proxytype", proxyType );
    m.insert( "proxyhost", proxy.hostName() );
    m.insert( "proxyport", proxy.port() );
    m.insert( "proxyuser", proxy.user() );
    m.insert( "proxypass", proxy.password() );

    // QJson sucks
    QVariantList hosts;
    foreach ( const QString& host, factory->noProxyHosts() )
        hosts << host;
    m.insert( "noproxyhosts", hosts );

    QByteArray data = m_serializer.serialize( m );
    sendMsg( data );
}


void
ScriptResolver::reload()
{
    startProcess();
}


bool
ScriptResolver::running() const
{
    return !m_stopped;
}


void
ScriptResolver::readStderr()
{
    tLog() << "SCRIPT_STDERR" << filePath() << m_proc.readAllStandardError();
}


ScriptResolver::ErrorState
ScriptResolver::error() const
{
    return m_error;
}


void
ScriptResolver::readStdout()
{
    if ( m_msgsize == 0 )
    {
        if ( m_proc.bytesAvailable() < 4 )
            return;

        quint32 len_nbo;
        m_proc.read( (char*) &len_nbo, 4 );
        m_msgsize = qFromBigEndian( len_nbo );
    }

    if ( m_msgsize > 0 )
    {
        m_msg.append( m_proc.read( m_msgsize - m_msg.length() ) );
    }

    if ( m_msgsize == (quint32) m_msg.length() )
    {
        handleMsg( m_msg );
        m_msgsize = 0;
        m_msg.clear();

        if ( m_proc.bytesAvailable() )
            QTimer::singleShot( 0, this, SLOT( readStdout() ) );
    }
}


void
ScriptResolver::sendMsg( const QByteArray& msg )
{
//     qDebug() << Q_FUNC_INFO << m_ready << msg << msg.length();
    if ( !m_proc.isOpen() )
        return;

    quint32 len;
    qToBigEndian( msg.length(), (uchar*) &len );
    m_proc.write( (const char*) &len, 4 );
    m_proc.write( msg );
}


void
ScriptResolver::handleMsg( const QByteArray& msg )
{
//    qDebug() << Q_FUNC_INFO << msg.size() << QString::fromAscii( msg );

    // Might be called from waitForFinished() in ~ScriptResolver, no database in that case, abort.
    if ( m_deleting )
        return;

    bool ok;
    QVariant v = m_parser.parse( msg, &ok );
    if ( !ok || v.type() != QVariant::Map )
    {
        Q_ASSERT(false);
        return;
    }
    QVariantMap m = v.toMap();
    QString msgtype = m.value( "_msgtype" ).toString();

    if ( msgtype == "settings" )
    {
        doSetup( m );
        return;
    }
    else if ( msgtype == "confwidget" )
    {
        setupConfWidget( m );
        return;
    }

    else if ( msgtype == "results" )
    {
        const QString qid = m.value( "qid" ).toString();
        QList< Tomahawk::result_ptr > results;
        const QVariantList reslist = m.value( "results" ).toList();

        foreach( const QVariant& rv, reslist )
        {
            QVariantMap m = rv.toMap();
            qDebug() << "Found result:" << m;

            Tomahawk::result_ptr rp = Tomahawk::Result::get( m.value( "url" ).toString() );
            Tomahawk::artist_ptr ap = Tomahawk::Artist::get( m.value( "artist" ).toString(), false );
            rp->setArtist( ap );
            rp->setAlbum( Tomahawk::Album::get( ap, m.value( "album" ).toString(), false ) );
            rp->setAlbumPos( m.value( "albumpos" ).toUInt() );
            rp->setTrack( m.value( "track" ).toString() );
            rp->setDuration( m.value( "duration" ).toUInt() );
            rp->setBitrate( m.value( "bitrate" ).toUInt() );
            rp->setSize( m.value( "size" ).toUInt() );
            rp->setRID( uuid() );
            rp->setFriendlySource( m_name );
            rp->setYear( m.value( "year").toUInt() );
            rp->setDiscNumber( m.value( "discnumber" ).toUInt() );

            rp->setMimetype( m.value( "mimetype" ).toString() );
            if ( rp->mimetype().isEmpty() )
            {
                rp->setMimetype( TomahawkUtils::extensionToMimetype( m.value( "extension" ).toString() ) );
                Q_ASSERT( !rp->mimetype().isEmpty() );
            }

            results << rp;
        }

        Tomahawk::Pipeline::instance()->reportResults( qid, results );
    }
    else if ( msgtype == "playlist" )
    {

        QList< Tomahawk::query_ptr > tracks;
        const QString qid = m.value( "qid" ).toString();
        const QString title = m.value( "identifier" ).toString();
        const QVariantList reslist = m.value( "playlist" ).toList();

        if( !reslist.isEmpty() )
        {
            foreach( const QVariant& rv, reslist )
            {
                QVariantMap m = rv.toMap();
                qDebug() << "Found playlist result:" << m;
                Tomahawk::query_ptr q = Tomahawk::Query::get( m.value( "artist" ).toString() , m.value( "track" ).toString() , QString(), uuid(), false );
                tracks << q;
            }
        }
    }
}


void
ScriptResolver::cmdExited( int code, QProcess::ExitStatus status )
{
    m_ready = false;
    tLog() << Q_FUNC_INFO << "SCRIPT EXITED, code" << code << "status" << status << filePath();
    Tomahawk::Pipeline::instance()->removeResolver( this );

    m_error = ExternalResolver::FailedToLoad;
    emit changed();

    if ( m_stopped )
    {
        tLog() << "*** Script resolver stopped ";
        emit terminated();

        return;
    }

    if ( m_num_restarts < 10 )
    {
        m_num_restarts++;
        tLog() << "*** Restart num" << m_num_restarts;
        startProcess();
        sendConfig();
    }
    else
    {
        tLog() << "*** Reached max restarts, not restarting.";
    }
}

void
ScriptResolver::resolve( const Tomahawk::query_ptr& query )
{
    QVariantMap m;
    m.insert( "_msgtype", "rq" );

    if ( query->isFullTextQuery() )
    {
        m.insert( "fulltext", query->fullTextQuery() );
        m.insert( "artist", query->artist() );
        m.insert( "track", query->fullTextQuery() );
        m.insert( "qid", query->id() );
    }
    else
    {
        m.insert( "artist", query->artist() );
        m.insert( "track", query->track() );
        m.insert( "qid", query->id() );
    }

    const QByteArray msg = m_serializer.serialize( QVariant( m ) );
    sendMsg( msg );
}


void
ScriptResolver::doSetup( const QVariantMap& m )
{
//    qDebug() << Q_FUNC_INFO << m;

    m_name    = m.value( "name" ).toString();
    m_weight  = m.value( "weight", 0 ).toUInt();
    m_timeout = m.value( "timeout", 5 ).toUInt() * 1000;
    qDebug() << "SCRIPT" << filePath() << "READY," << "name" << m_name << "weight" << m_weight << "timeout" << m_timeout;

    m_ready = true;
    m_configSent = false;

    if ( !m_stopped )
        Tomahawk::Pipeline::instance()->addResolver( this );

    emit changed();
}


void
ScriptResolver::setupConfWidget( const QVariantMap& m )
{
    bool compressed = m.value( "compressed", "false" ).toString() == "true";
    qDebug() << "Resolver has a preferences widget! compressed?" << compressed;

    QByteArray uiData = m[ "widget" ].toByteArray();
    if( compressed )
        uiData = qUncompress( QByteArray::fromBase64( uiData ) );
    else
        uiData = QByteArray::fromBase64( uiData );

    if ( m.contains( "images" ) )
        uiData = fixDataImagePaths( uiData, compressed, m[ "images" ].toMap() );
    m_configWidget = QWeakPointer< QWidget >( widgetFromData( uiData, 0 ) );

    emit changed();
}


void ScriptResolver::startProcess()
{
    if ( !QFile::exists( filePath() ) )
        m_error = Tomahawk::ExternalResolver::FileNotFound;
    else
    {
        m_error = Tomahawk::ExternalResolver::NoError;
    }

    QFileInfo fi( filePath() );

    QString interpreter;
    QString runPath = filePath();

#ifdef Q_OS_WIN
    if ( fi.suffix().toLower() != "exe" )
    {
        DWORD dwSize = MAX_PATH;

        wchar_t path[MAX_PATH] = { 0 };
        wchar_t *ext = (wchar_t *) ("." + fi.suffix()).utf16();

        HRESULT hr = AssocQueryStringW(
                (ASSOCF) 0,
                ASSOCSTR_EXECUTABLE,
                ext,
                L"open",
                path,
                &dwSize
        );

        if ( ! FAILED( hr ) )
        {
            interpreter = QString( "\"%1\"" ).arg(QString::fromUtf16((const ushort *) path));
        }
    }
    else
    {
        // have to enclose in quotes if path contains spaces on windows...
        runPath = QString( "\"%1\"" ).arg( filePath() );
    }
#endif // Q_OS_WIN

    if( interpreter.isEmpty() )
        m_proc.start( runPath );
    else
        m_proc.start( interpreter, QStringList() << filePath() );

    sendConfig();
}


void
ScriptResolver::saveConfig()
{
    Q_ASSERT( !m_configWidget.isNull() );

    QVariantMap m;
    m.insert( "_msgtype", "setpref" );
    QVariant widgets = configMsgFromWidget( m_configWidget.data() );
    m.insert( "widgets", widgets );
    QByteArray data = m_serializer.serialize( m );

    sendMsg( data );
}


QWidget*
ScriptResolver::configUI() const
{
    if ( m_configWidget.isNull() )
        return 0;
    else
        return m_configWidget.data();
}


void
ScriptResolver::stop()
{
    m_stopped = true;
    Tomahawk::Pipeline::instance()->removeResolver( this );
}
