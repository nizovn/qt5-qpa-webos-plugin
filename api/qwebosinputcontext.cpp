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

#include "qwebosinputcontext_p.h"
#include "qwebosintegration_p.h"

#include <QtCore/QDebug>
#include <QtGui/QGuiApplication>
#include <QtGui/QInputMethodEvent>

#include <PDL.h>

#if defined(QWEBOSINPUTCONTEXT_DEBUG)
#define qInputContextDebug qDebug
#else
#define qInputContextDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QWebOSInputContext::QWebOSInputContext(QWebOSIntegration *integration) :
    QPlatformInputContext(),
    m_inputPanelVisible(false),
    m_integration(integration)
{
    keyboardVisibilityChanged(false);
}

QWebOSInputContext::~QWebOSInputContext()
{
}

bool QWebOSInputContext::isValid() const
{
    return true;
}

bool QWebOSInputContext::hasPhysicalKeyboard()
{
    // TODO: This should query the system to check if a USB keyboard is connected.
    return false;
}

void QWebOSInputContext::reset()
{
}

bool QWebOSInputContext::filterEvent( const QEvent *event )
{
    if (hasPhysicalKeyboard())
        return false;

    if (event->type() == QEvent::CloseSoftwareInputPanel) {
        hideInputPanel();
        qInputContextDebug("hiding virtual keyboard");
        return false;
    }

    if (event->type() == QEvent::RequestSoftwareInputPanel) {
        showInputPanel();
        qInputContextDebug("requesting virtual keyboard");
        return false;
    }

    return false;

}

QRectF QWebOSInputContext::keyboardRect() const
{
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    return QRectF(screenGeometry.x(), screenGeometry.height() - 100,
                  screenGeometry.width(), 100);
}

bool QWebOSInputContext::handleKeyboardEvent(int flags, int sym, int mod, int scan, int cap)
{
    Q_UNUSED(flags);
    Q_UNUSED(sym);
    Q_UNUSED(mod);
    Q_UNUSED(scan);
    Q_UNUSED(cap);
    return false;
}

void QWebOSInputContext::showInputPanel()
{
    qInputContextDebug();
    if (!m_inputPanelVisible) {
        PDL_SetKeyboardState(PDL_TRUE);
        m_inputPanelVisible = true;
    }
}

void QWebOSInputContext::hideInputPanel()
{
    qInputContextDebug();
    if (m_inputPanelVisible) {
        PDL_SetKeyboardState(PDL_FALSE);
        m_inputPanelVisible = false;
    }
}

bool QWebOSInputContext::isInputPanelVisible() const
{
    return m_inputPanelVisible;
}

void QWebOSInputContext::keyboardHeightChanged()
{
    emitKeyboardRectChanged();
}

void QWebOSInputContext::keyboardVisibilityChanged(bool visible)
{
    qInputContextDebug() << "visible=" << visible;
    if (m_inputPanelVisible != visible) {
        m_inputPanelVisible = visible;
        emitInputPanelVisibleChanged();
    }
}

void QWebOSInputContext::setFocusObject(QObject *object)
{
    qInputContextDebug() << "input item=" << object;

    if (!inputMethodAccepted()) {
        if (m_inputPanelVisible)
            hideInputPanel();
    } else {
        if (!m_inputPanelVisible)
            showInputPanel();
    }
}

QT_END_NAMESPACE
