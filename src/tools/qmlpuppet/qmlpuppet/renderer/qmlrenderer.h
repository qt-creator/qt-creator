// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmlbase.h"

#include <rhi/qrhi.h>

#include <private/qquickrendercontrol_p.h>
#include <private/qquickrendertarget_p.h>

#include <QQmlEngine>
#include <QQuickWindow>

QT_BEGIN_NAMESPACE
class QQuickItem;
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

    void setRenderSize(QSize size);

    QStringList m_importPaths;
    QSize m_reqMinSize;
    QSize m_reqMaxSize;
    QSize m_renderSize;
    QString m_sourceFile;
    QString m_outFile;
    bool m_verbose = false;
    bool m_is3D = false;
    bool m_fit3D = false;
    bool m_isLibIcon = false;

    QQuickItem *m_containerItem = nullptr;
    QRhi *m_rhi = nullptr;

    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QQuickRenderControl> m_renderControl;
    std::unique_ptr<QQuickWindow> m_window;
    std::unique_ptr<QObject> m_helper;
    std::unique_ptr<QRhiTexture> m_texture;
    std::unique_ptr<QRhiRenderBuffer> m_buffer;
    std::unique_ptr<QRhiTextureRenderTarget> m_texTarget;
    std::unique_ptr<QRhiRenderPassDescriptor> m_rpDesc;
};
