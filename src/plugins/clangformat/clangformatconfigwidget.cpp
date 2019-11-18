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
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <texteditor/displaysettings.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>
#include <utils/executeondestruction.h>
#include <utils/qtcassert.h>
#include <utils/genericconstants.h>

#include <QFile>
#include <QMessageBox>

#include <sstream>

using namespace ProjectExplorer;

namespace ClangFormat {

static const char kFileSaveWarning[]
    = "Disable formatting on file save in the Beautifier plugin to enable this check";

static bool isBeautifierPluginActivated()
{
    const QVector<ExtensionSystem::PluginSpec *> specs = ExtensionSystem::PluginManager::plugins();
    return std::find_if(specs.begin(),
                        specs.end(),
                        [](ExtensionSystem::PluginSpec *spec) {
                            return spec->name() == "Beautifier";
                        })
           != specs.end();
}

static bool isBeautifierOnSaveActivated()
{
    if (!isBeautifierPluginActivated())
        return false;

    QSettings *s = Core::ICore::settings();
    bool activated = false;
    s->beginGroup(Utils::Constants::BEAUTIFIER_SETTINGS_GROUP);
    s->beginGroup(Utils::Constants::BEAUTIFIER_GENERAL_GROUP);
    if (s->value(Utils::Constants::BEAUTIFIER_AUTO_FORMAT_ON_SAVE, false).toBool())
        activated = true;
    s->endGroup();
    s->endGroup();
    return activated;
}

bool ClangFormatConfigWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Wheel && qobject_cast<QComboBox *>(object)) {
        event->ignore();
        return true;
    }
    return QWidget::eventFilter(object, event);
}

void ClangFormatConfigWidget::showEvent(QShowEvent *event)
{
    TextEditor::CodeStyleEditorWidget::showEvent(event);
    if (isBeautifierOnSaveActivated()) {
        bool wasEnabled = m_ui->formatOnSave->isEnabled();
        m_ui->formatOnSave->setChecked(false);
        m_ui->formatOnSave->setEnabled(false);
        m_ui->fileSaveWarning->setText(tr(kFileSaveWarning));
        if (wasEnabled)
            apply();
    } else {
        m_ui->formatOnSave->setEnabled(true);
        m_ui->fileSaveWarning->setText("");
    }
}

ClangFormatConfigWidget::ClangFormatConfigWidget(ProjectExplorer::Project *project, QWidget *parent)
    : CodeStyleEditorWidget(parent)
    , m_project(project)
    , m_checks(std::make_unique<Ui::ClangFormatChecksWidget>())
    , m_ui(std::make_unique<Ui::ClangFormatConfigWidget>())
{
    m_ui->setupUi(this);

    initChecksAndPreview();

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

    connect(m_ui->overrideDefault, &QCheckBox::toggled,
            this, &ClangFormatConfigWidget::showOrHideWidgets);
    showOrHideWidgets();

    fillTable();
    updatePreview();

    connectChecks();
}

void ClangFormatConfigWidget::initChecksAndPreview()
{
    m_checksScrollArea = new QScrollArea();
    m_checksWidget = new QWidget;
    m_checks->setupUi(m_checksWidget);
    m_checksScrollArea->setWidget(m_checksWidget);
    m_checksScrollArea->setMaximumWidth(500);

    m_ui->horizontalLayout_2->addWidget(m_checksScrollArea);

    m_preview = new TextEditor::SnippetEditorWidget(this);
    m_ui->horizontalLayout_2->addWidget(m_preview);

    TextEditor::DisplaySettings displaySettings = m_preview->displaySettings();
    displaySettings.m_visualizeWhitespace = true;
    m_preview->setDisplaySettings(displaySettings);
    m_preview->setPlainText(QLatin1String(CppTools::Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]));
    m_preview->textDocument()->setIndenter(new ClangFormatIndenter(m_preview->document()));
    m_preview->textDocument()->setFontSettings(TextEditor::TextEditorSettings::fontSettings());
    m_preview->textDocument()->setSyntaxHighlighter(new CppEditor::CppHighlighter);

    Utils::FilePath fileName;
    if (m_project) {
        connect(m_ui->applyButton, &QPushButton::clicked, this, &ClangFormatConfigWidget::apply);
        fileName = m_project->projectFilePath().pathAppended("snippet.cpp");
    } else {
        fileName = Utils::FilePath::fromString(Core::ICore::userResourcePath())
                       .pathAppended("snippet.cpp");
    }
    m_preview->textDocument()->indenter()->setFileName(fileName);
}

void ClangFormatConfigWidget::connectChecks()
{
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
}

