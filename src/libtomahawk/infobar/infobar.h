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

#ifndef INFOBAR_H
#define INFOBAR_H

#include <QWidget>

#include "dllmacro.h"
#include "artist.h"

class QueryLabel;
class QCheckBox;
class QTimeLine;
class QSearchField;
class ContextWidget;

namespace Ui
{
    class InfoBar;
}

class DLLEXPORT InfoBar : public QWidget
{
Q_OBJECT

public:
    InfoBar( QWidget* parent = 0 );
    ~InfoBar();

public slots:
    void setCaption( const QString& s );

    void setDescription( const QString& s );
    // If you want a querylabel instead of an ElidedLabel
    void setDescription( const Tomahawk::artist_ptr& artist );
    void setDescription( const Tomahawk::album_ptr& album_ptr );

    void setLongDescription( const QString& s );
    void setPixmap( const QPixmap& p );

    void setFilter( const QString& filter );
    void setFilterAvailable( bool b );

    void setAutoUpdateAvailable( bool b );
signals:
    void filterTextChanged( const QString& filter );
    void autoUpdateChanged( bool checked );

protected:
    void changeEvent( QEvent* e );

private slots:
    void onFilterEdited();
    void artistClicked();

private:
    Ui::InfoBar* ui;

    QSearchField* m_searchWidget;
    QCheckBox* m_autoUpdate;
    QueryLabel* m_queryLabel;
};

#endif // INFOBAR_H
