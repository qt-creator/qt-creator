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

#include "clangformatconstants.h"
#include "clangformatsettings.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"
#include "../configurationpanel.h"

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpacerItem>

namespace Beautifier::Internal {

class ClangFormatOptionsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Beautifier::Internal::ClangFormat)

public:
    explicit ClangFormatOptionsPageWidget(ClangFormatSettings *settings);

    void apply() final;

private:
    ClangFormatSettings *m_settings;
    ConfigurationPanel *m_configurations;
    QRadioButton *m_usePredefinedStyle;
    QComboBox *m_predefinedStyle;
    QComboBox *m_fallbackStyle;
    Utils::PathChooser *m_command;
    QLineEdit *m_mime;
};

ClangFormatOptionsPageWidget::ClangFormatOptionsPageWidget(ClangFormatSettings *settings)
    : m_settings(settings)
{
    auto options = new QGroupBox(tr("Options"));
    options->setEnabled(false);

    auto useCustomizedStyle = new QRadioButton(tr("Use customized style:"));
    useCustomizedStyle->setAutoExclusive(true);

    m_configurations = new ConfigurationPanel;
    m_configurations->setSettings(m_settings);
    m_configurations->setCurrentConfiguration(m_settings->customStyle());

    m_usePredefinedStyle = new QRadioButton(tr("Use predefined style:"));

    m_usePredefinedStyle->setChecked(true);
    m_usePredefinedStyle->setAutoExclusive(true);

    m_predefinedStyle = new QComboBox;
    m_predefinedStyle->addItems(m_settings->predefinedStyles());
    const int predefinedStyleIndex = m_predefinedStyle->findText(m_settings->predefinedStyle());
    if (predefinedStyleIndex != -1)
        m_predefinedStyle->setCurrentIndex(predefinedStyleIndex);

    m_fallbackStyle = new QComboBox;
    m_fallbackStyle->addItems(m_settings->fallbackStyles());
    m_fallbackStyle->setEnabled(false);
    const int fallbackStyleIndex = m_fallbackStyle->findText(m_settings->fallbackStyle());
    if (fallbackStyleIndex != -1)
        m_fallbackStyle->setCurrentIndex(fallbackStyleIndex);

    m_mime = new QLineEdit(m_settings->supportedMimeTypesAsString());

    m_command = new Utils::PathChooser;
    m_command->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_command->setCommandVersionArguments({"--version"});
    m_command->setPromptDialogTitle(
                BeautifierPlugin::msgCommandPromptDialogTitle("Clang Format"));
    m_command->setFilePath(m_settings->command());

    if (m_settings->usePredefinedStyle())
        m_usePredefinedStyle->setChecked(true);
    else
        useCustomizedStyle->setChecked(true);

    using namespace Utils::Layouting;
    const Space empty;

    Form {
        m_usePredefinedStyle, m_predefinedStyle, br,
        empty, Row { tr("Fallback style:"), m_fallbackStyle }, br,
        useCustomizedStyle, m_configurations, br,
    }.attachTo(options);

    Column {
        Group {
            Title(tr("Configuration")),
            Form {
                tr("Clang Format command:"), m_command, br,
                tr("Restrict to MIME types:"), m_mime
            }
        },
        options,
        st
    }.attachTo(this);

    connect(m_command, &Utils::PathChooser::validChanged, options, &QWidget::setEnabled);
    connect(m_predefinedStyle, &QComboBox::currentTextChanged, [this](const QString &item) {
        m_fallbackStyle->setEnabled(item == "File");
    });
    connect(m_usePredefinedStyle, &QRadioButton::toggled, [this](bool checked) {
        m_fallbackStyle->setEnabled(checked && m_predefinedStyle->currentText() == "File");
        m_predefinedStyle->setEnabled(checked);
    });
}

void ClangFormatOptionsPageWidget::apply()
{
    m_settings->setCommand(m_command->filePath().toString());
    m_settings->setSupportedMimeTypes(m_mime->text());
    m_settings->setUsePredefinedStyle(m_usePredefinedStyle->isChecked());
    m_settings->setPredefinedStyle(m_predefinedStyle->currentText());
    m_settings->setFallbackStyle(m_fallbackStyle->currentText());
    m_settings->setCustomStyle(m_configurations->currentConfiguration());
    m_settings->save();

    // update since not all MIME types are accepted (invalids or duplicates)
    m_mime->setText(m_settings->supportedMimeTypesAsString());
}

ClangFormatOptionsPage::ClangFormatOptionsPage(ClangFormatSettings *settings)
{
    setId("ClangFormat");
    setDisplayName(ClangFormatOptionsPageWidget::tr("Clang Format"));
    setCategory(Constants::OPTION_CATEGORY);
    setWidgetCreator([settings] { return new ClangFormatOptionsPageWidget(settings); });
}

} // Beautifier::Internal
