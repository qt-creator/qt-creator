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
#include "clangformatfile.h"
#include "clangformatsettings.h"
#include "clangformatutils.h"
#include "ui_clangformatchecks.h"
#include "ui_clangformatconfigwidget.h"

#include <clang/Format/Format.h>

#include <coreplugin/icore.h>
#include <cppeditor/cpphighlighter.h>
#include <cppeditor/cppcodestylesettings.h>
#include <cppeditor/cppcodestylesnippets.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/editorconfiguration.h>
#include <texteditor/displaysettings.h>
#include <texteditor/icodestylepreferences.h>
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

static int indentIndex() { return 0; }
static int formatIndex() { return 1; }


bool ClangFormatConfigWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Wheel && qobject_cast<QComboBox *>(object)) {
        event->ignore();
        return true;
    }
    return QWidget::eventFilter(object, event);
}

ClangFormatConfigWidget::ClangFormatConfigWidget(TextEditor::ICodeStylePreferences *codeStyle,
                                                 ProjectExplorer::Project *project,
                                                 QWidget *parent)
    : CppCodeStyleWidget(parent)
    , m_project(project)
    , m_checks(std::make_unique<Ui::ClangFormatChecksWidget>())
    , m_ui(std::make_unique<Ui::ClangFormatConfigWidget>())
{
    m_ui->setupUi(this);
    setEnabled(!codeStyle->isReadOnly());

    m_config = std::make_unique<ClangFormatFile>(filePathToCurrentSettings(codeStyle),
                                                 codeStyle->isReadOnly());

    initChecksAndPreview();
    showCombobox();

    if (m_project) {
        m_ui->overrideDefault->setChecked(
            m_project->namedSettings(Constants::OVERRIDE_FILE_ID).toBool());
    } else {
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

ClangFormatConfigWidget::~ClangFormatConfigWidget() = default;

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
    m_preview->setPlainText(QLatin1String(CppEditor::Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]));
    m_preview->textDocument()->setIndenter(new ClangFormatIndenter(m_preview->document()));
    m_preview->textDocument()->setFontSettings(TextEditor::TextEditorSettings::fontSettings());
    m_preview->textDocument()->setSyntaxHighlighter(new CppEditor::CppHighlighter);

    Utils::FilePath fileName;
    if (m_project) {
        fileName = m_project->projectFilePath().pathAppended("snippet.cpp");
    } else {
        fileName = Core::ICore::userResourcePath("snippet.cpp");
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

        const auto button = qobject_cast<QPushButton *>(child);
        if (button != nullptr)
            connect(button, &QPushButton::clicked, this, &ClangFormatConfigWidget::onTableChanged);
    }
}

void ClangFormatConfigWidget::onTableChanged()
{
    if (m_disableTableUpdate)
        return;

    saveChanges(sender());
}

void ClangFormatConfigWidget::showCombobox()
{
    m_ui->indentingOrFormatting->insertItem(indentIndex(), tr("Indenting only"));
    m_ui->indentingOrFormatting->insertItem(formatIndex(), tr("Full formatting"));

    if (ClangFormatSettings::instance().formatCodeInsteadOfIndent())
        m_ui->indentingOrFormatting->setCurrentIndex(formatIndex());
    else
        m_ui->indentingOrFormatting->setCurrentIndex(indentIndex());

    m_ui->indentingOrFormatting->show();
}

static bool projectConfigExists()
{
    return Core::ICore::userResourcePath()
        .pathAppended("clang-format")
        .pathAppended(currentProjectUniqueId())
        .pathAppended(Constants::SETTINGS_FILE_NAME)
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

    if (!m_project) {
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

    const std::string configText = readFile(m_config->filePath().path());

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

void ClangFormatConfigWidget::saveChanges(QObject *sender)
{
    if (sender->objectName() == "BasedOnStyle") {
        const auto *basedOnStyle = m_checksWidget->findChild<QComboBox *>("BasedOnStyle");
        m_config->setBasedOnStyle(basedOnStyle->currentText());
    } else {
        QList<ClangFormatFile::Field> fields;

        for (QObject *child : m_checksWidget->children()) {
            if (child->objectName() == "BasedOnStyle")
                continue;
            auto *label = qobject_cast<QLabel *>(child);
            if (!label)
                continue;

            QWidget *valueWidget = m_checksWidget->findChild<QWidget *>(label->text().trimmed());
            if (!valueWidget) {
                // Currently BraceWrapping only.
                fields.append({label->text(), ""});
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


                std::stringstream content;
                QStringList list = plainText->toPlainText().split('\n');
                for (const QString &line : list)
                    content << "\n  " << line.toStdString();

                fields.append({label->text(), QString::fromStdString(content.str())});
            } else {
                QString text;
                if (auto *comboBox = qobject_cast<QComboBox *>(valueWidget)) {
                    text = comboBox->currentText();
                } else {
                    auto *lineEdit = qobject_cast<QLineEdit *>(valueWidget);
                    QTC_ASSERT(lineEdit, continue;);
                    text = lineEdit->text();
                }

                if (!text.isEmpty() && text != "Default")
                    fields.append({label->text(), text});
            }
        }
        m_config->changeFields(fields);
    }

    fillTable();
    updatePreview();
    synchronize();
}

void ClangFormatConfigWidget::setCodeStyleSettings(const CppEditor::CppCodeStyleSettings &settings)
{
    m_config->fromCppCodeStyleSettings(settings);

    fillTable();
    updatePreview();
}

void ClangFormatConfigWidget::setTabSettings(const TextEditor::TabSettings &settings)
{
    m_config->fromTabSettings(settings);

    fillTable();
    updatePreview();
}

void ClangFormatConfigWidget::synchronize()
{
    emit codeStyleSettingsChanged(m_config->toCppCodeStyleSettings(m_project));
    emit tabSettingsChanged(m_config->toTabSettings(m_project));
}

void ClangFormatConfigWidget::apply()
{
    ClangFormatSettings &settings = ClangFormatSettings::instance();
    const bool isFormatting = m_ui->indentingOrFormatting->currentIndex()
                              == formatIndex();
    settings.setFormatCodeInsteadOfIndent(isFormatting);
    settings.setOverrideDefaultFile(m_ui->overrideDefault->isChecked());

    if (!isBeautifierOnSaveActivated()) {
        settings.setFormatOnSave(isFormatting);
    }

    if (m_project) {
        m_project->setNamedSettings(Constants::OVERRIDE_FILE_ID, m_ui->overrideDefault->isChecked());
    }
    settings.write();

    if (!m_checksWidget->isVisible())
        return;

    saveChanges(this);
}

} // namespace ClangFormat
