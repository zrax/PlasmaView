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

#include "trackball.h"

#include <QQuaternion>
#include <QtMath>

static const float s_pi = 3.1415926535897932384626f;

void Trackball::push(const QPointF &p)
{
    m_lastPos = p;

    m_lastPos3D = QVector3D(p.x(), p.y(), 0.0f);
    float sqZ = 1 - QVector3D::dotProduct(m_lastPos3D, m_lastPos3D);

    if (sqZ > 0)
        m_lastPos3D.setZ(qSqrt(sqZ));
    else
        m_lastPos3D.normalize();

    m_lastRotation = m_rotation;
}

void Trackball::move(const QPointF &p)
{
    QVector3D currentPos = QVector3D(p.x(), p.y(), 0.0f);
    float sqZ = 1 - QVector3D::dotProduct(currentPos, currentPos);
    if (sqZ > 0)
        currentPos.setZ(qSqrt(sqZ));
    else
        currentPos.normalize();

    QVector3D axis = QVector3D::crossProduct(m_lastPos3D, currentPos);
    float angle = (180 / s_pi) * 2 * qAcos(QVector3D::dotProduct(m_lastPos3D, currentPos));

    axis.normalize();
    QQuaternion rotation = QQuaternion::fromAxisAndAngle(axis, angle);

    QMatrix4x4 rotationMat;
    rotationMat.rotate(rotation);
    m_rotation = rotationMat * m_lastRotation;
}
