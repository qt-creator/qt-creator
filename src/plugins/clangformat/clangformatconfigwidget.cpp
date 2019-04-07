/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangformatconfigwidget.h"

#include "clangformatconstants.h"
#include "clangformatindenter.h"
#include "clangformatsettings.h"
#include "clangformatutils.h"
#include "ui_clangformatconfigwidget.h"

#include <clang/Format/Format.h>

#include <coreplugin/icore.h>
#include <cppeditor/cpphighlighter.h>
#include <cpptools/cppcodestylesnippets.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <texteditor/displaysettings.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>

#include <QFile>
#include <QMessageBox>

#include <sstream>

using namespace ProjectExplorer;

namespace ClangFormat {

ClangFormatConfigWidget::ClangFormatConfigWidget(ProjectExplorer::Project *project, QWidget *parent)
    : CodeStyleEditorWidget(parent)
    , m_project(project)
    , m_ui(std::make_unique<Ui::ClangFormatConfigWidget>())
{
    m_ui->setupUi(this);

    m_preview = new TextEditor::SnippetEditorWidget(this);
    m_ui->horizontalLayout_2->addWidget(m_preview);
    if (m_project) {
        m_ui->applyButton->show();
        hideGlobalCheckboxes();
        m_ui->overrideDefault->setChecked(
            m_project->namedSettings(Constants::OVERRIDE_FILE_ID).toBool());
    } else {
        m_ui->applyButton->hide();
        showGlobalCheckboxes();
        m_ui->overrideDefault->setChecked(ClangFormatSettings::instance().overrideDefaultFile());
    }

    connect(m_ui->overrideDefault, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked)
            createStyleFileIfNeeded(!m_project);
        initialize();
    });
    initialize();
}

void ClangFormatConfigWidget::hideGlobalCheckboxes()
{
    m_ui->formatAlways->hide();
    m_ui->formatWhileTyping->hide();
    m_ui->formatOnSave->hide();
}

void ClangFormatConfigWidget::showGlobalCheckboxes()
{
    m_ui->formatAlways->setChecked(ClangFormatSettings::instance().formatCodeInsteadOfIndent());
    m_ui->formatAlways->show();

    m_ui->formatWhileTyping->setChecked(ClangFormatSettings::instance().formatWhileTyping());
    m_ui->formatWhileTyping->show();

    m_ui->formatOnSave->setChecked(ClangFormatSettings::instance().formatOnSave());
    m_ui->formatOnSave->show();
}

static bool projectConfigExists()
{
    return Utils::FileName::fromString(Core::ICore::userResourcePath())
        .appendPath("clang-format")
        .appendPath(currentProjectUniqueId())
        .appendPath((Constants::SETTINGS_FILE_NAME))
        .exists();
}

void ClangFormatConfigWidget::initialize()
{
    m_ui->projectHasClangFormat->hide();

    m_preview->setPlainText(QLatin1String(CppTools::Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]));
    m_preview->textDocument()->setIndenter(new ClangFormatIndenter(m_preview->document()));
    m_preview->textDocument()->setFontSettings(TextEditor::TextEditorSettings::fontSettings());
    m_preview->textDocument()->setSyntaxHighlighter(new CppEditor::CppHighlighter);

    TextEditor::DisplaySettings displaySettings = m_preview->displaySettings();
    displaySettings.m_visualizeWhitespace = true;
    m_preview->setDisplaySettings(displaySettings);

    QLayoutItem *lastItem = m_ui->verticalLayout->itemAt(m_ui->verticalLayout->count() - 1);
    if (lastItem->spacerItem())
        m_ui->verticalLayout->removeItem(lastItem);

    m_ui->clangFormatOptionsTable->setEnabled(true);
    if (!m_ui->overrideDefault->isChecked()) {
        if (m_project) {
            m_ui->clangFormatOptionsTable->hide();
            m_preview->hide();
            m_ui->verticalLayout->addStretch(1);
            return;
        } else {
            // Show the fallback configuration only globally.
            m_ui->clangFormatOptionsTable->setEnabled(false);
        }
    }

    m_ui->clangFormatOptionsTable->show();
    m_preview->show();

    Utils::FileName fileName;
    if (m_project) {
        m_ui->projectHasClangFormat->hide();
        connect(m_ui->applyButton, &QPushButton::clicked, this, &ClangFormatConfigWidget::apply);
        fileName = m_project->projectFilePath().appendPath("snippet.cpp");
    } else {
        const Project *currentProject = SessionManager::startupProject();
        if (!currentProject || !projectConfigExists()) {
            m_ui->projectHasClangFormat->hide();
        } else {
            m_ui->projectHasClangFormat->show();
            m_ui->projectHasClangFormat->setText(
                tr("Current project has its own overridden .clang-format file "
                   "and can be configured in Projects > Code Style > C++."));
        }
        fileName = Utils::FileName::fromString(Core::ICore::userResourcePath())
                       .appendPath("snippet.cpp");
    }

    m_preview->textDocument()->indenter()->setFileName(fileName);
    fillTable();
    updatePreview();
}

void ClangFormatConfigWidget::updatePreview()
{
    QTextCursor cursor(m_preview->document());
    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    m_preview->textDocument()->autoFormatOrIndent(cursor);
}

void ClangFormatConfigWidget::fillTable()
{
    clang::format::FormatStyle style = m_project ? currentProjectStyle() : currentGlobalStyle();

    const std::string configText = clang::format::configurationAsText(style);
    m_ui->clangFormatOptionsTable->setPlainText(QString::fromStdString(configText));
}

ClangFormatConfigWidget::~ClangFormatConfigWidget() = default;

void ClangFormatConfigWidget::apply()
{
    ClangFormatSettings &settings = ClangFormatSettings::instance();
    if (!m_project) {
        settings.setFormatCodeInsteadOfIndent(m_ui->formatAlways->isChecked());
        settings.setFormatWhileTyping(m_ui->formatWhileTyping->isChecked());
        settings.setFormatOnSave(m_ui->formatOnSave->isChecked());
        settings.setOverrideDefaultFile(m_ui->overrideDefault->isChecked());
    } else {
        m_project->setNamedSettings(Constants::OVERRIDE_FILE_ID, m_ui->overrideDefault->isChecked());
    }
    settings.write();

    if (!m_ui->clangFormatOptionsTable->isVisible())
        return;

    const QString text = m_ui->clangFormatOptionsTable->toPlainText();
    clang::format::FormatStyle style;
    style.Language = clang::format::FormatStyle::LK_Cpp;
    const std::error_code error = clang::format::parseConfiguration(text.toStdString(), &style);
    if (error.value() != static_cast<int>(clang::format::ParseError::Success)) {
        QMessageBox::warning(this,
                             tr("Error in ClangFormat configuration"),
                             QString::fromStdString(error.message()));
        if (m_ui->overrideDefault->isChecked()) {
            fillTable();
            updatePreview();
        }
        return;
    }

    QString filePath = Core::ICore::userResourcePath();
    if (m_project)
        filePath += "/clang-format/" + currentProjectUniqueId();
    filePath += "/" + QLatin1String(Constants::SETTINGS_FILE_NAME);

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly))
        return;

    file.write(text.toUtf8());
    file.close();

    if (m_ui->overrideDefault->isChecked())
        updatePreview();
}

} // namespace ClangFormat
