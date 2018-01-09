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

#include "qweboswindowframe_p.h"
#include "qweboswindow_p.h"
#include <QWindow>

QT_BEGIN_NAMESPACE

QWebOSWindowFrame::QWebOSWindowFrame(QWebOSWindow *w)
    : m_window(w),
      m_closeButtonPressed(false),
      m_titleSize(26),
      m_borderSize(2)
{
}

QWebOSWindowFrame::~QWebOSWindowFrame()
{
}

QMargins QWebOSWindowFrame::frameMargins() const
{
    if (hasFrame())
        return QMargins(m_borderSize, m_borderSize + m_titleSize, m_borderSize, m_borderSize);
    return QMargins();
}

QRect QWebOSWindowFrame::closeButtonRect() const
{
    QRect rect = QRect(m_window->sourceWindow()->frameGeometry().topRight(), QSize(-m_titleSize, m_titleSize));
    rect.adjust(-m_borderSize+1, m_borderSize, 1, 0);
    return rect;
}

bool QWebOSWindowFrame::hasFrame() const
{
    Qt::WindowType type = m_window->sourceWindow()->type();
    return (type == Qt::Window || type == Qt::Dialog);
}

void QWebOSWindowFrame::drawFrame(QPainter &painter)
{
    if (!hasFrame()) return;

    painter.fillRect(m_window->sourceWindow()->frameGeometry(), Qt::gray);
    painter.fillRect(closeButtonRect(), m_closeButtonPressed? Qt::darkRed: Qt::red);

    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(3);
    pen.setBrush(Qt::white);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);

    QRect r = closeButtonRect();
    qreal dX = r.width()*0.3;
    qreal dY = r.height()*0.3;
    qreal centerX = r.x() + r.width()*0.5;
    qreal centerY = r.y() + r.height()*0.5;
    painter.drawLine(centerX-dX, centerY-dY, centerX+dX, centerY+dY);
    painter.drawLine(centerX+dX, centerY-dY, centerX-dX, centerY+dY);
}

void QWebOSWindowFrame::handleTouchEvent(QWindowSystemInterface::TouchPoint &point)
{
    if (!hasFrame()) return;

    QPoint pos = point.area.topLeft().toPoint();
    QWindow *w = m_window->sourceWindow();
    if (!closeButtonRect().contains(pos)) {
        if (m_closeButtonPressed) {
            m_closeButtonPressed = false;
            w->requestUpdate();
        }
        return;
    }
    switch (point.state) {
        case Qt::TouchPointPressed:
            m_closeButtonPressed = true;
            w->requestUpdate();
            break;
        case Qt::TouchPointReleased:
            if (m_closeButtonPressed)
                QWindowSystemInterface::handleCloseEvent(w);
            m_closeButtonPressed = false;
            break;
        default: break;
    }
}


QT_END_NAMESPACE
