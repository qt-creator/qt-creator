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

#include "ui_gtestsettingspage.h"

#include "../itestsettingspage.h"

#include <QPointer>

namespace Autotest {
namespace Internal {

class IFrameworkSettings;
class GTestSettings;

class GTestSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GTestSettingsWidget(QWidget *parent = 0);

    void setSettings(const GTestSettings &settings);
    GTestSettings settings() const;

private:
    Ui::GTestSettingsPage m_ui;
};

class GTestSettingsPage : public ITestSettingsPage
{
    Q_OBJECT
public:
    GTestSettingsPage(QSharedPointer<IFrameworkSettings> settings, const ITestFramework *framework);

    QWidget *widget() override;
    void apply() override;
    void finish() override { }

private:
    QSharedPointer<GTestSettings> m_settings;
    QPointer<GTestSettingsWidget> m_widget;
};

} // namespace Internal
} // namespace Autotest
