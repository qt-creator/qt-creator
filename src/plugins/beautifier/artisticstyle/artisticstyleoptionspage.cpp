// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "artisticstyleoptionspage.h"

#include "artisticstyleconstants.h"
#include "artisticstylesettings.h"
#include "artisticstyle.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"
#include "../configurationpanel.h"

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>

namespace Beautifier::Internal {

class ArtisticStyleOptionsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Beautifier::Internal::ArtisticStyle)

public:
    explicit ArtisticStyleOptionsPageWidget(ArtisticStyleSettings *settings);

    void apply() final;

private:
    ArtisticStyleSettings *m_settings;

    Utils::PathChooser *m_command;
    QLineEdit *m_mime;
    QCheckBox *m_useOtherFiles;
    QCheckBox *m_useSpecificConfigFile;
    Utils::PathChooser *m_specificConfigFile;
    QCheckBox *m_useHomeFile;
    QCheckBox *m_useCustomStyle;
    Beautifier::Internal::ConfigurationPanel *m_configurations;
};

ArtisticStyleOptionsPageWidget::ArtisticStyleOptionsPageWidget(ArtisticStyleSettings *settings)
    : m_settings(settings)
{
    resize(817, 631);

    m_command = new Utils::PathChooser;

    m_mime = new QLineEdit(m_settings->supportedMimeTypesAsString());

    auto options = new QGroupBox(tr("Options"));

    m_useOtherFiles = new QCheckBox(tr("Use file *.astylerc defined in project files"));
    m_useOtherFiles->setChecked(m_settings->useOtherFiles());

    m_useSpecificConfigFile = new QCheckBox(tr("Use specific config file:"));
    m_useSpecificConfigFile->setChecked(m_settings->useSpecificConfigFile());

    m_specificConfigFile = new Utils::PathChooser;
    m_specificConfigFile->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_specificConfigFile->setExpectedKind(Utils::PathChooser::File);
    m_specificConfigFile->setPromptDialogFilter(tr("AStyle (*.astylerc)"));
    m_specificConfigFile->setFilePath(m_settings->specificConfigFile());

    m_useHomeFile = new QCheckBox(
        tr("Use file .astylerc or astylerc in HOME").
            replace("HOME", QDir::toNativeSeparators(QDir::home().absolutePath())));
    m_useHomeFile->setChecked(m_settings->useHomeFile());

    m_useCustomStyle = new QCheckBox(tr("Use customized style:"));
    m_useCustomStyle->setChecked(m_settings->useCustomStyle());

    m_configurations = new Beautifier::Internal::ConfigurationPanel(options);
    m_configurations->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_configurations->setSettings(m_settings);
    m_configurations->setCurrentConfiguration(m_settings->customStyle());

    m_command->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_command->setCommandVersionArguments({"--version"});
    m_command->setPromptDialogTitle(BeautifierPlugin::msgCommandPromptDialogTitle(
                                          ArtisticStyle::tr(Constants::ARTISTICSTYLE_DISPLAY_NAME)));
    m_command->setFilePath(m_settings->command());

    using namespace Utils::Layouting;

    Column {
        m_useOtherFiles,
        Row { m_useSpecificConfigFile, m_specificConfigFile },
        m_useHomeFile,
        Row { m_useCustomStyle, m_configurations }
    }.attachTo(options);

    Column {
        Group {
            title(tr("Configuration")),
            Form {
                tr("Artistic Style command:"), m_command, br,
                tr("Restrict to MIME types:"), m_mime
            }
        },
        options,
        st
    }.attachTo(this);

    connect(m_command, &Utils::PathChooser::validChanged, options, &QWidget::setEnabled);
}

void ArtisticStyleOptionsPageWidget::apply()
{
    m_settings->setCommand(m_command->filePath().toString());
    m_settings->setSupportedMimeTypes(m_mime->text());
    m_settings->setUseOtherFiles(m_useOtherFiles->isChecked());
    m_settings->setUseSpecificConfigFile(m_useSpecificConfigFile->isChecked());
    m_settings->setSpecificConfigFile(m_specificConfigFile->filePath());
    m_settings->setUseHomeFile(m_useHomeFile->isChecked());
    m_settings->setUseCustomStyle(m_useCustomStyle->isChecked());
    m_settings->setCustomStyle(m_configurations->currentConfiguration());
    m_settings->save();

    // update since not all MIME types are accepted (invalids or duplicates)
    m_mime->setText(m_settings->supportedMimeTypesAsString());
}

ArtisticStyleOptionsPage::ArtisticStyleOptionsPage(ArtisticStyleSettings *settings)
{
    setId("ArtisticStyle");
    setDisplayName(ArtisticStyleOptionsPageWidget::tr("Artistic Style"));
    setCategory(Constants::OPTION_CATEGORY);
    setWidgetCreator([settings] { return new ArtisticStyleOptionsPageWidget(settings); });
}

} // Beautifier::Internal
