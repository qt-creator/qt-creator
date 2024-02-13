// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <coreplugin/welcomepagehelper.h>

QT_BEGIN_NAMESPACE
class QTextBrowser;
QT_END_NAMESPACE

namespace ExtensionManager::Internal {

class CollapsingWidget;
class ExtensionsBrowser;

class ExtensionManagerWidget final : public Core::ResizeSignallingWidget
{
public:
    explicit ExtensionManagerWidget();

private:
    void updateView(const QModelIndex &current, [[maybe_unused]] const QModelIndex &previous);

    ExtensionsBrowser *m_leftColumn;
    CollapsingWidget *m_secondarDescriptionWidget;
    QTextBrowser *m_primaryDescription;
    QTextBrowser *m_secondaryDescription;
};

} // ExtensionManager::Internal
