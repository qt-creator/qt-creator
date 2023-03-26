// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

#include <memory>

QT_BEGIN_NAMESPACE
class QQuickWindow;
class QQuickItem;
class QQuickDesignerSupport;
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
    ~IconRenderer();

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
    std::unique_ptr<QQuickDesignerSupport> m_designerSupport;
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
