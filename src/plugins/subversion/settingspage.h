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

#include "ui_settingspage.h"

#include <vcsbase/vcsbaseoptionspage.h>

#include <QWidget>
#include <QPointer>
#include <QString>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Subversion {
namespace Internal {

class SettingsPageWidget : public VcsBase::VcsClientOptionsPageWidget
{
    Q_OBJECT

public:
    explicit SettingsPageWidget(QWidget *parent = 0);

    VcsBase::VcsBaseClientSettings settings() const;
    void setSettings(const VcsBase::VcsBaseClientSettings &s);

private:
    Ui::SettingsPage m_ui;
};


class SettingsPage : public VcsBase::VcsClientOptionsPage
{
    Q_OBJECT

public:
    SettingsPage(Core::IVersionControl *control);
};

} // namespace Subversion
} // namespace Internal
