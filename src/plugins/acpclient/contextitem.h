// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QImage>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QIcon;
QT_END_NAMESPACE

namespace AcpClient::Internal {

class ContextItem : public QWidget
{
    Q_OBJECT

public:
    explicit ContextItem(const QString &text, const QIcon &icon, QWidget *parent = nullptr,
                         bool withBorder = true, int iconSize = 16);

    void setRemovable(bool removable);
    void setPreviewImage(const QImage &image);

signals:
    void removeRequested();
    void clicked();

protected:
    bool event(QEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *) override;

private:
    void buildPreviewTooltip();

    QWidget *m_closeButton = nullptr;
    bool m_withBorder = true;
    QImage m_previewImage;
    bool m_previewBuilt = false;
};

} // namespace AcpClient::Internal
