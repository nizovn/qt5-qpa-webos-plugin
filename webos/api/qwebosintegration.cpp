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

#include "qwebosintegration_p.h"
#include "qweboswindow_p.h"
#include "qwebosglcontext_p.h"
#include "qwebosoffscreenwindow_p.h"
#include "qwebosservices_p.h"

#include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>
#include <QtThemeSupport/private/qgenericunixthemes_p.h>
#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformCompositorSupport/private/qopenglcompositorbackingstore_p.h>

#include <SDL.h>
#include <PDL.h>

QT_BEGIN_NAMESPACE

QWebOSIntegration::QWebOSIntegration()
    : m_inputContext(0),
      m_screenEventHandler(new QWebOSScreenEventHandler(this)),
      m_screenEventThread(0),
      m_fontDb(new QGenericUnixFontDatabase),
      m_services(new QWebOSServices)
{
}

void QWebOSIntegration::addScreen(QPlatformScreen *screen, bool isPrimary)
{
    screenAdded(screen, isPrimary);
}

void QWebOSIntegration::removeScreen(QPlatformScreen *screen)
{
    destroyScreen(screen);
}

void QWebOSIntegration::initialize()
{
    PDL_Init(0);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_SetVideoMode(0, 0, 32, SDL_OPENGL);
    atexit(PDL_Quit);
    atexit(SDL_Quit);

    SDL_EnableUNICODE(true);
    if (!qEnvironmentVariableIsEmpty("QT_QPA_WEBOS_SCREEN_TIMEOUT_ENABLED"))
        PDL_ScreenTimeoutEnable(PDL_TRUE);

    addScreen(new QWebOSScreen(), true);
    int sdlDelay = 100;
    sdlDelay = qEnvironmentVariableIntValue("QT_QPA_WEBOS_EVENT_DELAY");
    if (sdlDelay < 10)
        sdlDelay = 10;

    m_sdlEventsPaused = false;
    m_sdlEventsNeedUpdate = false;
    m_sdlEventsTimer = new QTimer(this);
    m_sdlEventsTimer->setInterval(sdlDelay);
    connect(m_sdlEventsTimer, &QTimer::timeout, this, &QWebOSIntegration::processSDLEvents);
    m_sdlEventsTimer->start();

    m_inputContext = new QWebOSInputContext(this);
    m_screenEventThread = new QWebOSScreenEventThread(m_screenEventHandler, sdlDelay);
    m_screenEventThread->start();
}

void QWebOSIntegration::pauseSDLEvents()
{
    m_sdlEventsPaused = true;
}

void QWebOSIntegration::resumeSDLEvents()
{
    m_sdlEventsPaused = false;
    if (m_sdlEventsNeedUpdate)
        processSDLEvents();
    m_sdlEventsNeedUpdate = false;
    m_sdlEventsTimer->start();
}

void QWebOSIntegration::processSDLEvents()
{
    if (!m_sdlEventsPaused)
        SDL_PumpEvents();
    else
        m_sdlEventsNeedUpdate = false;
}

void QWebOSIntegration::destroy()
{
    foreach (QWindow *w, qGuiApp->topLevelWindows())
        w->destroy();

    while (!qGuiApp->screens().isEmpty())
        removeScreen(qGuiApp->screens().constLast()->handle());

    SDL_Quit();
    PDL_Quit();
}

QAbstractEventDispatcher *QWebOSIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformServices *QWebOSIntegration::services() const
{
    return m_services.data();
}

QPlatformFontDatabase *QWebOSIntegration::fontDatabase() const
{
    return m_fontDb.data();
}

QPlatformTheme *QWebOSIntegration::createPlatformTheme(const QString &name) const
{
    return QGenericUnixTheme::createUnixTheme(name);
}

QPlatformBackingStore *QWebOSIntegration::createPlatformBackingStore(QWindow *window) const
{
    QOpenGLCompositorBackingStore *bs = new QOpenGLCompositorBackingStore(window);
    if (!window->handle())
        window->create();
    static_cast<QWebOSWindow *>(window->handle())->setBackingStore(bs);
    return bs;
}

QPlatformWindow *QWebOSIntegration::createPlatformWindow(QWindow *window) const
{
    QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
    QWebOSWindow *w = new QWebOSWindow(window);
    w->create();

    // Activate only the window for the primary screen to make input work
    if (window->type() != Qt::ToolTip && window->screen() == QGuiApplication::primaryScreen())
        w->requestActivateWindow();

    return w;
}

QPlatformOpenGLContext *QWebOSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QWebOSGLContext(context);
}

QPlatformOffscreenSurface *QWebOSIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    QSurfaceFormat fmt = surface->requestedFormat();
    return new QWebOSOffscreenWindow(fmt, surface);
}

bool QWebOSIntegration::hasCapability(QPlatformIntegration::Capability cap) const
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

QPlatformNativeInterface *QWebOSIntegration::nativeInterface() const
{
    return const_cast<QWebOSIntegration *>(this);
}

void *QWebOSIntegration::nativeResourceForIntegration(const QByteArray &resource)
{
    Q_UNUSED(resource)
    void *result = 0;
    return result;
}

void *QWebOSIntegration::nativeResourceForScreen(const QByteArray &resource, QScreen *)
{
    Q_UNUSED(resource)
    void *result = 0;
    return result;
}

void *QWebOSIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    Q_UNUSED(resource)
    Q_UNUSED(window)
    void *result = 0;
    return result;
}

void *QWebOSIntegration::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    Q_UNUSED(resource)
    Q_UNUSED(context)
    void *result = 0;
    return result;
}

QPlatformNativeInterface::NativeResourceForContextFunction QWebOSIntegration::nativeResourceFunctionForContext(const QByteArray &resource)
{
    Q_UNUSED(resource)
    return 0;
}

QFunctionPointer QWebOSIntegration::platformFunction(const QByteArray &function) const
{
    Q_UNUSED(function)
    return 0;
}

QT_END_NAMESPACE
