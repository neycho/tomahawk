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

#ifndef TRACKVIEW_H
#define TRACKVIEW_H

#include <QtGui/QTreeView>
#include <QtGui/QSortFilterProxyModel>
#include <QtCore/QTimer>

#include "contextmenu.h"
#include "playlistitemdelegate.h"

#include "dllmacro.h"

class QAction;
class LoadingSpinner;
class TrackHeader;
class TrackModel;
class TrackProxyModel;
class OverlayWidget;

class DLLEXPORT TrackView : public QTreeView
{
Q_OBJECT

public:
    explicit TrackView( QWidget* parent = 0 );
    ~TrackView();

    virtual QString guid() const { return m_guid; }
    virtual void setGuid( const QString& guid );

    virtual void setTrackModel( TrackModel* model );
    virtual void setModel( QAbstractItemModel* model );
    void setProxyModel( TrackProxyModel* model );

    virtual TrackModel* model() const { return m_model; }
    TrackProxyModel* proxyModel() const { return m_proxyModel; }
    PlaylistItemDelegate* delegate() const { return m_delegate; }
    TrackHeader* header() const { return m_header; }
    OverlayWidget* overlay() const { return m_overlay; }
    Tomahawk::ContextMenu* contextMenu() const { return m_contextMenu; }
    LoadingSpinner* loadingSpinner() const { return m_loadingSpinner; }

    QModelIndex hoveredIndex() const { return m_hoveredIndex; }
    QModelIndex contextMenuIndex() const { return m_contextMenuIndex; }
    void setContextMenuIndex( const QModelIndex& idx ) { m_contextMenuIndex = idx; }

    bool updatesContextView() const { return m_updateContextView; }
    void setUpdatesContextView( bool b ) { m_updateContextView = b; }

    // Starts playing from the beginning if resolved, or waits until a track is playable
    void startPlayingFromStart();

public slots:
    virtual void onItemActivated( const QModelIndex& index );

    void playItem();
    void onMenuTriggered( int action );

    void onViewChanged();
    void onScrollTimeout();

signals:
    void itemActivated( const QModelIndex& index );

protected:
    virtual void resizeEvent( QResizeEvent* event );

    virtual void startDrag( Qt::DropActions supportedActions );
    virtual void dragEnterEvent( QDragEnterEvent* event );
    virtual void dragLeaveEvent( QDragLeaveEvent* /*event*/ ) { m_dragging = false; setDirtyRegion( m_dropRect ); }
    virtual void dragMoveEvent( QDragMoveEvent* event );
    virtual void dropEvent( QDropEvent* event );

    void wheelEvent( QWheelEvent* event );
    void mouseMoveEvent( QMouseEvent* event );
    void mousePressEvent( QMouseEvent* event );
    void leaveEvent( QEvent* event );
    void paintEvent( QPaintEvent* event );
    void keyPressEvent( QKeyEvent* event );

protected slots:
    virtual void currentChanged( const QModelIndex& current, const QModelIndex& previous );

private slots:
    void onItemResized( const QModelIndex& index );
    void onFilterChanged( const QString& filter );

    void onCustomContextMenu( const QPoint& pos );

    void autoPlayResolveFinished( const Tomahawk::query_ptr& query, int row  );

private:
    void startAutoPlay( const QModelIndex& index );
    bool tryToPlayItem( const QModelIndex& index );
    void updateHoverIndex( const QPoint& pos );

    QString m_guid;
    TrackModel* m_model;
    TrackProxyModel* m_proxyModel;
    PlaylistItemDelegate* m_delegate;
    TrackHeader* m_header;
    OverlayWidget* m_overlay;
    LoadingSpinner* m_loadingSpinner;

    bool m_resizing;
    bool m_dragging;
    QRect m_dropRect;

    bool m_updateContextView;

    QModelIndex m_hoveredIndex;
    QModelIndex m_contextMenuIndex;

    Tomahawk::query_ptr m_autoPlaying;

    Tomahawk::ContextMenu* m_contextMenu;
    
    QTimer m_timer;
};

#endif // TRACKVIEW_H
