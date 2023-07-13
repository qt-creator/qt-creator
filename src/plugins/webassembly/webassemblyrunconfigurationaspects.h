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
    WebBrowserSelectionAspect(Utils::AspectContainer *container);

    void setTarget(ProjectExplorer::Target *target);

    void addToLayout(Layouting::LayoutItem &parent) override;

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    QString currentBrowser() const;

    static WebBrowserEntries parseEmrunOutput(const QByteArray &output);

    struct Data : BaseAspect::Data
    {
        QString currentBrowser;
    };

private:
    QComboBox *m_webBrowserComboBox = nullptr;
    QString m_currentBrowser;
    WebBrowserEntries m_availableBrowsers;
};

} // namespace Internal
} // namespace Webassembly
