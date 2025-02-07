// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmlbase.h"

QT_BEGIN_NAMESPACE
class QQuickItem;
class QQuickRenderControl;
class QQuickWindow;
class QRhi;
class QRhiRenderBuffer;
class QRhiRenderPassDescriptor;
class QRhiTexture;
class QRhiTextureRenderTarget;
QT_END_NAMESPACE

class QmlRenderer : public QmlBase
{
    using QmlBase::QmlBase;

private:
    void initCoreApp() override;
    void populateParser() override;
    void initQmlRunner() override;

    bool setupRenderer();
    bool initRhi();
    void render();

    void info(const QString &msg);
    void error(const QString &msg);
    void asyncQuit(int errorCode);

    QStringList m_importPaths;
    QSize m_requestedSize;
    QSize m_renderSize;
    QString m_sourceFile;
    QString m_outFile;
    bool m_verbose = false;
    bool m_is3D = false;
    bool m_fit3D = false;

    QQuickWindow *m_window = nullptr;
    QQuickItem *m_containerItem = nullptr;

    QQuickRenderControl *m_renderControl = nullptr;
    QRhi *m_rhi = nullptr;
    QRhiTexture *m_texture = nullptr;
    QRhiRenderBuffer *m_buffer = nullptr;
    QRhiTextureRenderTarget *m_texTarget = nullptr;
    QRhiRenderPassDescriptor *m_rpDesc = nullptr;
};
