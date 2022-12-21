// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QDesignerOptionsPageInterface;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

class SettingsPageWidget;

class SettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit SettingsPage(QDesignerOptionsPageInterface *designerPage);

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    QDesignerOptionsPageInterface *m_designerPage;
    bool m_initialized = false;
    QPointer<QWidget> m_widget;
};

class SettingsPageProvider : public Core::IOptionsPageProvider
{
    Q_OBJECT

public:
    SettingsPageProvider();

    QList<Core::IOptionsPage *> pages() const override;
    bool matches(const QRegularExpression &regex) const override;

private:
    mutable bool m_initialized = false;
    mutable QStringList m_keywords;
};

} // namespace Internal
} // namespace Designer
