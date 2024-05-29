// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <coreplugin/welcomepagehelper.h>

namespace ExtensionManager::Internal {

class ExtensionManagerWidget final : public Core::ResizeSignallingWidget
{
public:
    explicit ExtensionManagerWidget(QWidget *parent = nullptr);
    ~ExtensionManagerWidget();

private:
    void updateView(const QModelIndex &current);
    void fetchAndInstallPlugin(const QUrl &url);

    class ExtensionManagerWidgetPrivate *d = nullptr;
};

} // ExtensionManager::Internal
