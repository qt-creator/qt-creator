// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uncrustifyoptionspage.h"

#include "uncrustifyconstants.h"
#include "uncrustifysettings.h"
#include "uncrustify.h"

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

class UncrustifyOptionsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Beautifier::Internal::ClangFormat)

public:
    explicit UncrustifyOptionsPageWidget(UncrustifySettings *settings);

    void apply() final;

private:
    UncrustifySettings *m_settings;

    Utils::PathChooser *m_command;
    QLineEdit *m_mime;
    QCheckBox *m_useOtherFiles;
    QCheckBox *m_useSpecificFile;
    Utils::PathChooser *m_uncrusifyFilePath;
    QCheckBox *m_useHomeFile;
    QCheckBox *m_useCustomStyle;
    ConfigurationPanel *m_configurations;
    QCheckBox *m_formatEntireFileFallback;
};

UncrustifyOptionsPageWidget::UncrustifyOptionsPageWidget(UncrustifySettings *settings)
    : m_settings(settings)
{
    resize(817, 631);

    m_command = new Utils::PathChooser;

    m_mime = new QLineEdit(m_settings->supportedMimeTypesAsString());

    m_useOtherFiles = new QCheckBox(tr("Use file uncrustify.cfg defined in project files"));
    m_useOtherFiles->setChecked(m_settings->useOtherFiles());

    m_useSpecificFile = new QCheckBox(tr("Use file specific uncrustify.cfg"));
    m_useSpecificFile->setChecked(m_settings->useSpecificConfigFile());

    m_uncrusifyFilePath = new Utils::PathChooser;
    m_uncrusifyFilePath->setExpectedKind(Utils::PathChooser::File);
    m_uncrusifyFilePath->setPromptDialogFilter(tr("Uncrustify file (*.cfg)"));
    m_uncrusifyFilePath->setFilePath(m_settings->specificConfigFile());

    m_useHomeFile = new QCheckBox(tr("Use file uncrustify.cfg in HOME")
        .replace( "HOME", QDir::toNativeSeparators(QDir::home().absolutePath())));
    m_useHomeFile->setChecked(m_settings->useHomeFile());

    m_useCustomStyle = new QCheckBox(tr("Use customized style:"));
    m_useCustomStyle->setChecked(m_settings->useCustomStyle());

    m_configurations = new ConfigurationPanel;
    m_configurations->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_configurations->setSettings(m_settings);
    m_configurations->setCurrentConfiguration(m_settings->customStyle());

    m_formatEntireFileFallback = new QCheckBox(tr("Format entire file if no text was selected"));
    m_formatEntireFileFallback->setToolTip(tr("For action Format Selected Text"));
    m_formatEntireFileFallback->setChecked(m_settings->formatEntireFileFallback());

    m_command->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_command->setCommandVersionArguments({"--version"});
    m_command->setPromptDialogTitle(BeautifierPlugin::msgCommandPromptDialogTitle(
                                          Uncrustify::tr(Constants::UNCRUSTIFY_DISPLAY_NAME)));
    m_command->setFilePath(m_settings->command());

    auto options = new QGroupBox(tr("Options"));

    using namespace Utils::Layouting;

    Column {
        m_useOtherFiles,
        Row { m_useSpecificFile, m_uncrusifyFilePath },
        m_useHomeFile,
        Row { m_useCustomStyle, m_configurations },
        m_formatEntireFileFallback
    }.attachTo(options);

    Column {
        Group {
            title(tr("Configuration")),
            Form {
                tr("Uncrustify command:"), m_command, br,
                tr("Restrict to MIME types:"), m_mime
            }
        },
        options,
        st
    }.attachTo(this);

    connect(m_command, &Utils::PathChooser::validChanged, options, &QWidget::setEnabled);
}

void UncrustifyOptionsPageWidget::apply()
{
    m_settings->setCommand(m_command->filePath().toString());
    m_settings->setSupportedMimeTypes(m_mime->text());
    m_settings->setUseOtherFiles(m_useOtherFiles->isChecked());
    m_settings->setUseHomeFile(m_useHomeFile->isChecked());
    m_settings->setUseSpecificConfigFile(m_useSpecificFile->isChecked());
    m_settings->setSpecificConfigFile(m_uncrusifyFilePath->filePath());
    m_settings->setUseCustomStyle(m_useCustomStyle->isChecked());
    m_settings->setCustomStyle(m_configurations->currentConfiguration());
    m_settings->setFormatEntireFileFallback(m_formatEntireFileFallback->isChecked());
    m_settings->save();

    // update since not all MIME types are accepted (invalids or duplicates)
    m_mime->setText(m_settings->supportedMimeTypesAsString());
}

UncrustifyOptionsPage::UncrustifyOptionsPage(UncrustifySettings *settings)
{
    setId("Uncrustify");
    setDisplayName(UncrustifyOptionsPageWidget::tr("Uncrustify"));
    setCategory(Constants::OPTION_CATEGORY);
    setWidgetCreator([settings] { return new UncrustifyOptionsPageWidget(settings); });
}

} // Beautifier::Internal
