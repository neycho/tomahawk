/*
    The collection  - acts as container for someones music library
    load() -> async populate by calling addArtists etc,
    then finishedLoading() is emitted.
    then use artists() etc to get the data.
*/

#ifndef TOMAHAWK_COLLECTION_H
#define TOMAHAWK_COLLECTION_H

#include <QHash>
#include <QList>
#include <QSharedPointer>
#include <QDebug>

#include "tomahawk/functimeout.h"
#include "tomahawk/query.h"
#include "tomahawk/source.h"
#include "tomahawk/typedefs.h"
#include "typedefs.h"
#include "dynamic/dynamicplaylist.h"

namespace Tomahawk
{

class Collection : public QObject
{
Q_OBJECT

public:
    Collection( const source_ptr& source, const QString& name, QObject* parent = 0 );
    virtual ~Collection();

    virtual QString name() const;

    virtual void loadPlaylists() { qDebug() << Q_FUNC_INFO; }
    virtual void loadTracks() { qDebug() << Q_FUNC_INFO; }
    virtual void loadDynamicPlaylists() { qDebug() << Q_FUNC_INFO ; }

    virtual Tomahawk::playlist_ptr playlist( const QString& guid );
    virtual Tomahawk::dynplaylist_ptr dynamicPlaylist( const QString& guid );
    
    virtual void addPlaylist( const Tomahawk::playlist_ptr& p );
    virtual void deletePlaylist( const Tomahawk::playlist_ptr& p );
    
    virtual void addDynamicPlaylist( const Tomahawk::dynplaylist_ptr& p );
    virtual void deleteDynamicPlaylist( const Tomahawk::dynplaylist_ptr& p );
    
    virtual QList< Tomahawk::playlist_ptr > playlists() { return m_playlists; }
    virtual QList< Tomahawk::dynplaylist_ptr > dynamicPlaylists() { return m_dynplaylists; }
    virtual QList< Tomahawk::query_ptr > tracks() { return m_tracks; }

    const source_ptr& source() const;
    unsigned int lastmodified() const { return m_lastmodified; }

signals:
    void tracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& );
    void tracksRemoved( const QList<QVariant>&, const Tomahawk::collection_ptr& );
    void tracksFinished( const Tomahawk::collection_ptr& );

    void playlistsAdded( const QList<Tomahawk::playlist_ptr>& );
    void playlistsDeleted( const QList<Tomahawk::playlist_ptr>& );
    
    void dynamicPlaylistsAdded( const QList<Tomahawk::dynplaylist_ptr>& );
    void dynamicPlaylistsDeleted( const QList<Tomahawk::dynplaylist_ptr>& );

public slots:
    virtual void addTracks( const QList<QVariant> &newitems ) = 0;
    virtual void removeTracks( const QList<QVariant> &olditems ) = 0;

    void setPlaylists( const QList<Tomahawk::playlist_ptr>& plists );
    void setDynamicPlaylists( const QList< Tomahawk::dynplaylist_ptr >& dynplists );
    void setTracks( const QList<Tomahawk::query_ptr>& tracks, Tomahawk::collection_ptr collection );
protected:
    QString m_name;
    unsigned int m_lastmodified; // unix time of last change to collection

private:
    source_ptr m_source;
    QList< Tomahawk::playlist_ptr > m_playlists;
    QList< Tomahawk::query_ptr > m_tracks;
    QList< Tomahawk::dynplaylist_ptr > m_dynplaylists;
};

}; // ns

inline uint qHash( const QSharedPointer<Tomahawk::Collection>& key )
{
    return qHash( (void *)key.data() );
}

#endif // TOMAHAWK_COLLECTION_H
