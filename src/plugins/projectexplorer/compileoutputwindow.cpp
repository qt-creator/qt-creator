/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "compileoutputwindow.h"

#include "buildmanager.h"
#include "ioutputparser.h"
#include "projectexplorer.h"
#include "projectexplorericons.h"
#include "projectexplorersettings.h"
#include "showoutputtaskhandler.h"
#include "task.h"
#include "taskhub.h"

#include <coreplugin/outputwindow.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/behaviorsettings.h>
#include <utils/algorithm.h>
#include <utils/outputformatter.h>
#include <utils/proxyaction.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QToolButton>
#include <QVBoxLayout>

namespace ProjectExplorer {
namespace Internal {

const char SETTINGS_KEY[] = "ProjectExplorer/CompileOutput/Zoom";
const char C_COMPILE_OUTPUT[] = "ProjectExplorer.CompileOutput";
const char POP_UP_KEY[] = "ProjectExplorer/Settings/ShowCompilerOutput";
const char WRAP_OUTPUT_KEY[] = "ProjectExplorer/Settings/WrapBuildOutput";
const char MAX_LINES_KEY[] = "ProjectExplorer/Settings/MaxBuildOutputLines";
const char OPTIONS_PAGE_ID[] = "C.ProjectExplorer.CompileOutputOptions";

CompileOutputWindow::CompileOutputWindow(QAction *cancelBuildAction) :
    m_cancelBuildButton(new QToolButton),
    m_settingsButton(new QToolButton)
{
    Core::Context context(C_COMPILE_OUTPUT);
    m_outputWindow = new Core::OutputWindow(context, SETTINGS_KEY);
    m_outputWindow->setWindowTitle(displayName());
    m_outputWindow->setWindowIcon(Icons::WINDOW.icon());
    m_outputWindow->setReadOnly(true);
    m_outputWindow->setUndoRedoEnabled(false);
    m_outputWindow->setMaxCharCount(Core::Constants::DEFAULT_MAX_CHAR_COUNT);

    outputFormatter()->overridePostPrintAction([this](Utils::OutputLineParser *parser) {
        if (const auto taskParser = qobject_cast<OutputTaskParser *>(parser)) {
            int offset = 0;
            Utils::reverseForeach(taskParser->taskInfo(), [this, &offset](const OutputTaskParser::TaskInfo &ti) {
                registerPositionOf(ti.task, ti.linkedLines, ti.skippedLines, offset);
                offset += ti.linkedLines;
            });
        }
        parser->runPostPrintActions();
    });

    // Let selected text be colored as if the text edit was editable,
    // otherwise the highlight for searching is too light
    QPalette p = m_outputWindow->palette();
    QColor activeHighlight = p.color(QPalette::Active, QPalette::Highlight);
    p.setColor(QPalette::Highlight, activeHighlight);
    QColor activeHighlightedText = p.color(QPalette::Active, QPalette::HighlightedText);
    p.setColor(QPalette::HighlightedText, activeHighlightedText);
    m_outputWindow->setPalette(p);

    Utils::ProxyAction *cancelBuildProxyButton =
            Utils::ProxyAction::proxyActionWithIcon(cancelBuildAction,
                                                    Utils::Icons::STOP_SMALL_TOOLBAR.icon());
    m_cancelBuildButton->setDefaultAction(cancelBuildProxyButton);
    m_settingsButton->setToolTip(tr("Open Settings Page"));
    m_settingsButton->setIcon(Utils::Icons::SETTINGS_TOOLBAR.icon());

    auto updateFontSettings = [this] {
        m_outputWindow->setBaseFont(TextEditor::TextEditorSettings::fontSettings().font());
    };

    auto updateZoomEnabled = [this] {
        m_outputWindow->setWheelZoomEnabled(
                    TextEditor::TextEditorSettings::behaviorSettings().m_scrollWheelZooming);
    };

    updateFontSettings();
    updateZoomEnabled();
    setupFilterUi("CompileOutputPane.Filter");
    setFilteringEnabled(true);

    connect(this, &IOutputPane::zoomIn, m_outputWindow, &Core::OutputWindow::zoomIn);
    connect(this, &IOutputPane::zoomOut, m_outputWindow, &Core::OutputWindow::zoomOut);
    connect(this, &IOutputPane::resetZoom, m_outputWindow, &Core::OutputWindow::resetZoom);
    connect(TextEditor::TextEditorSettings::instance(), &TextEditor::TextEditorSettings::fontSettingsChanged,
            this, updateFontSettings);
    connect(TextEditor::TextEditorSettings::instance(), &TextEditor::TextEditorSettings::behaviorSettingsChanged,
            this, updateZoomEnabled);

    connect(m_settingsButton, &QToolButton::clicked, this, [] {
        Core::ICore::showOptionsDialog(OPTIONS_PAGE_ID);
    });

    auto agg = new Aggregation::Aggregate;
    agg->add(m_outputWindow);
    agg->add(new Core::BaseTextFind(m_outputWindow));

    qRegisterMetaType<QTextCharFormat>("QTextCharFormat");

    m_handler = new ShowOutputTaskHandler(this);
    ExtensionSystem::PluginManager::addObject(m_handler);
    setupContext(C_COMPILE_OUTPUT, m_outputWindow);
    loadSettings();
    updateFromSettings();
}

CompileOutputWindow::~CompileOutputWindow()
{
    ExtensionSystem::PluginManager::removeObject(m_handler);
    delete m_handler;
    delete m_cancelBuildButton;
    delete m_settingsButton;
}

void CompileOutputWindow::updateFromSettings()
{
    m_outputWindow->setWordWrapEnabled(m_settings.wrapOutput);
    m_outputWindow->setMaxCharCount(m_settings.maxCharCount);
}

bool CompileOutputWindow::hasFocus() const
{
    return m_outputWindow->window()->focusWidget() == m_outputWindow;
}

bool CompileOutputWindow::canFocus() const
{
    return true;
}

void CompileOutputWindow::setFocus()
{
    m_outputWindow->setFocus();
}

QWidget *CompileOutputWindow::outputWidget(QWidget *)
{
    return m_outputWindow;
}

QList<QWidget *> CompileOutputWindow::toolBarWidgets() const
{
    return QList<QWidget *>{m_cancelBuildButton, m_settingsButton} + IOutputPane::toolBarWidgets();
}

void CompileOutputWindow::appendText(const QString &text, BuildStep::OutputFormat format)
{
    Utils::OutputFormat fmt = Utils::NormalMessageFormat;
    switch (format) {
    case BuildStep::OutputFormat::Stdout:
        fmt = Utils::StdOutFormat;
        break;
    case BuildStep::OutputFormat::Stderr:
        fmt = Utils::StdErrFormat;
        break;
    case BuildStep::OutputFormat::NormalMessage:
        fmt = Utils::NormalMessageFormat;
        break;
    case BuildStep::OutputFormat::ErrorMessage:
        fmt = Utils::ErrorMessageFormat;
        break;

    }

    m_outputWindow->appendMessage(text, fmt);
}

void CompileOutputWindow::clearContents()
{
    m_outputWindow->clear();
    m_taskPositions.clear();
}

void CompileOutputWindow::visibilityChanged(bool)
{ }

int CompileOutputWindow::priorityInStatusBar() const
{
    return 50;
}

bool CompileOutputWindow::canNext() const
{
    return false;
}

bool CompileOutputWindow::canPrevious() const
{
    return false;
}

void CompileOutputWindow::goToNext()
{ }

void CompileOutputWindow::goToPrev()
{ }

bool CompileOutputWindow::canNavigate() const
{
    return false;
}

void CompileOutputWindow::registerPositionOf(const Task &task, int linkedOutputLines, int skipLines,
                                             int offset)
{
    if (linkedOutputLines <= 0)
        return;

    const int blocknumber = m_outputWindow->document()->blockCount() - offset - 1;
    const int firstLine = blocknumber - linkedOutputLines - skipLines;
    const int lastLine = firstLine + linkedOutputLines - 1;

    m_taskPositions.insert(task.taskId, qMakePair(firstLine, lastLine));
}

bool CompileOutputWindow::knowsPositionOf(const Task &task)
{
    return (m_taskPositions.contains(task.taskId));
}

void CompileOutputWindow::showPositionOf(const Task &task)
{
    QPair<int, int> position = m_taskPositions.value(task.taskId);
    QTextCursor newCursor(m_outputWindow->document()->findBlockByNumber(position.second));

    // Move cursor to end of last line of interest:
    newCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
    m_outputWindow->setTextCursor(newCursor);

    // Move cursor and select lines:
    newCursor.setPosition(m_outputWindow->document()->findBlockByNumber(position.first).position(),
                          QTextCursor::KeepAnchor);
    m_outputWindow->setTextCursor(newCursor);

    // Center cursor now:
    m_outputWindow->centerCursor();
}

void CompileOutputWindow::flush()
{
    m_outputWindow->flush();
}

void CompileOutputWindow::reset()
{
    m_outputWindow->reset();
}

void CompileOutputWindow::setSettings(const CompileOutputSettings &settings)
{
    m_settings = settings;
    storeSettings();
    updateFromSettings();
}

Utils::OutputFormatter *CompileOutputWindow::outputFormatter() const
{
    return m_outputWindow->outputFormatter();
}

void CompileOutputWindow::updateFilter()
{
    m_outputWindow->updateFilterProperties(filterText(), filterCaseSensitivity(),
                                           filterUsesRegexp(), filterIsInverted());
}

void CompileOutputWindow::loadSettings()
{
    QSettings * const s = Core::ICore::settings();
    m_settings.popUp = s->value(POP_UP_KEY, false).toBool();
    m_settings.wrapOutput = s->value(WRAP_OUTPUT_KEY, true).toBool();
    m_settings.maxCharCount = s->value(MAX_LINES_KEY,
                                       Core::Constants::DEFAULT_MAX_CHAR_COUNT).toInt() * 100;
}

void CompileOutputWindow::storeSettings() const
{
    QSettings * const s = Core::ICore::settings();
    s->setValue(POP_UP_KEY, m_settings.popUp);
    s->setValue(WRAP_OUTPUT_KEY, m_settings.wrapOutput);
    s->setValue(MAX_LINES_KEY, m_settings.maxCharCount / 100);
}

class CompileOutputSettingsWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Internal::CompileOutputSettingsPage)
public:
    CompileOutputSettingsWidget()
    {
        const CompileOutputSettings &settings = BuildManager::compileOutputSettings();
        m_wrapOutputCheckBox.setText(tr("Word-wrap output"));
        m_wrapOutputCheckBox.setChecked(settings.wrapOutput);
        m_popUpCheckBox.setText(tr("Open pane when building"));
        m_popUpCheckBox.setChecked(settings.popUp);
        m_maxCharsBox.setMaximum(100000000);
        m_maxCharsBox.setValue(settings.maxCharCount);
        const auto layout = new QVBoxLayout(this);
        layout->addWidget(&m_wrapOutputCheckBox);
        layout->addWidget(&m_popUpCheckBox);
        const auto maxCharsLayout = new QHBoxLayout;
        const QString msg = tr("Limit output to %1 characters");
        const QStringList parts = msg.split("%1") << QString() << QString();
        maxCharsLayout->addWidget(new QLabel(parts.at(0).trimmed()));
        maxCharsLayout->addWidget(&m_maxCharsBox);
        maxCharsLayout->addWidget(new QLabel(parts.at(1).trimmed()));
        maxCharsLayout->addStretch(1);
        layout->addLayout(maxCharsLayout);
        layout->addStretch(1);
    }

    void apply() final
    {
        CompileOutputSettings s;
        s.wrapOutput = m_wrapOutputCheckBox.isChecked();
        s.popUp = m_popUpCheckBox.isChecked();
        s.maxCharCount = m_maxCharsBox.value();
        BuildManager::setCompileOutputSettings(s);
    }

private:
    QCheckBox m_wrapOutputCheckBox;
    QCheckBox m_popUpCheckBox;
    QSpinBox m_maxCharsBox;
};

CompileOutputSettingsPage::CompileOutputSettingsPage()
{
    setId(OPTIONS_PAGE_ID);
    setDisplayName(CompileOutputSettingsWidget::tr("Compile Output"));
    setCategory(Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new CompileOutputSettingsWidget; });
}

} // Internal
} // ProjectExplorer
