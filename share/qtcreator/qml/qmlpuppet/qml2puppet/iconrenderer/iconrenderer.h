/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

#include <designersupportdelegate.h>

QT_BEGIN_NAMESPACE
class QQuickWindow;
class QQuickItem;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class QQuickRenderControl;
class QRhi;
class QRhiTexture;
class QRhiRenderBuffer;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;
#endif
QT_END_NAMESPACE

class IconRenderer : public QObject
{
    Q_OBJECT

public:
    explicit IconRenderer(int size, const QString &filePath, const QString &source);

    void setupRender();

private:
    void startCreateIcon();
    void focusCamera();
    void finishCreateIcon();
    void render(const QString &fileName);
    void resizeContent(int dimensions);
    bool initRhi();

    int m_size = 16;
    QString m_filePath;
    QString m_source;
    QQuickWindow *m_window = nullptr;
    QQuickItem *m_contentItem = nullptr;
    QQuickItem *m_containerItem = nullptr;
    DesignerSupport m_designerSupport;
    bool m_is3D = false;
    int m_focusStep = 0;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QQuickRenderControl *m_renderControl = nullptr;
    QRhi *m_rhi = nullptr;
    QRhiTexture *m_texture = nullptr;
    QRhiRenderBuffer *m_buffer = nullptr;
    QRhiTextureRenderTarget *m_texTarget = nullptr;
    QRhiRenderPassDescriptor *m_rpDesc = nullptr;
#endif
};
