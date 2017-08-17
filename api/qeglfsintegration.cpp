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
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QScreen>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QWindow>
#include <QtCore/QLoggingCategory>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatforminputcontextfactory_p.h>

#include "qeglfsintegration_p.h"
#include "qeglfswindow_p.h"
#include "qeglfscontext_p.h"
#include "qeglfsoffscreenwindow_p.h"

#include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>
#include <QtServiceSupport/private/qgenericunixservices_p.h>
#include <QtThemeSupport/private/qgenericunixthemes_p.h>
#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformCompositorSupport/private/qopenglcompositorbackingstore_p.h>

#include <QtPlatformHeaders/qeglfsfunctions.h>

#include <SDL.h>
#include <PDL.h>

QT_BEGIN_NAMESPACE

QEglFSIntegration::QEglFSIntegration()
    : m_inputContext(0),
      m_screenEventHandler(new QQnxScreenEventHandler(this)),
      m_screenEventThread(0),
      m_fontDb(new QGenericUnixFontDatabase),
      m_services(new QGenericUnixServices)
{
}

void QEglFSIntegration::addScreen(QPlatformScreen *screen, bool isPrimary)
{
    screenAdded(screen, isPrimary);
}

void QEglFSIntegration::removeScreen(QPlatformScreen *screen)
{
    destroyScreen(screen);
}

void QEglFSIntegration::initialize()
{
    PDL_Init(0);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetVideoMode(0, 0, 32, SDL_OPENGL);
    atexit(PDL_Quit);
    atexit(SDL_Quit);
    SDL_EnableUNICODE(true);

    addScreen(new QEglFSScreen(), true);

    m_inputContext = new QQnxInputContext(this);
    m_screenEventThread = new QQnxScreenEventThread(m_screenEventHandler);
    m_screenEventThread->start();
}

void QEglFSIntegration::destroy()
{
    foreach (QWindow *w, qGuiApp->topLevelWindows())
        w->destroy();

    while (!qGuiApp->screens().isEmpty())
        removeScreen(qGuiApp->screens().constLast()->handle());

    SDL_Quit();
    PDL_Quit();
}

QAbstractEventDispatcher *QEglFSIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformServices *QEglFSIntegration::services() const
{
    return m_services.data();
}

QPlatformFontDatabase *QEglFSIntegration::fontDatabase() const
{
    return m_fontDb.data();
}

QPlatformTheme *QEglFSIntegration::createPlatformTheme(const QString &name) const
{
    return QGenericUnixTheme::createUnixTheme(name);
}

QPlatformBackingStore *QEglFSIntegration::createPlatformBackingStore(QWindow *window) const
{
    QOpenGLCompositorBackingStore *bs = new QOpenGLCompositorBackingStore(window);
    if (!window->handle())
        window->create();
    static_cast<QEglFSWindow *>(window->handle())->setBackingStore(bs);
    return bs;
}

QPlatformWindow *QEglFSIntegration::createPlatformWindow(QWindow *window) const
{
    QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
    QEglFSWindow *w = new QEglFSWindow(window);
    w->create();

    // Activate only the window for the primary screen to make input work
    if (window->type() != Qt::ToolTip && window->screen() == QGuiApplication::primaryScreen())
        w->requestActivateWindow();

    return w;
}

QPlatformOpenGLContext *QEglFSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QEglFSContext(context);
}

QPlatformOffscreenSurface *QEglFSIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    QSurfaceFormat fmt = surface->requestedFormat();
    return new QEglFSOffscreenWindow(fmt, surface);
}

bool QEglFSIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return false;
    case OpenGL: return true;
    case ThreadedOpenGL: return false;
    case RasterGLSurface: return true;
    case WindowManagement: return false;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformNativeInterface *QEglFSIntegration::nativeInterface() const
{
    return const_cast<QEglFSIntegration *>(this);
}

void *QEglFSIntegration::nativeResourceForIntegration(const QByteArray &resource)
{
    Q_UNUSED(resource)
    void *result = 0;
    return result;
}

void *QEglFSIntegration::nativeResourceForScreen(const QByteArray &resource, QScreen *)
{
    Q_UNUSED(resource)
    void *result = 0;
    return result;
}

void *QEglFSIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    Q_UNUSED(resource)
    Q_UNUSED(window)
    void *result = 0;
    return result;
}

void *QEglFSIntegration::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    Q_UNUSED(resource)
    Q_UNUSED(context)
    void *result = 0;
    return result;
}

QPlatformNativeInterface::NativeResourceForContextFunction QEglFSIntegration::nativeResourceFunctionForContext(const QByteArray &resource)
{
    Q_UNUSED(resource)
    return 0;
}

QFunctionPointer QEglFSIntegration::platformFunction(const QByteArray &function) const
{
    Q_UNUSED(function)
    return 0;
}

QT_END_NAMESPACE
