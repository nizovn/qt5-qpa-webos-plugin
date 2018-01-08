/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#ifndef QWEBOSSCREENEVENTHANDLER_H
#define QWEBOSSCREENEVENTHANDLER_H

#include <qpa/qwindowsysteminterface.h>

#include <SDL.h>
#include <PDL.h>
#include <QTimer>

QT_BEGIN_NAMESPACE

class QWebOSIntegration;
class QWebOSScreenEventThread;

class QWebOSScreenEventHandler : public QObject
{
    Q_OBJECT
public:
    explicit QWebOSScreenEventHandler(QWebOSIntegration *integration);

    bool handleEvent(SDL_Event event);

    static void injectKeyboardEvent(SDL_Event event);
    static void clearCurrentFocusObject();
    static int handleSpecialKeys(SDLKey key, int def);

    void setScreenEventThread(QWebOSScreenEventThread *eventThread);

private Q_SLOTS:
    void processEventsFromScreenThread();

private:
    void handleKeyboardEvent(SDL_Event event);
    void handleTouchEvent(SDL_Event event);
    void handleActiveEvent(SDL_Event event);
    void handleLongTap(int touchId);

private:
    enum {
        MaximumTouchPoints = 5
    };

    QWebOSIntegration *m_webosIntegration;
    QTouchDevice *m_touchDevice;
    QWindowSystemInterface::TouchPoint m_touchPoints[MaximumTouchPoints];
    QPoint m_touchPointsStartPos[MaximumTouchPoints];
    bool m_touchIgnoreEvents[MaximumTouchPoints];
    QTimer m_touchTimers[MaximumTouchPoints];
    QWebOSScreenEventThread *m_eventThread;
};

QT_END_NAMESPACE

#endif // QWEBOSSCREENEVENTHANDLER_H
