/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

/**
 * \class AlbumInfoWidget
 * \brief ViewPage, which displays an album for an artist and recommends others.
 *
 * This Tomahawk ViewPage displays the tracks of any given album
 * It is our default ViewPage when showing an artist via ViewManager.
 *
 */

#ifndef ALBUMINFOWIDGET_H
#define ALBUMINFOWIDGET_H

#include <QtGui/QWidget>

#include "playlistinterface.h"
#include "viewpage.h"
#include "infosystem/infosystem.h"

#include "dllmacro.h"
#include "typedefs.h"

class AlbumModel;
class TreeModel;
class OverlayButton;

namespace Ui
{
    class AlbumInfoWidget;
}

class DLLEXPORT AlbumInfoWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    AlbumInfoWidget( const Tomahawk::album_ptr& album, Tomahawk::ModelMode startingMode = Tomahawk::InfoSystemMode, QWidget* parent = 0 );
    ~AlbumInfoWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const { return m_title; }
    virtual DescriptionType descriptionType();
    virtual QString description() const { return m_description; }
    virtual Tomahawk::artist_ptr descriptionArtist() const;
    virtual QString longDescription() const { return m_longDescription; }
    virtual QPixmap pixmap() const { if ( m_pixmap.isNull() ) return Tomahawk::ViewPage::pixmap(); else return m_pixmap; }

    virtual bool isTemporaryPage() const { return true; }
    virtual bool showStatsBar() const { return false; }

    virtual bool jumpToCurrentTrack() { return false; }
    virtual bool isBeingPlayed() const;

public slots:
    void setMode( Tomahawk::ModelMode mode );

    /** \brief Loads information for a given album.
     *  \param album The album that you want to load information for.
     *
     *  Calling this method will make AlbumInfoWidget load information about
     *  an album, and related other albums. This method will be automatically
     *  called by the constructor, but you can use it to load another album's
     *  information at any point.
     */
    void load( const Tomahawk::album_ptr& album );

signals:
    void longDescriptionChanged( const QString& description );
    void descriptionChanged( const Tomahawk::artist_ptr& artist );
    void pixmapChanged( const QPixmap& pixmap );

protected:
    void changeEvent( QEvent* e );

private slots:
    void loadAlbums( bool autoRefetch = false );
    void gotAlbums( const QList<Tomahawk::album_ptr>& albums );
    void onAlbumCoverUpdated();

    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );

    void onModeToggle();
    void onAlbumsModeToggle();

    void onLoadingStarted();
    void onLoadingFinished();

private:
    Ui::AlbumInfoWidget *ui;

    Tomahawk::album_ptr m_album;

    AlbumModel* m_albumsModel;
    TreeModel* m_tracksModel;

    OverlayButton* m_button;
    OverlayButton* m_buttonAlbums;

    QString m_title;
    QString m_description;
    QString m_longDescription;
    QPixmap m_pixmap;

    QString m_infoId;
};

#endif // ALBUMINFOWIDGET_H
