/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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

#include "toolitemsettings.h"
#include "tooltreeitem.h"
#include "ui_toolitemsettings.h"

namespace MesonProjectManager {
namespace Internal {
ToolItemSettings::ToolItemSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ToolItemSettings)
{
    ui->setupUi(this);
    ui->mesonPathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    ui->mesonPathChooser->setHistoryCompleter(QLatin1String("Meson.Command.History"));
    connect(ui->mesonPathChooser,
            &Utils::PathChooser::rawPathChanged,
            this,
            &ToolItemSettings::store);
    connect(ui->mesonNameLineEdit, &QLineEdit::textChanged, this, &ToolItemSettings::store);
}

ToolItemSettings::~ToolItemSettings()
{
    delete ui;
}

void ToolItemSettings::load(ToolTreeItem *item)
{
    if (item) {
        m_currentId = Utils::nullopt;
        ui->mesonNameLineEdit->setDisabled(item->isAutoDetected());
        ui->mesonNameLineEdit->setText(item->name());
        ui->mesonPathChooser->setDisabled(item->isAutoDetected());
        ui->mesonPathChooser->setFileName(item->executable());
        m_currentId = item->id();
    } else {
        m_currentId = Utils::nullopt;
    }
}

void ToolItemSettings::store()
{
    if (m_currentId)
        emit applyChanges(*m_currentId,
                          ui->mesonNameLineEdit->text(),
                          ui->mesonPathChooser->filePath());
}

} // namespace Internal
} // namespace MesonProjectManager
