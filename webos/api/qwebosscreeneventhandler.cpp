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

#include "qwebosscreeneventhandler_p.h"
#include "qwebosscreeneventthread_p.h"
#include "qwebosintegration_p.h"

#include <QDebug>
#include <QGuiApplication>

#include <errno.h>

#include <QtGui/private/qwindow_p.h>

#if defined(QWEBOSSCREENEVENT_DEBUG)
#define qScreenEventDebug qDebug
#else
#define qScreenEventDebug QT_NO_QDEBUG_MACRO
#endif

#define HP_BT_LEFT 18
#define HP_BT_UP 19
#define HP_BT_RIGHT 20
#define HP_BT_DOWN 21

QT_BEGIN_NAMESPACE

QWebOSScreenEventHandler::QWebOSScreenEventHandler(QWebOSIntegration *integration)
    : m_webosIntegration(integration)
    , m_touchDevice(0)
    , m_eventThread(0)
{
    // Create a touch device
    m_touchDevice = new QTouchDevice;
    m_touchDevice->setType(QTouchDevice::TouchScreen);
    m_touchDevice->setCapabilities(QTouchDevice::Position);
    QWindowSystemInterface::registerTouchDevice(m_touchDevice);

    // initialize array of touch points
    for (int i = 0; i < MaximumTouchPoints; i++) {

        // map array index to id
        m_touchPoints[i].id = i;

        // pressure is not supported - use default
        m_touchPoints[i].pressure = 1.0;

        // nothing touching
        m_touchPoints[i].state = Qt::TouchPointReleased;
    }
}

bool QWebOSScreenEventHandler::handleEvent(SDL_Event event)
{
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONUP:
        handleTouchEvent(event);
        break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
        handleKeyboardEvent(event);
        break;
    case SDL_ACTIVEEVENT:
        handleActiveEvent(event);
        break;

    default:
        // event ignored
        qScreenEventDebug() << Q_FUNC_INFO << "unknown event" << event.type;
        return false;
    }

    return true;
}

void QWebOSScreenEventHandler::injectKeyboardEvent(SDL_Event event)
{
    // determine event type
    QEvent::Type type = (event.type == SDL_KEYUP) ? QEvent::KeyPress : QEvent::KeyRelease;
    int keyToPass = event.key.keysym.unicode;
    if (keyToPass == PDLK_GESTURE_DISMISS_KEYBOARD) {
        clearCurrentFocusObject();
        return;
    }
    keyToPass = handleSpecialKeys(event.key.keysym.sym, keyToPass);
    qScreenEventDebug() << Q_FUNC_INFO << "Qt key t=" << type << ", k=" << keyToPass << ", s=" << event.key.keysym.unicode;
    QWindowSystemInterface::handleKeyEvent(QGuiApplication::focusWindow(), type, keyToPass, (event.key.keysym.mod == KMOD_LSHIFT) ? Qt::ShiftModifier :Qt::NoModifier, QChar(event.key.keysym.unicode), false  ) ;
}

void QWebOSScreenEventHandler::clearCurrentFocusObject()
{
    if (QWindow *focusWindow = QGuiApplication::focusWindow())
        static_cast<QWindowPrivate *>(QObjectPrivate::get(focusWindow))->clearFocusObject();
}

void QWebOSScreenEventHandler::setScreenEventThread(QWebOSScreenEventThread *eventThread)
{
    m_eventThread = eventThread;
}

void QWebOSScreenEventHandler::processEventsFromScreenThread()
{
    if (!m_eventThread)
        return;

    QWebOSScreenEventArray *events = m_eventThread->lock();

    for (int i = 0; i < events->size(); ++i) {
        SDL_Event event = events->at(i);
        if (event.type == SDL_NOEVENT)
            continue;
        (*events)[i].type = SDL_NOEVENT;

        m_eventThread->unlock();

        handleEvent(event);

        m_eventThread->lock();
    }

    events->clear();

    m_eventThread->unlock();
}

void QWebOSScreenEventHandler::handleKeyboardEvent(SDL_Event event)
{
    injectKeyboardEvent(event);
}

void QWebOSScreenEventHandler::handleTouchEvent(SDL_Event event)
{
    QPoint pos;
    int touchId = MaximumTouchPoints;
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        pos = QPoint(event.button.x, event.button.y);
        touchId = event.button.which;
        break;
    case SDL_MOUSEMOTION:
        pos = QPoint(event.motion.x, event.motion.y);
        touchId = event.motion.which;
        if (event.motion.state == SDL_RELEASED) {
            touchId = MaximumTouchPoints;
        }
        break;
    }

    // check if finger is valid
    if (touchId < MaximumTouchPoints) {

        // Map window handle to top-level QWindow
        QWindow *w = QGuiApplication::focusWindow();
        if (!w)
            w = QGuiApplication::topLevelAt(pos);

        if (w) {
            m_touchPoints[touchId].area = QRectF(pos.x(), pos.y(), 0, 0);
            // determine event type and update state of current touch point
            QEvent::Type type = QEvent::None;
            switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                m_touchPoints[touchId].state = Qt::TouchPointPressed;
                type = QEvent::TouchBegin;
                break;
            case SDL_MOUSEMOTION:
                m_touchPoints[touchId].state = Qt::TouchPointMoved;
                type = QEvent::TouchUpdate;
                break;
            case SDL_MOUSEBUTTONUP:
                m_touchPoints[touchId].state = Qt::TouchPointReleased;
                type = QEvent::TouchEnd;
                break;
            }

            // build list of active touch points
            QList<QWindowSystemInterface::TouchPoint> pointList;
            for (int i = 0; i < MaximumTouchPoints; i++) {
                if (i == touchId) {
                    // current touch point is always active
                    pointList.append(m_touchPoints[i]);
                } else if (m_touchPoints[i].state != Qt::TouchPointReleased) {
                    // finger is down but did not move
                    m_touchPoints[i].state = Qt::TouchPointStationary;
                    pointList.append(m_touchPoints[i]);
                }
            }

            // inject event into Qt
            QWindowSystemInterface::handleTouchEvent(w, m_touchDevice, pointList);
            qScreenEventDebug() << "Qt touch, w =" << w
                                << ", p=" << m_touchPoints[touchId].area.topLeft()
                                << ", t=" << type;
        }
    }
}

void QWebOSScreenEventHandler::handleActiveEvent(SDL_Event event)
{
    if (event.active.state == SDL_APPACTIVE) {
        Qt::ApplicationState newState = (event.active.gain)? Qt::ApplicationActive : Qt::ApplicationInactive;
        QWindowSystemInterface::handleApplicationStateChanged(newState);
    }
}
int QWebOSScreenEventHandler::handleSpecialKeys(SDLKey key, int def)
{
  // Special-case misc keys
  switch ((int)key)
  {
    case SDLK_TAB:
      return Qt::Key_Tab;
    case SDLK_RETURN:
      return Qt::Key_Enter;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
      return Qt::Key_Control;
    case SDLK_LALT:
    case SDLK_RALT:
      return Qt::Key_Alt;
    case HP_BT_LEFT:
    case SDLK_LEFT:
      return Qt::Key_Left;
    case HP_BT_UP:
    case SDLK_UP:
      return Qt::Key_Up;
    case HP_BT_RIGHT:
    case SDLK_RIGHT:
      return Qt::Key_Right;
    case HP_BT_DOWN:
    case SDLK_DOWN:
      return Qt::Key_Down;
    case SDLK_BACKSPACE:
      return Qt::Key_Backspace;
    default:
      return def;
  }
}

QT_END_NAMESPACE
