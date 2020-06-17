/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "uncrustifyoptionspage.h"
#include "ui_uncrustifyoptionspage.h"

#include "uncrustifyconstants.h"
#include "uncrustifysettings.h"
#include "uncrustify.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"

namespace Beautifier {
namespace Internal {

class UncrustifyOptionsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Beautifier::Internal::ClangFormat)

public:
    explicit UncrustifyOptionsPageWidget(UncrustifySettings *settings);

    void apply() final;

private:
    Ui::UncrustifyOptionsPage ui;
    UncrustifySettings *m_settings;
};

UncrustifyOptionsPageWidget::UncrustifyOptionsPageWidget(UncrustifySettings *settings)
    : m_settings(settings)
{
    ui.setupUi(this);
    ui.useHomeFile->setText(ui.useHomeFile->text().replace(
                                 "HOME", QDir::toNativeSeparators(QDir::home().absolutePath())));
    ui.uncrusifyFilePath->setExpectedKind(Utils::PathChooser::File);
    ui.uncrusifyFilePath->setPromptDialogFilter(tr("Uncrustify file (*.cfg)"));

    ui.command->setExpectedKind(Utils::PathChooser::ExistingCommand);
    ui.command->setCommandVersionArguments({"--version"});
    ui.command->setPromptDialogTitle(BeautifierPlugin::msgCommandPromptDialogTitle(
                                          Uncrustify::tr(Constants::UNCRUSTIFY_DISPLAY_NAME)));
    connect(ui.command, &Utils::PathChooser::validChanged, ui.options, &QWidget::setEnabled);
    ui.configurations->setSettings(m_settings);

    ui.command->setFilePath(m_settings->command());
    ui.mime->setText(m_settings->supportedMimeTypesAsString());
    ui.useOtherFiles->setChecked(m_settings->useOtherFiles());
    ui.useHomeFile->setChecked(m_settings->useHomeFile());
    ui.useSpecificFile->setChecked(m_settings->useSpecificConfigFile());
    ui.uncrusifyFilePath->setFilePath(m_settings->specificConfigFile());
    ui.useCustomStyle->setChecked(m_settings->useCustomStyle());
    ui.configurations->setCurrentConfiguration(m_settings->customStyle());
    ui.formatEntireFileFallback->setChecked(m_settings->formatEntireFileFallback());
}

void UncrustifyOptionsPageWidget::apply()
{
    m_settings->setCommand(ui.command->filePath().toString());
    m_settings->setSupportedMimeTypes(ui.mime->text());
    m_settings->setUseOtherFiles(ui.useOtherFiles->isChecked());
    m_settings->setUseHomeFile(ui.useHomeFile->isChecked());
    m_settings->setUseSpecificConfigFile(ui.useSpecificFile->isChecked());
    m_settings->setSpecificConfigFile(ui.uncrusifyFilePath->filePath());
    m_settings->setUseCustomStyle(ui.useCustomStyle->isChecked());
    m_settings->setCustomStyle(ui.configurations->currentConfiguration());
    m_settings->setFormatEntireFileFallback(ui.formatEntireFileFallback->isChecked());
    m_settings->save();

    // update since not all MIME types are accepted (invalids or duplicates)
    ui.mime->setText(m_settings->supportedMimeTypesAsString());
}

UncrustifyOptionsPage::UncrustifyOptionsPage(UncrustifySettings *settings)
{
    setId("Uncrustify");
    setDisplayName(UncrustifyOptionsPageWidget::tr("Uncrustify"));
    setCategory(Constants::OPTION_CATEGORY);
    setWidgetCreator([settings] { return new UncrustifyOptionsPageWidget(settings); });
}

} // namespace Internal
} // namespace Beautifier
