/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qtextstream.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/QOpenGLContext>
#include "qwebosopenglcompositorbackingstore_p.h"

#include "qweboswindow_p.h"

QT_BEGIN_NAMESPACE

QWebOSWindow::QWebOSWindow(QWindow *w)
    : QPlatformWindow(w),
      m_backingStore(0),
      m_raster(false),
      m_winId(0),
      m_frame(new QWebOSWindowFrame(this)),
      m_surface(NULL),
      m_flags(0)
{
}

QWebOSWindow::~QWebOSWindow()
{
    destroy();
    delete m_frame;
}

static WId newWId()
{
    static WId id = 0;

    if (id == std::numeric_limits<WId>::max())
        qWarning("QWEBOSPlatformWindow: Out of window IDs");

    return ++id;
}

void QWebOSWindow::create()
{
    if (m_flags.testFlag(Created))
        return;

    m_winId = newWId();

    // Save the original surface type before changing to OpenGLSurface.
    m_raster = (window()->surfaceType() == QSurface::RasterSurface);
    if (m_raster) // change to OpenGL, but not for RasterGLSurface
        window()->setSurfaceType(QSurface::OpenGLSurface);

    if (window()->type() == Qt::Desktop) {
        QRect fullscreenRect(QPoint(), screen()->availableGeometry().size());
        QPlatformWindow::setGeometry(fullscreenRect);
        QWindowSystemInterface::handleGeometryChange(window(), fullscreenRect);
        return;
    }

    m_flags = Created;

    if (window()->type() == Qt::Desktop)
        return;

    // Stop if there is already a window backed by a native window and surface. Additional
    // raster windows will not have their own native window, surface and context. Instead,
    // they will be composited onto the root window's surface.
    QWebOSScreen *screen = this->screen();
    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    if (screen->primarySurface() != NULL) {
        if (Q_UNLIKELY(!isRaster() || !compositor->targetWindow())) {
            // We can have either a single OpenGL window or multiple raster windows.
            // Other combinations cannot work.
            qFatal("WEBOS: OpenGL windows cannot be mixed with others.");
            return;
        }
        m_format = compositor->targetWindow()->format();
        return;
    }

    m_flags |= HasNativeWindow;
    setGeometry(QRect()); // will become fullscreen

    resetSurface();

    screen->setPrimarySurface(m_surface);

    if (isRaster()) {
        QOpenGLContext *context = new QOpenGLContext(QGuiApplication::instance());
        context->setShareContext(qt_gl_global_share_context());
        context->setFormat(m_format);
        context->setScreen(window()->screen());
        if (Q_UNLIKELY(!context->create()))
            qFatal("WEBOS: Failed to create compositing context");
        compositor->setTarget(context, window(), screen->rawGeometry());
        compositor->setRotation(qEnvironmentVariableIntValue("QT_QPA_WEBOS_ROTATION"));
        // If there is a "root" window into which raster and QOpenGLWidget content is
        // composited, all other contexts must share with its context.
        if (!qt_gl_global_share_context()) {
            qt_gl_set_global_share_context(context);
            // What we set up here is in effect equivalent to the application setting
            // AA_ShareOpenGLContexts. Set the attribute to be fully consistent.
            QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
        }
    }
}

void QWebOSWindow::destroy()
{
    QWebOSScreen *screen = this->screen();
    if (m_flags.testFlag(HasNativeWindow)) {
        if (screen->primarySurface() == m_surface)
            screen->setPrimarySurface(NULL);

        invalidateSurface();
    }

    m_flags = 0;
    QOpenGLCompositor::instance()->removeWindow(this);
}

void QWebOSWindow::invalidateSurface()
{
}

void QWebOSWindow::resetSurface()
{
    m_format = window()->requestedFormat();
    m_surface = (void *) 1;
}

