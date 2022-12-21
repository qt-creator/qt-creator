// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QHelpFilterSettingsWidget;
QT_END_NAMESPACE

namespace Help {
namespace Internal {

class FilterSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    FilterSettingsPage();

    QWidget *widget() override;
    void apply() override;
    void finish() override;

signals:
    void filtersChanged();

private:

    void updateFilterPage();
    QPointer<QHelpFilterSettingsWidget> m_widget;

};

} // namespace Help
} // namespace Internal
