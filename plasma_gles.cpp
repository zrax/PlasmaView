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

#include "plasma_gles.h"

#include <QMessageBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QtMath>
#include <PRP/Geometry/plDrawableSpans.h>

static const float s_degPerRad = 0.0174532925f;

PlasmaGLWidget::PlasmaGLWidget(QWidget *parent)
    : QGLWidget(parent), m_theta(0.0f), m_phi(0.0f)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::StrongFocus);
}

void PlasmaGLWidget::clear()
{
    foreach (const RenderData &drawable, m_drawables)
        glDeleteBuffers(2, drawable.m_buffers);
}

void PlasmaGLWidget::addGeometry(plDrawableSpans *spans)
{
    for (size_t grp = 0; grp < spans->getNumBufferGroups(); ++grp) {
        plGBufferGroup *group = spans->getBuffer(grp);
        for (size_t buf = 0; buf < group->getNumVertBuffers(); ++buf) {
            RenderData render;
            render.m_stride = group->getStride();
            render.m_indexCount = group->getIdxBufferCount(buf);
            render.m_weights = (group->getFormat() & plGBufferGroup::kSkinWeightMask) >> 4;
            render.m_skinIndices = (group->getFormat() & plGBufferGroup::kSkinIndices) != 0;
            glGenBuffers(2, render.m_buffers);

            glBindBuffer(GL_ARRAY_BUFFER, render.m_buffers[0]);
            glBufferData(GL_ARRAY_BUFFER, group->getVertBufferSize(buf),
                         group->getVertBufferStorage(buf), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render.m_buffers[1]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, render.m_indexCount * sizeof(GLushort),
                         static_cast<const GLushort *>(group->getIdxBufferStorage(buf)),
                         GL_STATIC_DRAW);

            m_drawables.append(render);
        }
    }
}

void PlasmaGLWidget::initializeGL()
{
    qglClearColor(QColor(0, 255, 255));
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);

    if (!m_shader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vshader.glsl"))
        QMessageBox::warning(this, "Error compiling vshader.glsl", m_shader.log());
    if (!m_shader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fshader.glsl"))
        QMessageBox::warning(this, "Error compiling fshader.glsl", m_shader.log());
    if (!m_shader.link())
        return;
    if (!m_shader.bind())
        return;

    // Starting view position
    QMatrix4x4 view;
    view.rotate(-90.0f, 1.0f, 0.0f, 0.0f);
    view.rotate(0.0f, 0.0f, 0.0f, 1.0f);
    m_shader.setUniformValue("u_view", view);
}

void PlasmaGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));

    float aspect = float(w) / float(h ? h : 1);
    QMatrix4x4 projection;
    projection.perspective(45.0f, aspect, 1.0f, 20000.0f);
    m_shader.setUniformValue("u_projection", projection);
}

void PlasmaGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    foreach (const RenderData &render, m_drawables) {
        glBindBuffer(GL_ARRAY_BUFFER, render.m_buffers[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render.m_buffers[1]);

        uintptr_t offset = 0;
        int vertexPos = m_shader.attributeLocation("a_position");
        m_shader.enableAttributeArray(vertexPos);
        glVertexAttribPointer(vertexPos, 3, GL_FLOAT, GL_FALSE, render.m_stride,
                              reinterpret_cast<GLvoid *>(offset));
        offset += 3 * sizeof(float);

        if (render.m_weights > 0) {
            // Skip weights and skin indices
            offset += sizeof(float) * render.m_weights;
            if (render.m_skinIndices)
                offset += sizeof(int);
        }

        // Normals
        offset += 3 * sizeof(float);

        // Color
        int colorPos = m_shader.attributeLocation("a_color");
        m_shader.enableAttributeArray(colorPos);
        glVertexAttribPointer(colorPos, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                              render.m_stride, reinterpret_cast<GLvoid *>(offset));
        offset += sizeof(unsigned int);

        // Color 2?
        offset += sizeof(unsigned int);

        for (int u = 0; u < render.m_uvws; ++u) {
            // TODO
            offset += 3 * sizeof(float);
        }

        glDrawElements(GL_TRIANGLES, render.m_indexCount,
                       GL_UNSIGNED_SHORT, nullptr);
    }
}

void PlasmaGLWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Down:
        m_phi += 10.0f;
        break;
    case Qt::Key_Up:
        m_phi -= 10.0f;
        break;
    case Qt::Key_Left:
        m_theta -= 10.0f;
        break;
    case Qt::Key_Right:
        m_theta += 10.0f;
        break;
    case Qt::Key_W:
        m_position.setX(m_position.x() + qSin(m_theta * s_degPerRad) * 2.0f);
        m_position.setY(m_position.y() + qCos(m_theta * s_degPerRad) * 2.0f);
        break;
    case Qt::Key_S:
        m_position.setX(m_position.x() - qSin(m_theta * s_degPerRad) * 2.0f);
        m_position.setY(m_position.y() - qCos(m_theta * s_degPerRad) * 2.0f);
        break;
    case Qt::Key_A:
        m_position.setX(m_position.x() - qCos(m_theta * s_degPerRad) * 2.0f);
        m_position.setY(m_position.y() - qSin(m_theta * s_degPerRad) * 2.0f);
        break;
    case Qt::Key_D:
        m_position.setX(m_position.x() + qCos(m_theta * s_degPerRad) * 2.0f);
        m_position.setY(m_position.y() + qSin(m_theta * s_degPerRad) * 2.0f);
        break;
    case Qt::Key_PageUp:
        m_position.setZ(m_position.z() + 2.0f);
        break;
    case Qt::Key_PageDown:
        m_position.setZ(m_position.z() - 2.0f);
        break;
    case Qt::Key_Home:
        m_position = QVector3D(0.0f, 0.0f, 0.0f);
        m_theta = 0.0f;
        m_phi = 0.0f;
        break;
    }

    QMatrix4x4 view;
    view.rotate(-90.0f + m_phi, 1.0f, 0.0f, 0.0f);
    view.rotate(m_theta, 0.0f, 0.0f, 1.0f);
    view.translate(-m_position.x(), -m_position.y(), -m_position.z());
    m_shader.setUniformValue("u_view", view);
    updateGL();
}

void PlasmaGLWidget::mousePressEvent(QMouseEvent *event)
{
    m_mousePos = event->pos();
}

void PlasmaGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        // FPS-style movement
        float delta = m_mousePos.y() - event->pos().y();
        m_position.setX(m_position.x() + qSin(m_theta * s_degPerRad) * delta);
        m_position.setY(m_position.y() + qCos(m_theta * s_degPerRad) * delta);
        m_theta += event->pos().x() - m_mousePos.x();
    } else if (event->buttons() == Qt::RightButton) {
        // Look mode
        m_theta += event->pos().x() - m_mousePos.x();
        m_phi -= m_mousePos.y() - event->pos().y();
        if (m_phi < -90.0f)
            m_phi = -90.0f;
        else if (m_phi > 90.0f)
            m_phi = 90.0f;
    } else if (event->buttons() == Qt::MiddleButton ||
               (event->buttons() == (Qt::LeftButton | Qt::RightButton))) {
        // Pan mode
        float delta = event->pos().x() - m_mousePos.x();
        m_position.setZ(m_position.z() + m_mousePos.y() - event->pos().y());
        m_position.setX(m_position.x() + qSin((m_theta + 90.0f) * s_degPerRad) * delta);
        m_position.setY(m_position.y() + qCos((m_theta + 90.0f) * s_degPerRad) * delta);
    }
    m_mousePos = event->pos();

    QMatrix4x4 view;
    view.rotate(-90.0f + m_phi, 1.0f, 0.0f, 0.0f);
    view.rotate(m_theta, 0.0f, 0.0f, 1.0f);
    view.translate(-m_position.x(), -m_position.y(), -m_position.z());
    m_shader.setUniformValue("u_view", view);
    updateGL();
}
