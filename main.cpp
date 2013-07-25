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

#include <QApplication>
#include <QIcon>
#include <QGLFormat>
#include "plasmaview.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QGLFormat format;

#if !defined(QT_OPENGL_ES_2)
    // Try for OpenGL 3.2 Core profile
    format.setOption(QGL::NoDeprecatedFunctions);
    format.setVersion(3, 2);
    format.setProfile(QGLFormat::CoreProfile);
#endif

    // Ignored on GLESv2, but doesn't break anything
    format.setSampleBuffers(true);
    format.setSamples(4);

    QGLFormat::setDefaultFormat(format);

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    QIcon::setThemeName("oxygen");
#endif
    PlasmaView gui;
    gui.show();

    return app.exec();
}
