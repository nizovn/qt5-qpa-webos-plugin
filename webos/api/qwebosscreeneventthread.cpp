/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#include "qwebosscreeneventthread_p.h"
#include "qwebosscreeneventhandler_p.h"

#include <QtCore/QDebug>

#include <errno.h>
#include <unistd.h>

#include <cctype>
#include <QGuiApplication>

#if defined(QWEBOSSCREENEVENTTHREAD_DEBUG)
#define qScreenEventThreadDebug qDebug
#else
#define qScreenEventThreadDebug QT_NO_QDEBUG_MACRO
#endif

QWebOSScreenEventThread::QWebOSScreenEventThread(QWebOSScreenEventHandler *screenEventHandler, int sdlDelay)
    : QThread(),
      m_screenEventHandler(screenEventHandler),
      m_sdlDelay(sdlDelay),
      m_quit(false)
{
    screenEventHandler->setScreenEventThread(this);
    connect(this, SIGNAL(eventPending()), screenEventHandler, SLOT(processEventsFromScreenThread()), Qt::QueuedConnection);
    connect(this, SIGNAL(finished()), screenEventHandler, SLOT(processEventsFromScreenThread()), Qt::QueuedConnection);
}

QWebOSScreenEventThread::~QWebOSScreenEventThread()
{
    // block until thread terminates
    shutdown();
}

QWebOSScreenEventArray *QWebOSScreenEventThread::lock()
{
    m_mutex.lock();
    return &m_events;
}

void QWebOSScreenEventThread::unlock()
{
    m_mutex.unlock();
}

void QWebOSScreenEventThread::run()
{
    qScreenEventThreadDebug("screen event thread started");

    SDL_Event sdl_events[16];
    int num;

    // loop indefinitely
    while (!m_quit) {
        SDL_Delay(m_sdlDelay);
        while ( (!m_quit) && ((num = SDL_PeepEvents(sdl_events, 16, SDL_GETEVENT, SDL_ALLEVENTS)) > 0) ) {
            for (int i = 0; i < num; i++) {
                if (sdl_events[i].type == SDL_QUIT) {
                    qScreenEventThreadDebug() << Q_FUNC_INFO << "WEBOS user screen event";
                    m_quit = true;
                    break;
                }
                else {
                    m_mutex.lock();
                    m_events << sdl_events[i];
                    m_mutex.unlock();
                }
            }
            emit eventPending();
        }
    }

    qScreenEventThreadDebug("screen event thread stopped");

    // cleanup
    m_mutex.lock();
    m_events.clear();
    m_mutex.unlock();

    foreach (QWindow *w, QGuiApplication::topLevelWindows())
        QWindowSystemInterface::handleCloseEvent(w);
    QGuiApplication::quit();
}

void QWebOSScreenEventThread::shutdown()
{
    qScreenEventThreadDebug("screen event thread shutdown begin");

    // block until thread terminates
    wait();

    qScreenEventThreadDebug("screen event thread shutdown end");
}
