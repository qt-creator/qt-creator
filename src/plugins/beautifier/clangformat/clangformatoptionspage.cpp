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

#include "clangformatoptionspage.h"
#include "ui_clangformatoptionspage.h"

#include "clangformatconstants.h"
#include "clangformatsettings.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"

namespace Beautifier {
namespace Internal {

class ClangFormatOptionsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Beautifier::Internal::ClangFormat)

public:
    explicit ClangFormatOptionsPageWidget(ClangFormatSettings *settings);

    void apply() final;

private:
    Ui::ClangFormatOptionsPage ui;
    ClangFormatSettings *m_settings;
};

ClangFormatOptionsPageWidget::ClangFormatOptionsPageWidget(ClangFormatSettings *settings)
    : m_settings(settings)
{
    ui.setupUi(this);
    ui.options->setEnabled(false);
    ui.predefinedStyle->addItems(m_settings->predefinedStyles());
    ui.fallbackStyle->addItems(m_settings->fallbackStyles());
    ui.command->setExpectedKind(Utils::PathChooser::ExistingCommand);
    ui.command->setCommandVersionArguments({"--version"});
    ui.command->setPromptDialogTitle(
                BeautifierPlugin::msgCommandPromptDialogTitle("Clang Format"));
    connect(ui.command, &Utils::PathChooser::validChanged, ui.options, &QWidget::setEnabled);
    connect(ui.predefinedStyle, &QComboBox::currentTextChanged, [this](const QString &item) {
        ui.fallbackStyle->setEnabled(item == "File");
    });
    connect(ui.usePredefinedStyle, &QRadioButton::toggled, [this](bool checked) {
        ui.fallbackStyle->setEnabled(checked && ui.predefinedStyle->currentText() == "File");
        ui.predefinedStyle->setEnabled(checked);
    });
    ui.configurations->setSettings(m_settings);

    ui.command->setFilePath(m_settings->command());
    ui.mime->setText(m_settings->supportedMimeTypesAsString());
    const int predefinedStyleIndex = ui.predefinedStyle->findText(m_settings->predefinedStyle());
    if (predefinedStyleIndex != -1)
        ui.predefinedStyle->setCurrentIndex(predefinedStyleIndex);
    const int fallbackStyleIndex = ui.fallbackStyle->findText(m_settings->fallbackStyle());
    if (fallbackStyleIndex != -1)
        ui.fallbackStyle->setCurrentIndex(fallbackStyleIndex);
    ui.configurations->setSettings(m_settings);
    ui.configurations->setCurrentConfiguration(m_settings->customStyle());

    if (m_settings->usePredefinedStyle())
        ui.usePredefinedStyle->setChecked(true);
    else
        ui.useCustomizedStyle->setChecked(true);
}

void ClangFormatOptionsPageWidget::apply()
{
    m_settings->setCommand(ui.command->filePath().toString());
    m_settings->setSupportedMimeTypes(ui.mime->text());
    m_settings->setUsePredefinedStyle(ui.usePredefinedStyle->isChecked());
    m_settings->setPredefinedStyle(ui.predefinedStyle->currentText());
    m_settings->setFallbackStyle(ui.fallbackStyle->currentText());
    m_settings->setCustomStyle(ui.configurations->currentConfiguration());
    m_settings->save();

    // update since not all MIME types are accepted (invalids or duplicates)
    ui.mime->setText(m_settings->supportedMimeTypesAsString());
}

ClangFormatOptionsPage::ClangFormatOptionsPage(ClangFormatSettings *settings)
{
    setId("ClangFormat");
    setDisplayName(ClangFormatOptionsPageWidget::tr("Clang Format"));
    setCategory(Constants::OPTION_CATEGORY);
    setWidgetCreator([settings] { return new ClangFormatOptionsPageWidget(settings); });
}

} // namespace Internal
} // namespace Beautifier
