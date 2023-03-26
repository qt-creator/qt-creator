// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QQuickImageProvider>

namespace StudioWelcome {

namespace Internal {

class NewProjectDialogImageProvider : public QQuickImageProvider
{
public:
    NewProjectDialogImageProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QPixmap requestStatusPixmap(const QString &id, QSize *size, const QSize &requestedSize);
    QPixmap requestStylePixmap(const QString &id, QSize *size, const QSize &requestedSize);
    QPixmap requestDefaultPixmap(const QString &id, QSize *size, const QSize &requestedSize);

    static QPixmap invalidStyleIcon();
};

} // namespace Internal

} // namespace StudioWelcome
