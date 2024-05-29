// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace ExtensionManager::Internal {

class ExtensionsBrowser final : public QWidget
{
    Q_OBJECT

public:
    ExtensionsBrowser(QWidget *parent = nullptr);
    ~ExtensionsBrowser();

    void adjustToWidth(const int width);
    QSize sizeHint() const override;

    int extraListViewWidth() const; // Space for scrollbar, etc.

signals:
    void itemSelected(const QModelIndex &current, const QModelIndex &previous);

private:
    void fetchExtensions();

    class ExtensionsBrowserPrivate *d = nullptr;
};

} // ExtensionManager::Internal
