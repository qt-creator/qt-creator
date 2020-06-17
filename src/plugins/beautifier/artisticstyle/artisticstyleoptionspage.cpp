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

#include "artisticstyleoptionspage.h"
#include "ui_artisticstyleoptionspage.h"

#include "artisticstyleconstants.h"
#include "artisticstylesettings.h"
#include "artisticstyle.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"

namespace Beautifier {
namespace Internal {

class ArtisticStyleOptionsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Beautifier::Internal::ArtisticStyle)

public:
    explicit ArtisticStyleOptionsPageWidget(ArtisticStyleSettings *settings);

    void apply() final;

private:
    Ui::ArtisticStyleOptionsPage ui;
    ArtisticStyleSettings *m_settings;
};

ArtisticStyleOptionsPageWidget::ArtisticStyleOptionsPageWidget(ArtisticStyleSettings *settings)
    : m_settings(settings)
{
    ui.setupUi(this);
    ui.useHomeFile->setText(ui.useHomeFile->text().replace(
                                 "HOME", QDir::toNativeSeparators(QDir::home().absolutePath())));
    ui.specificConfigFile->setExpectedKind(Utils::PathChooser::File);
    ui.specificConfigFile->setPromptDialogFilter(tr("AStyle (*.astylerc)"));
    ui.command->setExpectedKind(Utils::PathChooser::ExistingCommand);
    ui.command->setCommandVersionArguments({"--version"});
    ui.command->setPromptDialogTitle(BeautifierPlugin::msgCommandPromptDialogTitle(
                                          ArtisticStyle::tr(Constants::ARTISTICSTYLE_DISPLAY_NAME)));
    connect(ui.command, &Utils::PathChooser::validChanged, ui.options, &QWidget::setEnabled);
    ui.configurations->setSettings(m_settings);

    ui.command->setFilePath(m_settings->command());
    ui.mime->setText(m_settings->supportedMimeTypesAsString());
    ui.useOtherFiles->setChecked(m_settings->useOtherFiles());
    ui.useSpecificConfigFile->setChecked(m_settings->useSpecificConfigFile());
    ui.specificConfigFile->setFilePath(m_settings->specificConfigFile());
    ui.useHomeFile->setChecked(m_settings->useHomeFile());
    ui.useCustomStyle->setChecked(m_settings->useCustomStyle());
    ui.configurations->setCurrentConfiguration(m_settings->customStyle());
}

void ArtisticStyleOptionsPageWidget::apply()
{
    m_settings->setCommand(ui.command->filePath().toString());
    m_settings->setSupportedMimeTypes(ui.mime->text());
    m_settings->setUseOtherFiles(ui.useOtherFiles->isChecked());
    m_settings->setUseSpecificConfigFile(ui.useSpecificConfigFile->isChecked());
    m_settings->setSpecificConfigFile(ui.specificConfigFile->filePath());
    m_settings->setUseHomeFile(ui.useHomeFile->isChecked());
    m_settings->setUseCustomStyle(ui.useCustomStyle->isChecked());
    m_settings->setCustomStyle(ui.configurations->currentConfiguration());
    m_settings->save();

    // update since not all MIME types are accepted (invalids or duplicates)
    ui.mime->setText(m_settings->supportedMimeTypesAsString());
}

ArtisticStyleOptionsPage::ArtisticStyleOptionsPage(ArtisticStyleSettings *settings)
{
    setId("ArtisticStyle");
    setDisplayName(ArtisticStyleOptionsPageWidget::tr("Artistic Style"));
    setCategory(Constants::OPTION_CATEGORY);
    setWidgetCreator([settings] { return new ArtisticStyleOptionsPageWidget(settings); });
}

} // namespace Internal
} // namespace Beautifier
