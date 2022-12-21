// Copyright (C) 2016 Lorenz Haas
// Copyright (C) 2022 Xavier BESSON
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakeformatteroptionspage.h"
#include "cmakeformattersettings.h"

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>

namespace CMakeProjectManager::Internal {

class CMakeFormatterOptionsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(CMakeFormat::Internal::GeneralOptionsPageWidget)

public:
    explicit CMakeFormatterOptionsPageWidget();

private:
    void apply() final;

    Utils::PathChooser *m_command;
    QCheckBox *m_autoFormat;
    QLineEdit *m_autoFormatMime;
    QCheckBox *m_autoFormatOnlyCurrentProject;
};

CMakeFormatterOptionsPageWidget::CMakeFormatterOptionsPageWidget()
{
    resize(817, 631);

    auto settings = CMakeFormatterSettings::instance();

    m_autoFormat = new QCheckBox(tr("Enable auto format on file save"));
    m_autoFormat->setChecked(settings->autoFormatOnSave());

    auto mimeLabel = new QLabel(tr("Restrict to MIME types:"));
    mimeLabel->setEnabled(false);

    m_autoFormatMime = new QLineEdit(settings->autoFormatMimeAsString());
    m_autoFormatMime->setEnabled(m_autoFormat->isChecked());

    m_autoFormatOnlyCurrentProject =
        new QCheckBox(tr("Restrict to files contained in the current project"));
    m_autoFormatOnlyCurrentProject->setEnabled(m_autoFormat->isChecked());
    m_autoFormatOnlyCurrentProject->setChecked(settings->autoFormatOnlyCurrentProject());

    m_command = new Utils::PathChooser;
    m_command->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_command->setCommandVersionArguments({"--version"});
    m_command->setPromptDialogTitle(tr("%1 Command").arg(Tr::Tr::tr("Formatter")));
    m_command->setFilePath(settings->command());

    using namespace Utils::Layouting;

    Column {
        Group {
            title(tr("Automatic Formatting on File Save")),
            Form {
                tr("CMakeFormat command:"), m_command, br,
                Span(2, m_autoFormat), br,
                mimeLabel, m_autoFormatMime, br,
                Span(2, m_autoFormatOnlyCurrentProject)
            }
        },
        st
    }.attachTo(this);

    connect(m_autoFormat, &QCheckBox::toggled, m_autoFormatMime, &QLineEdit::setEnabled);
    connect(m_autoFormat, &QCheckBox::toggled, mimeLabel, &QLabel::setEnabled);
    connect(m_autoFormat, &QCheckBox::toggled, m_autoFormatOnlyCurrentProject, &QCheckBox::setEnabled);
}

void CMakeFormatterOptionsPageWidget::apply()
{
    auto settings = CMakeFormatterSettings::instance();
    settings->setCommand(m_command->filePath().toString());
    settings->setAutoFormatOnSave(m_autoFormat->isChecked());
    settings->setAutoFormatMime(m_autoFormatMime->text());
    settings->setAutoFormatOnlyCurrentProject(m_autoFormatOnlyCurrentProject->isChecked());
    settings->save();
}

CMakeFormatterOptionsPage::CMakeFormatterOptionsPage()
{
    setId(Constants::Settings::FORMATTER_ID);
    setDisplayName(Tr::Tr::tr("Formatter"));
    setDisplayCategory("CMake");
    setCategory(Constants::Settings::CATEGORY);
    setWidgetCreator([] { return new CMakeFormatterOptionsPageWidget; });
}

} // CMakeProjectManager::Internal
