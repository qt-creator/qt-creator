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
#include "ui_clangformatchecks.h"
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
#include <utils/qtcassert.h>

#include <QFile>
#include <QMessageBox>
#include <QScrollArea>

#include <sstream>

using namespace ProjectExplorer;

namespace ClangFormat {

bool ClangFormatConfigWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Wheel && qobject_cast<QComboBox *>(object)) {
        event->ignore();
        return true;
    }
    return QWidget::eventFilter(object, event);
}

ClangFormatConfigWidget::ClangFormatConfigWidget(ProjectExplorer::Project *project, QWidget *parent)
    : CodeStyleEditorWidget(parent)
    , m_project(project)
    , m_checks(std::make_unique<Ui::ClangFormatChecksWidget>())
    , m_ui(std::make_unique<Ui::ClangFormatConfigWidget>())
{
    m_ui->setupUi(this);

    QScrollArea *scrollArea = new QScrollArea();
    m_checksWidget = new QWidget();
    m_checks->setupUi(m_checksWidget);
    scrollArea->setWidget(m_checksWidget);
    scrollArea->setMaximumWidth(470);

    m_ui->horizontalLayout_2->addWidget(scrollArea);

    for (QObject *child : m_checksWidget->children()) {
        auto comboBox = qobject_cast<QComboBox *>(child);
        if (comboBox != nullptr) {
            connect(comboBox,
                    QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this,
                    &ClangFormatConfigWidget::onTableChanged);
            comboBox->installEventFilter(this);
            continue;
        }

        auto button = qobject_cast<QPushButton *>(child);
        if (button != nullptr)
            connect(button, &QPushButton::clicked, this, &ClangFormatConfigWidget::onTableChanged);
    }

    m_preview = new TextEditor::SnippetEditorWidget(this);
    m_ui->horizontalLayout_2->addWidget(m_preview);
    if (m_project) {
        m_ui->applyButton->show();
        hideGlobalCheckboxes();
        m_ui->fallbackConfig->hide();
        m_ui->overrideDefault->setChecked(
            m_project->namedSettings(Constants::OVERRIDE_FILE_ID).toBool());
    } else {
        m_ui->applyButton->hide();
        showGlobalCheckboxes();
        m_ui->overrideDefault->setChecked(ClangFormatSettings::instance().overrideDefaultFile());
        m_ui->overrideDefault->setToolTip(
            tr("Override Clang Format configuration file with the fallback configuration."));
    }

    connect(m_ui->overrideDefault, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked)
            createStyleFileIfNeeded(!m_project);
        initialize();
    });
    initialize();
}

void ClangFormatConfigWidget::onTableChanged()
{
    const std::string newConfig = tableToString(sender());
    if (newConfig.empty())
        return;
    clang::format::FormatStyle style = m_project ? currentProjectStyle() : currentGlobalStyle();
    const std::string oldConfig = clang::format::configurationAsText(style);
    saveConfig(newConfig);
    fillTable();
    updatePreview();
    saveConfig(oldConfig);
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

    if (!m_ui->overrideDefault->isChecked() && m_project) {
        // Show the fallback configuration only globally.
        m_checksWidget->hide();
        m_preview->hide();
        m_ui->verticalLayout->addStretch(1);
        return;
    }

    m_checksWidget->show();
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

static inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

static inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
            s.end());
}

static inline void trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}

