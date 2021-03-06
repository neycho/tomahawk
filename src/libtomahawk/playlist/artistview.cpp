/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "artistview.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>

#include "audio/audioengine.h"
#include "context/ContextWidget.h"
#include "dynamic/widgets/LoadingSpinner.h"
#include "widgets/overlaywidget.h"

#include "contextmenu.h"
#include "tomahawksettings.h"
#include "treeheader.h"
#include "treeitemdelegate.h"
#include "treemodel.h"
#include "viewmanager.h"
#include "utils/logger.h"

#define SCROLL_TIMEOUT 280

using namespace Tomahawk;


ArtistView::ArtistView( QWidget* parent )
    : QTreeView( parent )
    , m_header( new TreeHeader( this ) )
    , m_overlay( new OverlayWidget( this ) )
    , m_model( 0 )
    , m_proxyModel( 0 )
//    , m_delegate( 0 )
    , m_loadingSpinner( new LoadingSpinner( this ) )
    , m_updateContextView( true )
    , m_contextMenu( new ContextMenu( this ) )
    , m_showModes( true )
{
    setAlternatingRowColors( true );
    setDragEnabled( true );
    setDropIndicatorShown( false );
    setDragDropOverwriteMode( false );
    setUniformRowHeights( false );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    setRootIsDecorated( true );
    setAnimated( false );
    setAllColumnsShowFocus( true );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setContextMenuPolicy( Qt::CustomContextMenu );

    setHeader( m_header );
    setProxyModel( new TreeProxyModel( this ) );

    #ifndef Q_WS_WIN
    QFont f = font();
    f.setPointSize( f.pointSize() - 1 );
    setFont( f );
    #endif

    #ifdef Q_WS_MAC
    f.setPointSize( f.pointSize() - 2 );
    setFont( f );
    #endif

    m_timer.setInterval( SCROLL_TIMEOUT );
    connect( verticalScrollBar(), SIGNAL( rangeChanged( int, int ) ), SLOT( onViewChanged() ) );
    connect( verticalScrollBar(), SIGNAL( valueChanged( int ) ), SLOT( onViewChanged() ) );
    connect( &m_timer, SIGNAL( timeout() ), SLOT( onScrollTimeout() ) );

    connect( this, SIGNAL( doubleClicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), SLOT( onCustomContextMenu( const QPoint& ) ) );
    connect( m_contextMenu, SIGNAL( triggered( int ) ), SLOT( onMenuTriggered( int ) ) );
}


ArtistView::~ArtistView()
{
    qDebug() << Q_FUNC_INFO;
}


void
ArtistView::setProxyModel( TreeProxyModel* model )
{
    m_proxyModel = model;
    setItemDelegate( new TreeItemDelegate( this, m_proxyModel ) );

    QTreeView::setModel( m_proxyModel );
}


void
ArtistView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    qDebug() << "Explicitly use setPlaylistModel instead";
    Q_ASSERT( false );
}


void
ArtistView::setTreeModel( TreeModel* model )
{
    m_model = model;

    if ( m_proxyModel )
    {
        m_proxyModel->setSourceTreeModel( model );
        m_proxyModel->sort( 0 );
    }

    connect( m_model, SIGNAL( loadingStarted() ), m_loadingSpinner, SLOT( fadeIn() ) );
    connect( m_model, SIGNAL( loadingFinished() ), m_loadingSpinner, SLOT( fadeOut() ) );
    connect( m_proxyModel, SIGNAL( filteringStarted() ), SLOT( onFilteringStarted() ) );
    connect( m_proxyModel, SIGNAL( filteringFinished() ), m_loadingSpinner, SLOT( fadeOut() ) );

    connect( m_model, SIGNAL( itemCountChanged( unsigned int ) ), SLOT( onItemCountChanged( unsigned int ) ) );
    connect( m_proxyModel, SIGNAL( filteringFinished() ), SLOT( onFilterChangeFinished() ) );
    connect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onViewChanged() ) );

    guid(); // this will set the guid on the header

    if ( model->columnStyle() == TreeModel::TrackOnly )
    {
        setHeaderHidden( true );
        setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    }
    else
    {
        setHeaderHidden( false );
        setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    }
}


void
ArtistView::onViewChanged()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    m_timer.start();
}


void
ArtistView::onScrollTimeout()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    QModelIndex left = indexAt( viewport()->rect().topLeft() );
    while ( left.isValid() && left.parent().isValid() )
        left = left.parent();

    QModelIndex right = indexAt( viewport()->rect().bottomLeft() );
    while ( right.isValid() && right.parent().isValid() )
        right = right.parent();

    int max = m_proxyModel->playlistInterface()->trackCount();
    if ( right.isValid() )
        max = right.row() + 1;

    if ( !max )
        return;

    for ( int i = left.row(); i < max; i++ )
    {
        m_model->getCover( m_proxyModel->mapToSource( m_proxyModel->index( i, 0 ) ) );
    }
}


void
ArtistView::currentChanged( const QModelIndex& current, const QModelIndex& previous )
{
    QTreeView::currentChanged( current, previous );

    if ( !m_updateContextView )
        return;

    TreeModelItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( current ) );
    if ( item )
    {
        if ( !item->result().isNull() )
            ViewManager::instance()->context()->setQuery( item->result()->toQuery() );
        else if ( !item->artist().isNull() )
            ViewManager::instance()->context()->setArtist( item->artist() );
        else if ( !item->album().isNull() )
            ViewManager::instance()->context()->setAlbum( item->album() );
        else if ( !item->query().isNull() )
            ViewManager::instance()->context()->setQuery( item->query() );
    }
}