void ClangFormatConfigWidget::onTableChanged()
{
    if (m_disableTableUpdate)
        return;

    const std::string newConfig = tableToString(sender());
    if (newConfig.empty())
        return;
    const std::string oldConfig = m_project ? currentProjectConfigText()
                                            : currentGlobalConfigText();
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
    if (isBeautifierOnSaveActivated()) {
        m_ui->formatOnSave->setChecked(false);
        m_ui->formatOnSave->setEnabled(false);
        m_ui->fileSaveWarning->setText(tr(kFileSaveWarning));
    }
}

static bool projectConfigExists()
{
    return Utils::FilePath::fromString(Core::ICore::userResourcePath())
        .pathAppended("clang-format")
        .pathAppended(currentProjectUniqueId())
        .pathAppended((Constants::SETTINGS_FILE_NAME))
        .exists();
}

void ClangFormatConfigWidget::showOrHideWidgets()
{
    m_ui->projectHasClangFormat->hide();

    QLayoutItem *lastItem = m_ui->verticalLayout->itemAt(m_ui->verticalLayout->count() - 1);
    if (lastItem->spacerItem())
        m_ui->verticalLayout->removeItem(lastItem);

    if (!m_ui->overrideDefault->isChecked() && m_project) {
        // Show the fallback configuration only globally.
        m_checksScrollArea->hide();
        m_preview->hide();
        m_ui->verticalLayout->addStretch(1);
        return;
    }

    createStyleFileIfNeeded(!m_project);
    m_checksScrollArea->show();
    m_preview->show();

    if (m_project) {
        m_ui->projectHasClangFormat->hide();
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
    }
}

void ClangFormatConfigWidget::updatePreview()
{
    QTextCursor cursor(m_preview->document());
    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    m_preview->textDocument()->autoIndent(cursor);
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

static void fillPlainText(QPlainTextEdit *plainText, const std::string &text, size_t index)
{
    if (index == std::string::npos) {
        plainText->setPlainText("");
        return;
    }
    size_t valueStart = text.find('\n', index + 1);
    size_t valueEnd;
    std::string value;
    QTC_ASSERT(valueStart != std::string::npos, return;);
    do {
        valueEnd = text.find('\n', valueStart + 1);
        if (valueEnd == std::string::npos)
            break;
        // Skip also 2 spaces - start with valueStart + 1 + 2.
        std::string line = text.substr(valueStart + 3, valueEnd - valueStart - 3);
        rtrim(line);
        value += value.empty() ? line : '\n' + line;
        valueStart = valueEnd;
    } while (valueEnd < text.size() - 1 && text.at(valueEnd + 1) == ' ');
    plainText->setPlainText(QString::fromStdString(value));
}

static void fillComboBoxOrLineEdit(QObject *object, const std::string &text, size_t index)
{
    auto *comboBox = qobject_cast<QComboBox *>(object);
    auto *lineEdit = qobject_cast<QLineEdit *>(object);
    if (index == std::string::npos) {
        if (comboBox)
            comboBox->setCurrentIndex(0);
        else
            lineEdit->setText("");
        return;
    }

    const size_t valueStart = text.find(':', index + 1);
    QTC_ASSERT(valueStart != std::string::npos, return;);
    const size_t valueEnd = text.find('\n', valueStart + 1);
    QTC_ASSERT(valueEnd != std::string::npos, return;);
    std::string value = text.substr(valueStart + 1, valueEnd - valueStart - 1);
    trim(value);

    if (comboBox)
        comboBox->setCurrentText(QString::fromStdString(value));
    else
        lineEdit->setText(QString::fromStdString(value));
}

void ClangFormatConfigWidget::fillTable()
{
    Utils::ExecuteOnDestruction executeOnDestruction([this]() { m_disableTableUpdate = false; });
    m_disableTableUpdate = true;

    const std::string configText = m_project ? currentProjectConfigText()
                                             : currentGlobalConfigText();

    for (QObject *child : m_checksWidget->children()) {
        if (!qobject_cast<QComboBox *>(child) && !qobject_cast<QLineEdit *>(child)
            && !qobject_cast<QPlainTextEdit *>(child)) {
            continue;
        }

        size_t index = configText.find('\n' + child->objectName().toStdString());
        if (index == std::string::npos)
            index = configText.find("\n  " + child->objectName().toStdString());

        if (qobject_cast<QPlainTextEdit *>(child))
            fillPlainText(qobject_cast<QPlainTextEdit *>(child), configText, index);
        else
            fillComboBoxOrLineEdit(child, configText, index);
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
