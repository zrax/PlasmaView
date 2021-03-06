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

#ifndef _PLASMA_SCENE_H
#define _PLASMA_SCENE_H

#include <QGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QVector3D>
#include <QList>

class plDrawableSpans;

class PlasmaGLWidget : public QGLWidget
{
    Q_OBJECT

public:
    PlasmaGLWidget(QWidget *parent = 0);
    virtual ~PlasmaGLWidget() { clear(); }

    void clear();
    void addGeometry(plDrawableSpans *spans);

    enum RenderMode {
        RenderWireframe, RenderFlat, RenderTextured
    };

public slots:
    void setRenderMode(RenderMode mode);

protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);

private:
    struct RenderData
    {
        QOpenGLVertexArrayObject m_vao;
        QOpenGLBuffer m_vBuffer, m_iBuffer;
        GLsizei m_stride;
        GLsizei m_indexCount;
        int m_weights;
        int m_uvws;
        bool m_skinIndices;

        RenderData(GLsizei stride, GLsizei indexCount, int weights,
                   int uvws, bool skinIndices)
            : m_vBuffer(QOpenGLBuffer::VertexBuffer),
              m_iBuffer(QOpenGLBuffer::IndexBuffer), m_stride(stride),
              m_indexCount(indexCount), m_weights(weights), m_uvws(uvws),
              m_skinIndices(skinIndices) { }
    };
    QList<RenderData *> m_drawables;
    QVector3D m_position;
    float m_theta, m_phi;
    QPoint m_mousePos;
    RenderMode m_renderMode;

    QOpenGLShaderProgram m_shader;
    int sha_position;
    int sha_color;
    int shu_view;

    void updateViewMatrix();
};

#endif
