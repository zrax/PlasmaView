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

#include "plasma_scene.h"

#include <QMessageBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QtCore/qmath.h>
#include <PRP/Geometry/plDrawableSpans.h>

static const float s_degPerRad = 0.0174532925f;

// Qt's samples inherit this to make it "look like raw OpenGL", but personally
// I think that looks hacky, and I'd rather explicitly call out that I'm using
// an automagical function wrapper.
#if defined(QT_OPENGL_ES_2)
#   include <QOpenGLFunctions_ES2>
    static QOpenGLFunctions_ES2 glf;
#else
#   include <QOpenGLFunctions_3_2_Core>
    static QOpenGLFunctions_3_2_Core glf;
#endif

PlasmaGLWidget::PlasmaGLWidget(QWidget *parent)
    : QGLWidget(parent), m_theta(0.0f), m_phi(0.0f),
      m_renderMode(RenderTextured)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::StrongFocus);
}

void PlasmaGLWidget::clear()
{
    foreach (RenderData *render, m_drawables)
        delete render;
    m_drawables.clear();

    m_position = QVector3D(0.0f, 0.0f, 0.0f);
    m_theta = 0.0f;
    m_phi = 0.0f;
    updateViewMatrix();
}

void PlasmaGLWidget::addGeometry(plDrawableSpans *spans)
{
    for (size_t grp = 0; grp < spans->getNumBufferGroups(); ++grp) {
        plGBufferGroup *group = spans->getBuffer(grp);
        for (size_t buf = 0; buf < group->getNumVertBuffers(); ++buf) {
            RenderData *render = new RenderData(
                    group->getStride(),
                    group->getIdxBufferCount(buf),
                    (group->getFormat() & plGBufferGroup::kSkinWeightMask) >> 4,
                    group->getFormat() & plGBufferGroup::kUVCountMask,
                    (group->getFormat() & plGBufferGroup::kSkinIndices) != 0
            );

            render->m_vao.create();
            render->m_vao.bind();

            render->m_vBuffer.create();
            render->m_vBuffer.bind();
            render->m_vBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
            render->m_vBuffer.allocate(group->getVertBufferStorage(buf),
                                       group->getVertBufferSize(buf));

            render->m_iBuffer.create();
            render->m_iBuffer.bind();
            render->m_iBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
            render->m_iBuffer.allocate(
                        static_cast<const GLushort *>(group->getIdxBufferStorage(buf)),
                        render->m_indexCount * sizeof(GLushort));

            m_drawables.append(render);
        }
    }
}

void PlasmaGLWidget::setRenderMode(RenderMode mode)
{
    m_renderMode = mode;
    updateGL();
}

