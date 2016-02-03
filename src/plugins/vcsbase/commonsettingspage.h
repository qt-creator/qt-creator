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

#include "commonvcssettings.h"

#include "vcsbaseoptionspage.h"

#include <QPointer>
#include <QWidget>

namespace VcsBase {
namespace Internal {

namespace Ui { class CommonSettingsPage; }

class CommonSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommonSettingsWidget(QWidget *parent = 0);
    ~CommonSettingsWidget() override;

    CommonVcsSettings settings() const;
    void setSettings(const CommonVcsSettings &s);

private:
    void updatePath();

    Ui::CommonSettingsPage *m_ui;
};

class CommonOptionsPage : public VcsBaseOptionsPage
{
    Q_OBJECT

public:
    explicit CommonOptionsPage(QObject *parent = 0);

    QWidget *widget() override;
    void apply() override;
    void finish() override;

    CommonVcsSettings settings() const { return m_settings; }

signals:
    void settingsChanged(const VcsBase::Internal::CommonVcsSettings &s);

private:
    QPointer<CommonSettingsWidget> m_widget;
    CommonVcsSettings m_settings;
};

} // namespace Internal
} // namespace VcsBase
