/* This file is part of PlasmaView.
 *
 * PlasmaView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PlasmaView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Gneral Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PlasmaView.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PLASMAVIEW_H
#define _PLASMAVIEW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <PRP/KeyedObject/plLocation.h>

class QTreeWidget;
class plResManager;
class PlasmaGLWidget;

class PlasmaView : public QMainWindow
{
    Q_OBJECT

public:
    PlasmaView();
    virtual ~PlasmaView();

    void loadAge(const QString &filename);

private slots:
    void onOpenAge();
    void selectObject(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    plResManager *m_resMgr;
    plLocation m_currentLocation;

    QTreeWidget *m_objectTree;
    PlasmaGLWidget *m_render;
};

class PlasmaTreeWidgetItem : public QTreeWidgetItem
{
public:
    PlasmaTreeWidgetItem(QTreeWidget *parent, const QStringList &strings)
        : QTreeWidgetItem(parent, strings) { }
    PlasmaTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings)
        : QTreeWidgetItem(parent, strings) { }

    void setLocation(const plLocation &loc) { m_location = loc; }
    const plLocation &location() const { return m_location; }

private:
    plLocation m_location;
};

#endif
