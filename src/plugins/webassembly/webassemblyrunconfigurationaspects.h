// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfigurationaspects.h>

QT_FORWARD_DECLARE_CLASS(QComboBox)

namespace WebAssembly {
namespace Internal {

using WebBrowserEntry = QPair<QString, QString>; // first: id, second: display name
using WebBrowserEntries = QList<WebBrowserEntry>;

class WebBrowserSelectionAspect : public Utils::BaseAspect
{
    Q_OBJECT

public:
    WebBrowserSelectionAspect(ProjectExplorer::Target *target);

    void addToLayout(Utils::LayoutBuilder &builder) override;

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    QString currentBrowser() const;

    struct Data : BaseAspect::Data
    {
        QString currentBrowser;
    };

private:
    QComboBox *m_webBrowserComboBox = nullptr;
    QString m_currentBrowser;
    const WebBrowserEntries m_availableBrowsers;
};

} // namespace Internal
} // namespace Webassembly
