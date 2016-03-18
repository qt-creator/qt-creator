/****************************************************************************
**
** Copyright (C) 2016 Dmitry Savchenko
** Copyright (C) 2016 Vasiliy Sorokin
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

#include "settings.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

namespace Todo {
namespace Internal {

class OptionsDialog;

class OptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    OptionsPage(const Settings &settings, QObject *parent = 0);

    void setSettings(const Settings &settings);

    QWidget *widget();
    void apply();
    void finish();

signals:
    void settingsChanged(const Settings &settings);

private:
    QPointer<OptionsDialog> m_widget;
    Settings m_settings;
};

} // namespace Internal
} // namespace Todo
