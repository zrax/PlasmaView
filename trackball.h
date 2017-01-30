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

#ifndef _TRACKBALL_H
#define _TRACKBALL_H

/* Borrowed from PlasmaShop */

#include <QVector3D>
#include <QMatrix4x4>

class Trackball
{
public:
    Trackball() { }

    void push(const QPointF &p);
    void move(const QPointF &p);
    void release(const QPointF &) { }

    QMatrix4x4 rotation() const { return m_rotation; }

private:
    QMatrix4x4 m_rotation;
    QMatrix4x4 m_lastRotation;

    QPointF m_lastPos;
    QVector3D m_lastPos3D;
};

#endif
