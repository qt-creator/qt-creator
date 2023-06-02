// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

QT_BEGIN_NAMESPACE
class QDesignerOptionsPageInterface;
QT_END_NAMESPACE

namespace Designer::Internal {

class SettingsPage : public Core::IOptionsPage
{
public:
    explicit SettingsPage(QDesignerOptionsPageInterface *designerPage);
};

class SettingsPageProvider : public Core::IOptionsPageProvider
{
public:
    SettingsPageProvider();

    QList<Core::IOptionsPage *> pages() const override;
    bool matches(const QRegularExpression &regex) const override;

private:
    mutable bool m_initialized = false;
    mutable QStringList m_keywords;
};

} // Designer::Internal