void ClangFormatConfigWidget::fillTable()
{
    clang::format::FormatStyle style = m_project ? currentProjectStyle() : currentGlobalStyle();

    using namespace std;
    const string configText = clang::format::configurationAsText(style);

    for (QObject *child : m_checksWidget->children()) {
        if (!qobject_cast<QComboBox *>(child) && !qobject_cast<QLineEdit *>(child)
            && !qobject_cast<QPlainTextEdit *>(child)) {
            continue;
        }

        const size_t index = configText.find(child->objectName().toStdString());

        auto *plainText = qobject_cast<QPlainTextEdit *>(child);
        if (plainText) {
            if (index == string::npos) {
                plainText->setPlainText("");
                continue;
            }
            size_t valueStart = configText.find('\n', index);
            size_t valueEnd;
            string value;
            QTC_ASSERT(valueStart != string::npos, continue;);
            do {
                valueEnd = configText.find('\n', valueStart + 1);
                if (valueEnd == string::npos)
                    break;
                // Skip also 2 spaces - start with valueStart + 1 + 2.
                string line = configText.substr(valueStart + 3, valueEnd - valueStart - 3);
                rtrim(line);
                value += value.empty() ? line : '\n' + line;
                valueStart = valueEnd;
            } while (valueEnd < configText.size() - 1 && configText.at(valueEnd + 1) == ' ');
            plainText->setPlainText(QString::fromStdString(value));
        } else {
            auto *comboBox = qobject_cast<QComboBox *>(child);
            auto *lineEdit = qobject_cast<QLineEdit *>(child);
            if (index == string::npos) {
                if (comboBox)
                    comboBox->setCurrentIndex(0);
                else
                    lineEdit->setText("");
                continue;
            }

            const size_t valueStart = configText.find(':', index);
            QTC_ASSERT(valueStart != string::npos, continue;);
            const size_t valueEnd = configText.find('\n', valueStart + 1);
            QTC_ASSERT(valueEnd != string::npos, continue;);
            string value = configText.substr(valueStart + 1, valueEnd - valueStart - 1);
            trim(value);

            if (comboBox)
                comboBox->setCurrentText(QString::fromStdString(value));
            else
                lineEdit->setText(QString::fromStdString(value));
        }
    }
}

std::string ClangFormatConfigWidget::tableToString(QObject *sender)
{
    std::stringstream content;
    content << "---";

    if (sender->objectName() == "BasedOnStyle") {
        auto *basedOnStyle = m_checksWidget->findChild<QComboBox *>("BasedOnStyle");
        content << "\nBasedOnStyle: " << basedOnStyle->currentText().toStdString() << '\n';
    } else {
        for (QObject *child : m_checksWidget->children()) {
            auto *label = qobject_cast<QLabel *>(child);
            if (!label)
                continue;

            QWidget *valueWidget = m_checksWidget->findChild<QWidget *>(label->text().trimmed());
            if (!valueWidget) {
                // Currently BraceWrapping only.
                content << '\n' << label->text().toStdString() << ":";
                continue;
            }

            if (!qobject_cast<QComboBox *>(valueWidget) && !qobject_cast<QLineEdit *>(valueWidget)
                && !qobject_cast<QPlainTextEdit *>(valueWidget)) {
                continue;
            }

            auto *plainText = qobject_cast<QPlainTextEdit *>(valueWidget);
            if (plainText) {
                if (plainText->toPlainText().trimmed().isEmpty())
                    continue;

                content << '\n' << label->text().toStdString() << ":";
                QStringList list = plainText->toPlainText().split('\n');
                for (const QString &line : list)
                    content << "\n  " << line.toStdString();
            } else {
                auto *comboBox = qobject_cast<QComboBox *>(valueWidget);
                std::string text;
                if (comboBox) {
                    text = comboBox->currentText().toStdString();
                } else {
                    auto *lineEdit = qobject_cast<QLineEdit *>(valueWidget);
                    QTC_ASSERT(lineEdit, continue;);
                    text = lineEdit->text().toStdString();
                }

                if (!text.empty() && text != "Default")
                    content << '\n' << label->text().toStdString() << ": " << text;
            }
        }
        content << '\n';
    }

    std::string text = content.str();
    clang::format::FormatStyle style;
    style.Language = clang::format::FormatStyle::LK_Cpp;
    const std::error_code error = clang::format::parseConfiguration(text, &style);
    if (error.value() != static_cast<int>(clang::format::ParseError::Success)) {
        QMessageBox::warning(this,
                             tr("Error in ClangFormat configuration"),
                             QString::fromStdString(error.message()));
        fillTable();
        updatePreview();
        return std::string();
    }

    return text;
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

    if (!m_checksWidget->isVisible())
        return;

    const std::string config = tableToString(this);
    if (config.empty())
        return;

    saveConfig(config);
    fillTable();
    updatePreview();
}

void ClangFormatConfigWidget::saveConfig(const std::string &text) const
{
    QString filePath = Core::ICore::userResourcePath();
    if (m_project)
        filePath += "/clang-format/" + currentProjectUniqueId();
    filePath += "/" + QLatin1String(Constants::SETTINGS_FILE_NAME);

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly))
        return;

    file.write(text.c_str());
    file.close();
}

} // namespace ClangFormat
