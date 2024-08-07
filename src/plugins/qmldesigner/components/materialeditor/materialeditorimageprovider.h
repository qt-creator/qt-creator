// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QQuickImageProvider>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace QmlDesigner {

class MaterialEditorView;

class MaterialEditorImageProvider : public QQuickImageProvider
{
    Q_OBJECT

public:
    explicit MaterialEditorImageProvider(MaterialEditorView *materialEditorView);

    void setPixmap(const QPixmap &pixmap);
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

signals:
    void requestPreview(QSize);

private:
    void setRequestedSize(const QSize &requestedSize);

    QPixmap m_previewPixmap;
    QSize m_requestedSize;
    QTimer *m_delayTimer = nullptr; // Delays the preview requests
};

} // namespace QmlDesigner
