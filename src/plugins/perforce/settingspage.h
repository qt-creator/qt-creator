/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QWidget>

#include <vcsbase/vcsbaseoptionspage.h>

#include "ui_settingspage.h"

namespace Perforce {
namespace Internal {

class PerforceSettings;
class PerforceChecker;
struct Settings;

class SettingsPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPageWidget(QWidget *parent = 0);
    ~SettingsPageWidget() override;

    void setSettings(const PerforceSettings &);
    Settings settings() const;

private:
    void slotTest();
    void setStatusText(const QString &);
    void setStatusError(const QString &);
    void testSucceeded(const QString &repo);

    Ui::SettingsPage m_ui;
    PerforceChecker *m_checker = nullptr;
};

class SettingsPage : public VcsBase::VcsBaseOptionsPage
{
    Q_OBJECT

public:
    SettingsPage();
    ~SettingsPage() override;

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    SettingsPageWidget *m_widget = nullptr;
};

} // namespace Internal
} // namespace Perforce