void
ArtistView::onItemActivated( const QModelIndex& index )
{
    TreeModelItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item )
    {
        if ( !item->artist().isNull() )
            ViewManager::instance()->show( item->artist() );
        else if ( !item->album().isNull() )
            ViewManager::instance()->show( item->album(), m_model->mode() );
        else if ( !item->result().isNull() && item->result()->isOnline() )
        {
            m_model->setCurrentItem( item->index );
            AudioEngine::instance()->playItem( m_proxyModel->playlistInterface(), item->result() );
        }
    }
}


void
ArtistView::keyPressEvent( QKeyEvent* event )
{
    QTreeView::keyPressEvent( event );

    if ( !model() )
        return;

    if ( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return )
    {
        onItemActivated( currentIndex() );
    }
}


void
ArtistView::resizeEvent( QResizeEvent* event )
{
    QTreeView::resizeEvent( event );
    m_header->checkState();

    if ( !model() )
        return;

    if ( model()->columnCount( QModelIndex() ) == 1 )
    {
        m_header->resizeSection( 0, event->size().width() );
    }
}


void
ArtistView::onItemCountChanged( unsigned int items )
{
    if ( items == 0 )
    {
        if ( m_model->collection().isNull() || ( !m_model->collection().isNull() && m_model->collection()->source()->isLocal() ) )
            m_overlay->setText( tr( "After you have scanned your music collection you will find your tracks right here." ) );
        else
            m_overlay->setText( tr( "This collection is currently empty." ) );

        m_overlay->show();
    }
    else
        m_overlay->hide();
}


void
ArtistView::onFilterChangeFinished()
{
    if ( selectedIndexes().count() )
        scrollTo( selectedIndexes().at( 0 ), QAbstractItemView::PositionAtCenter );

    if ( !proxyModel()->playlistInterface()->filter().isEmpty() && !proxyModel()->playlistInterface()->trackCount() && model()->trackCount() )
    {
        m_overlay->setText( tr( "Sorry, your filter '%1' did not match any results." ).arg( proxyModel()->playlistInterface()->filter() ) );
        m_overlay->show();
    }
    else
        if ( model()->trackCount() )
            m_overlay->hide();
}


void
ArtistView::onFilteringStarted()
{
    m_overlay->hide();
    m_loadingSpinner->fadeIn();
}


void
ArtistView::startDrag( Qt::DropActions supportedActions )
{
    QList<QPersistentModelIndex> pindexes;
    QModelIndexList indexes;
    foreach( const QModelIndex& idx, selectedIndexes() )
    {
        if ( ( m_proxyModel->flags( idx ) & Qt::ItemIsDragEnabled ) )
        {
            indexes << idx;
            pindexes << idx;
        }
    }

    if ( indexes.count() == 0 )
        return;

    qDebug() << "Dragging" << indexes.count() << "indexes";
    QMimeData* data = m_proxyModel->mimeData( indexes );
    if ( !data )
        return;

    QDrag* drag = new QDrag( this );
    drag->setMimeData( data );

    QPixmap p;
    if ( data->hasFormat( "application/tomahawk.metadata.artist" ) )
        p = TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeArtist, indexes.count() );
    else if ( data->hasFormat( "application/tomahawk.metadata.album" ) )
        p = TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeAlbum, indexes.count() );
    else
        p = TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeTrack, indexes.count() );

    drag->setPixmap( p );
    drag->setHotSpot( QPoint( -20, -20 ) );

    drag->exec( supportedActions, Qt::CopyAction );
}


void
ArtistView::onCustomContextMenu( const QPoint& pos )
{
    m_contextMenu->clear();

    QModelIndex idx = indexAt( pos );
    idx = idx.sibling( idx.row(), 0 );
    m_contextMenuIndex = idx;

    if ( !idx.isValid() )
        return;

    QList<query_ptr> queries;
    QList<artist_ptr> artists;
    QList<album_ptr> albums;

    foreach ( const QModelIndex& index, selectedIndexes() )
    {
        if ( index.column() || selectedIndexes().contains( index.parent() ) )
            continue;

        TreeModelItem* item = m_proxyModel->itemFromIndex( m_proxyModel->mapToSource( index ) );

        if ( item && !item->result().isNull() )
            queries << item->result()->toQuery();
        else if ( item && !item->query().isNull() )
            queries << item->query();
        if ( item && !item->artist().isNull() )
            artists << item->artist();
        if ( item && !item->album().isNull() )
            albums << item->album();
    }

    m_contextMenu->setQueries( queries );
    m_contextMenu->setArtists( artists );
    m_contextMenu->setAlbums( albums );

    m_contextMenu->exec( mapToGlobal( pos ) );
}


void
ArtistView::onMenuTriggered( int action )
{
    switch ( action )
    {
        case ContextMenu::ActionPlay:
            onItemActivated( m_contextMenuIndex );
            break;

        default:
            break;
    }
}


bool
ArtistView::jumpToCurrentTrack()
{
    if ( !m_proxyModel )
        return false;

    scrollTo( m_proxyModel->currentIndex(), QAbstractItemView::PositionAtCenter );
    return true;
}


QString
ArtistView::guid() const
{
    if ( m_guid.isEmpty() )
    {
        m_guid = QString( "artistview/%1" ).arg( m_model->columnCount( QModelIndex() ) );
        m_header->setGuid( m_guid );
    }

    return m_guid;
}
