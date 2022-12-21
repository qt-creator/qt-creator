// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

namespace Autotest {
namespace Internal {

struct TestSettings;
class TestSettingsWidget;

class TestSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    explicit TestSettingsPage(TestSettings *settings);

    QWidget *widget() override;
    void apply() override;
    void finish() override { }

private:
    TestSettings *m_settings;
    QPointer<TestSettingsWidget> m_widget;
};

} // namespace Internal
} // namespace Autotest
