// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
        ui->mesonPathChooser->setFilePath(item->executable());
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