void QWebOSWindow::setVisible(bool visible)
{
    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    QList<QOpenGLCompositorWindow *> windows = compositor->windows();
    QWindow *wnd = window();

    if (wnd->type() != Qt::Desktop) {
        if (visible) {
            compositor->addWindow(this);
        } else {
            compositor->removeWindow(this);
            windows = compositor->windows();
            if (windows.size())
                windows.last()->sourceWindow()->requestActivate();
        }
    }
    QWindowSystemInterface::handleExposeEvent(wnd, QRect(QPoint(0, 0), wnd->geometry().size()));

    if (visible)
        QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
}

void QWebOSWindow::setGeometry(const QRect &r)
{
    QRect rect;
    bool forceFullscreen = m_flags.testFlag(HasNativeWindow);
    if (forceFullscreen)
        rect = screen()->availableGeometry();
    else
        rect = r;

    const bool changed = rect != QPlatformWindow::geometry();
    QPlatformWindow::setGeometry(rect);

    // if we corrected the size, trigger a resize event
    if (rect != r)
        QWindowSystemInterface::handleGeometryChange(window(), rect, r);

    if (changed)
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), rect.size()));
}

QRect QWebOSWindow::geometry() const
{
    // For yet-to-become-fullscreen windows report the geometry covering the entire
    // screen. This is particularly important for Quick where the root object may get
    // sized to some geometry queried before calling create().
    if (!m_flags.testFlag(Created) && screen()->primarySurface() == NULL)
        return screen()->availableGeometry();

    return QPlatformWindow::geometry();
}

void QWebOSWindow::requestActivateWindow()
{
    if (window()->type() != Qt::Desktop)
        QOpenGLCompositor::instance()->moveToTop(this);
    QWindow *wnd = window();
    QWindowSystemInterface::handleWindowActivated(wnd);
    QWindowSystemInterface::handleExposeEvent(wnd, QRect(QPoint(0, 0), wnd->geometry().size()));
}

void QWebOSWindow::raise()
{
    QWindow *wnd = window();
    if (wnd->type() != Qt::Desktop) {
        QOpenGLCompositor::instance()->moveToTop(this);
        QWindowSystemInterface::handleExposeEvent(wnd, QRect(QPoint(0, 0), wnd->geometry().size()));
    }
}

void QWebOSWindow::lower()
{
    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    QList<QOpenGLCompositorWindow *> windows = compositor->windows();
    if (window()->type() != Qt::Desktop && windows.count() > 1) {
        int idx = windows.indexOf(this);
        if (idx > 0) {
            compositor->changeWindowIndex(this, idx - 1);
            QWindowSystemInterface::handleExposeEvent(windows.last()->sourceWindow(),
                                                      QRect(QPoint(0, 0), windows.last()->sourceWindow()->geometry().size()));
        }
    }
}

QSurfaceFormat QWebOSWindow::format() const
{
    return m_format;
}

QWebOSScreen *QWebOSWindow::screen() const
{
    return static_cast<QWebOSScreen *>(QPlatformWindow::screen());
}

bool QWebOSWindow::isRaster() const
{
    return m_raster || window()->surfaceType() == QSurface::RasterGLSurface;
}

QWindow *QWebOSWindow::sourceWindow() const
{
    return window();
}

const QPlatformTextureList *QWebOSWindow::textures() const
{
    if (m_backingStore)
        return m_backingStore->textures();

    return 0;
}

QMargins QWebOSWindow::frameMargins() const
{
    return m_frame->frameMargins();
}

bool QWebOSWindow::handleTouchEventFrame(QWindowSystemInterface::TouchPoint &point) const
{
    m_frame->handleTouchEvent(point);
    QPoint pos(point.area.topLeft().toPoint());
    return (window()->frameGeometry().contains(pos) && !window()->geometry().contains(pos));
}

void QWebOSWindow::drawFrame(QPainter &painter)
{
    m_frame->drawFrame(painter);
}

void QWebOSWindow::endCompositing()
{
    if (m_backingStore)
        m_backingStore->notifyComposited();
}

WId QWebOSWindow::winId() const
{
    return m_winId;
}

void QWebOSWindow::setOpacity(qreal)
{
    if (!isRaster())
        qWarning("QWebOSWindow: Cannot set opacity for non-raster windows");

    // Nothing to do here. The opacity is stored in the QWindow.
}

QT_END_NAMESPACE