void PlasmaGLWidget::initializeGL()
{
    glf.initializeOpenGLFunctions();
    qglClearColor(QColor(0, 255, 255));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    if (!m_shader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vshader.glsl"))
        QMessageBox::warning(this, "Error compiling vshader.glsl", m_shader.log());
    if (!m_shader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fshader.glsl"))
        QMessageBox::warning(this, "Error compiling fshader.glsl", m_shader.log());
    if (!m_shader.link())
        return;
    if (!m_shader.bind())
        return;

    // Shader parameter locations
    sha_position = m_shader.attributeLocation("a_position");
    sha_color = m_shader.attributeLocation("a_color");
    shu_view = m_shader.uniformLocation("u_view");

    // Starting view position
    updateViewMatrix();

    qDebug("OpenGL initialized version: %s; GLSL: %s",
        glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
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

#if !defined(QT_OPENGL_ES_2)
    glPolygonMode(GL_FRONT_AND_BACK, (m_renderMode == RenderWireframe) ? GL_LINE : GL_FILL);
#endif

    foreach (RenderData *render, m_drawables) {
        render->m_vao.bind();
        render->m_vBuffer.bind();
        render->m_iBuffer.bind();

        uintptr_t offset = 0;
        m_shader.enableAttributeArray(sha_position);
        glf.glVertexAttribPointer(sha_position, 3, GL_FLOAT, GL_FALSE, render->m_stride,
                                  reinterpret_cast<GLvoid *>(offset));
        offset += 3 * sizeof(float);

        if (render->m_weights > 0) {
            // Skip weights and skin indices
            offset += sizeof(float) * render->m_weights;
            if (render->m_skinIndices)
                offset += sizeof(int);
        }

        // Normals
        offset += 3 * sizeof(float);

        // Color
        m_shader.enableAttributeArray(sha_color);
        glf.glVertexAttribPointer(sha_color, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                                  render->m_stride, reinterpret_cast<GLvoid *>(offset));
        offset += sizeof(unsigned int);

        // Color 2?
        offset += sizeof(unsigned int);

        for (int u = 0; u < render->m_uvws; ++u) {
            // TODO
            offset += 3 * sizeof(float);
        }

#if defined(QT_OPENGL_ES_2)
        if (m_renderMode == RenderWireframe) {
            // This is kinda slow, but it works with our triangle-based index data...
            for (GLsizei i = 0; i < render->m_indexCount; i += 3) {
                glDrawElements(GL_LINE_LOOP, 3, GL_UNSIGNED_SHORT,
                               reinterpret_cast<GLvoid *>(i * sizeof(GLushort)));
            }
        } else {
            glDrawElements(GL_TRIANGLES, render->m_indexCount,
                           GL_UNSIGNED_SHORT, nullptr);
        }
#else
        glDrawElements(GL_TRIANGLES, render->m_indexCount,
                       GL_UNSIGNED_SHORT, nullptr);
#endif
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
        m_position.setX(m_position.x() - qSin((m_theta + 90.0f) * s_degPerRad) * 2.0f);
        m_position.setY(m_position.y() - qCos((m_theta + 90.0f) * s_degPerRad) * 2.0f);
        break;
    case Qt::Key_D:
        m_position.setX(m_position.x() + qSin((m_theta + 90.0f) * s_degPerRad) * 2.0f);
        m_position.setY(m_position.y() + qCos((m_theta + 90.0f) * s_degPerRad) * 2.0f);
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

    updateViewMatrix();
    updateGL();
}

void PlasmaGLWidget::mousePressEvent(QMouseEvent *event)
{
    m_mousePos = event->pos();
    setCursor(QCursor(Qt::BlankCursor));
}

void PlasmaGLWidget::mouseReleaseEvent(QMouseEvent *)
{
    setCursor(QCursor(Qt::ArrowCursor));
}

void PlasmaGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        // Flat Movement
        float delta = (m_mousePos.y() - event->pos().y()) * .5f;
        m_position.setX(m_position.x() + qSin(m_theta * s_degPerRad) * delta);
        m_position.setY(m_position.y() + qCos(m_theta * s_degPerRad) * delta);
        delta = (event->pos().x() - m_mousePos.x()) * .5f;
        m_position.setX(m_position.x() + qSin((m_theta + 90.0f) * s_degPerRad) * delta);
        m_position.setY(m_position.y() + qCos((m_theta + 90.0f) * s_degPerRad) * delta);
    } else if (event->buttons() == Qt::RightButton) {
        // Look mode
        m_theta += (event->pos().x() - m_mousePos.x()) * .5f;
        m_phi -= (m_mousePos.y() - event->pos().y()) * .5f;
        if (m_phi < -90.0f)
            m_phi = -90.0f;
        else if (m_phi > 90.0f)
            m_phi = 90.0f;
    } else if (event->buttons() == Qt::MiddleButton ||
               (event->buttons() == (Qt::LeftButton | Qt::RightButton))) {
        // Pan mode
        float delta = (event->pos().x() - m_mousePos.x()) * .5f;
        m_position.setZ(m_position.z() + (m_mousePos.y() - event->pos().y()) * .5f);
        m_position.setX(m_position.x() + qSin((m_theta + 90.0f) * s_degPerRad) * delta);
        m_position.setY(m_position.y() + qCos((m_theta + 90.0f) * s_degPerRad) * delta);
    }

    if (event->buttons() != Qt::NoButton && !rect().contains(event->pos())) {
        QCursor::setPos(mapToGlobal(rect().center()));
        m_mousePos = rect().center();
    } else {
        m_mousePos = event->pos();
    }

    updateViewMatrix();
    updateGL();
}

void PlasmaGLWidget::updateViewMatrix()
{
    QMatrix4x4 view;
    view.rotate(-90.0f + m_phi, 1.0f, 0.0f, 0.0f);
    view.rotate(m_theta, 0.0f, 0.0f, 1.0f);
    view.translate(-m_position.x(), -m_position.y(), -m_position.z());
    m_shader.setUniformValue(shu_view, view);
}
