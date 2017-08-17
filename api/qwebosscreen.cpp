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
#include <QtGui/qwindow.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtPlatformCompositorSupport/private/qopenglcompositor_p.h>
#include <private/qmath_p.h>

#include "qwebosscreen_p.h"
#include "qweboswindow_p.h"

#include <SDL.h>

QT_BEGIN_NAMESPACE

QWebOSScreen::QWebOSScreen()
    : m_surface(NULL)
{
}

QWebOSScreen::~QWebOSScreen()
{
    QOpenGLCompositor::destroy();
}

QRect QWebOSScreen::geometry() const
{
    QRect r = rawGeometry();

    static int rotation = qEnvironmentVariableIntValue("QT_QPA_WEBOS_ROTATION");
    switch (rotation) {
    case 0:
    case 180:
    case -180:
        break;
    case 90:
    case -90: {
        int h = r.height();
        r.setHeight(r.width());
        r.setWidth(h);
        break;
    }
    default:
        qWarning("Invalid rotation %d specified in QT_QPA_WEBOS_ROTATION", rotation);
        break;
    }

    return r;
}

QRect QWebOSScreen::rawGeometry() const
{
    const SDL_VideoInfo* info = SDL_GetVideoInfo();
    const int defaultWidth = info->current_w;
    const int defaultHeight = info->current_h;

    static QSize size;

    if (size.isEmpty()) {
        int width = qEnvironmentVariableIntValue("QT_QPA_WEBOS_WIDTH");
        int height = qEnvironmentVariableIntValue("QT_QPA_WEBOS_HEIGHT");

        if (width && height) {
            size.setWidth(width);
            size.setHeight(height);
        }
        else {
            size.setWidth(defaultWidth);
            size.setHeight(defaultHeight);
        }
    }
    return QRect(QPoint(0, 0), size);
}

int QWebOSScreen::depth() const
{
    const SDL_VideoInfo* info = SDL_GetVideoInfo();
    const int defaultDepth = info->vfmt->BitsPerPixel;
    static int depth = qEnvironmentVariableIntValue("QT_QPA_WEBOS_DEPTH");

    if (depth == 0) {
        depth = defaultDepth;
    }
    return depth;
}

QImage::Format QWebOSScreen::format() const
{
    return depth() == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
}

QSizeF QWebOSScreen::physicalSize() const
{
    const int defaultPhysicalDpi = 100;
    static QSizeF size;

    if (size.isEmpty()) {
        // Note: in millimeters
        int width = qEnvironmentVariableIntValue("QT_QPA_WEBOS_PHYSICAL_WIDTH");
        int height = qEnvironmentVariableIntValue("QT_QPA_WEBOS_PHYSICAL_HEIGHT");

        if (width && height) {
            size.setWidth(width);
            size.setHeight(height);
        }
        else {
            size.setWidth(screen()->size().width() * Q_MM_PER_INCH / defaultPhysicalDpi);
            size.setHeight(screen()->size().height() * Q_MM_PER_INCH / defaultPhysicalDpi);
            qWarning("Unable to query physical screen size, defaulting to %d dpi.\n"
                     "To override, set QT_QPA_WEBOS_PHYSICAL_WIDTH "
                     "and QT_QPA_WEBOS_PHYSICAL_HEIGHT (in millimeters).", defaultPhysicalDpi);
        }
    }

    return size;
}

QDpi QWebOSScreen::logicalDpi() const
{
    const QSizeF ps = physicalSize();
    const QSize s = screen()->size();

    if (!ps.isEmpty() && !s.isEmpty())
        return QDpi(25.4 * s.width() / ps.width(),
                    25.4 * s.height() / ps.height());
    else
        return QDpi(100, 100);
}

qreal QWebOSScreen::pixelDensity() const
{
    return qMax(1, qRound(logicalDpi().first / qreal(100)));
}

Qt::ScreenOrientation QWebOSScreen::nativeOrientation() const
{
    return Qt::PrimaryOrientation;
}

Qt::ScreenOrientation QWebOSScreen::orientation() const
{
    return Qt::PrimaryOrientation;
}

qreal QWebOSScreen::refreshRate() const
{
    return 60;
}

void QWebOSScreen::setPrimarySurface(void *surface)
{
    m_surface = surface;
}

QPixmap QWebOSScreen::grabWindow(WId wid, int x, int y, int width, int height) const
{
    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    const QList<QOpenGLCompositorWindow *> windows = compositor->windows();
    Q_ASSERT(!windows.isEmpty());

    QImage img;

    if (static_cast<QWebOSWindow *>(windows.first()->sourceWindow()->handle())->isRaster()) {
        // Request the compositor to render everything into an FBO and read it back. This
        // is of course slow, but it's safe and reliable. It will not include the mouse
        // cursor, which is a plus.
        img = compositor->grab();
    } else {
        // Just a single OpenGL window without compositing. Do not support this case for now. Doing
        // glReadPixels is not an option since it would read from the back buffer which may have
        // undefined content when calling right after a swapBuffers (unless preserved swap is
        // available and enabled, but we have no support for that).
        qWarning("grabWindow: Not supported for non-composited OpenGL content. Use QQuickWindow::grabWindow() instead.");
        return QPixmap();
    }

    if (!wid) {
        const QSize screenSize = geometry().size();
        if (width < 0)
            width = screenSize.width() - x;
        if (height < 0)
            height = screenSize.height() - y;
        return QPixmap::fromImage(img).copy(x, y, width, height);
    }

    foreach (QOpenGLCompositorWindow *w, windows) {
        const QWindow *window = w->sourceWindow();
        if (window->winId() == wid) {
            const QRect geom = window->geometry();
            if (width < 0)
                width = geom.width() - x;
            if (height < 0)
                height = geom.height() - y;
            QRect rect(geom.topLeft() + QPoint(x, y), QSize(width, height));
            rect &= window->geometry();
            return QPixmap::fromImage(img).copy(rect);
        }
    }
    return QPixmap();
}

QT_END_NAMESPACE
