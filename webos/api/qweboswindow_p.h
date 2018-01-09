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

#ifndef QWEBOSWINDOW_H
#define QWEBOSWINDOW_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qwebosglobal_p.h"
#include "qwebosintegration_p.h"
#include "qwebosscreen_p.h"
#include "qweboswindowframe_p.h"

#include <qpa/qplatformwindow.h>
#include "qwebosopenglcompositor_p.h"

QT_BEGIN_NAMESPACE

class QOpenGLCompositorBackingStore;
class QPlatformTextureList;
class Q_WEBOS_EXPORT QWebOSWindow : public QPlatformWindow, public QOpenGLCompositorWindow
{
public:
    QWebOSWindow(QWindow *w);
    ~QWebOSWindow();

    void create();
    void destroy();

    void setGeometry(const QRect &) override;
    QRect geometry() const override;
    QMargins frameMargins() const override;
    void setVisible(bool visible) override;
    void requestActivateWindow() override;
    void raise() override;
    void lower() override;

    void propagateSizeHints() override { }
    void setMask(const QRegion &) override { }
    bool setKeyboardGrabEnabled(bool) override { return false; }
    bool setMouseGrabEnabled(bool) override { return false; }
    void setOpacity(qreal) override;
    WId winId() const override;

    QSurfaceFormat format() const override;

    QWebOSScreen *screen() const;

    bool hasNativeWindow() const { return m_flags.testFlag(HasNativeWindow); }

    void invalidateSurface() override;
    virtual void resetSurface();

    QOpenGLCompositorBackingStore *backingStore() { return m_backingStore; }
    void setBackingStore(QOpenGLCompositorBackingStore *backingStore) { m_backingStore = backingStore; }
    bool isRaster() const;
    QWindow *sourceWindow() const override;
    const QPlatformTextureList *textures() const override;
    void endCompositing() override;
    void drawFrame(QPainter &painter) override;
    bool handleTouchEventFrame(QWindowSystemInterface::TouchPoint &point) const;

protected:
    QOpenGLCompositorBackingStore *m_backingStore;
    bool m_raster;
    WId m_winId;
    QWebOSWindowFrame *m_frame;

    void *m_surface;

    QSurfaceFormat m_format;

    enum Flag {
        Created = 0x01,
        HasNativeWindow = 0x02
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Flags m_flags;
};

QT_END_NAMESPACE

#endif // QWEBOSWINDOW_H
