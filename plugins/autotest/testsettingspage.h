/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#ifndef TESTSETTINGSPAGE_H
#define TESTSETTINGSPAGE_H

#include "ui_testsettingspage.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

namespace Autotest {
namespace Internal {

struct TestSettings;

class TestSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TestSettingsWidget(QWidget *parent = 0);

    void setSettings(const TestSettings &settings);
    TestSettings settings() const;

private:
    Ui::TestSettingsPage m_ui;

};

class TestSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    explicit TestSettingsPage(const QSharedPointer<TestSettings> &settings);
    ~TestSettingsPage();

    QWidget *widget();
    void apply();
    void finish() { }

private:
    QSharedPointer<TestSettings> m_settings;
    QPointer<TestSettingsWidget> m_widget;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTSETTINGSPAGE_H
