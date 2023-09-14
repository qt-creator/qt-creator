// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compileoutputwindow.h"

#include "buildmanager.h"
#include "projectexplorerconstants.h"
#include "projectexplorericons.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "showoutputtaskhandler.h"

#include <coreplugin/outputwindow.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/behaviorsettings.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
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

namespace ProjectExplorer::Internal {

const char SETTINGS_KEY[] = "ProjectExplorer/CompileOutput/Zoom";
const char C_COMPILE_OUTPUT[] = "ProjectExplorer.CompileOutput";
const char OPTIONS_PAGE_ID[] = "C.ProjectExplorer.CompileOutputOptions";

CompileOutputWindow::CompileOutputWindow(QAction *cancelBuildAction) :
    m_cancelBuildButton(new QToolButton),
    m_settingsButton(new QToolButton)
{
    setId("CompileOutput");
    setDisplayName(QCoreApplication::translate("QtC::ProjectExplorer", "Compile Output"));
    setPriorityInStatusBar(40);

    Core::Context context(C_COMPILE_OUTPUT);
    m_outputWindow = new Core::OutputWindow(context, SETTINGS_KEY);
    m_outputWindow->setWindowTitle(displayName());
    m_outputWindow->setWindowIcon(Icons::WINDOW.icon());
    m_outputWindow->setReadOnly(true);
    m_outputWindow->setUndoRedoEnabled(false);
    m_outputWindow->setMaxCharCount(Core::Constants::DEFAULT_MAX_CHAR_COUNT);

    Utils::ProxyAction *cancelBuildProxyButton =
            Utils::ProxyAction::proxyActionWithIcon(cancelBuildAction,
                                                    Utils::Icons::STOP_SMALL_TOOLBAR.icon());
    m_cancelBuildButton->setDefaultAction(cancelBuildProxyButton);
    m_settingsButton->setToolTip(Core::ICore::msgShowOptionsDialog());
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

    connect(this, &IOutputPane::zoomInRequested, m_outputWindow, &Core::OutputWindow::zoomIn);
    connect(this, &IOutputPane::zoomOutRequested, m_outputWindow, &Core::OutputWindow::zoomOut);
    connect(this, &IOutputPane::resetZoomRequested, m_outputWindow, &Core::OutputWindow::resetZoom);
    connect(TextEditor::TextEditorSettings::instance(), &TextEditor::TextEditorSettings::fontSettingsChanged,
            this, updateFontSettings);
    connect(TextEditor::TextEditorSettings::instance(), &TextEditor::TextEditorSettings::behaviorSettingsChanged,
            this, updateZoomEnabled);

    connect(m_settingsButton, &QToolButton::clicked, this, [] {
        Core::ICore::showOptionsDialog(OPTIONS_PAGE_ID);
    });

    qRegisterMetaType<QTextCharFormat>("QTextCharFormat");

    m_handler = new ShowOutputTaskHandler(this,
        Tr::tr("Show Compile &Output"),
        Tr::tr("Show the output that generated this issue in Compile Output."),
        Tr::tr("O"));
    ExtensionSystem::PluginManager::addObject(m_handler);
    setupContext(C_COMPILE_OUTPUT, m_outputWindow);
    updateFromSettings();

    CompileOutputSettings &s = compileOutputSettings();
    m_outputWindow->setWordWrapEnabled(s.wrapOutput());
    m_outputWindow->setMaxCharCount(s.maxCharCount());

    connect(&s.wrapOutput, &Utils::BaseAspect::changed, m_outputWindow, [this] {
        m_outputWindow->setWordWrapEnabled(compileOutputSettings().wrapOutput());
    });
    connect(&s.maxCharCount, &Utils::BaseAspect::changed, m_outputWindow, [this] {
        m_outputWindow->setMaxCharCount(compileOutputSettings().maxCharCount());
    });
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
    m_outputWindow->registerPositionOf(task.taskId, linkedOutputLines, skipLines, offset);
}

void CompileOutputWindow::flush()
{
    m_outputWindow->flush();
}

void CompileOutputWindow::reset()
{
    m_outputWindow->reset();
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

// CompileOutputSettings

CompileOutputSettings &compileOutputSettings()
{
    static CompileOutputSettings theSettings;
    return theSettings;
}

CompileOutputSettings::CompileOutputSettings()
{
    setAutoApply(false);

    wrapOutput.setSettingsKey("ProjectExplorer/Settings/WrapBuildOutput");
    wrapOutput.setDefaultValue(true);
    wrapOutput.setLabelText(Tr::tr("Word-wrap output"));

    popUp.setSettingsKey("ProjectExplorer/Settings/ShowCompilerOutput");
    popUp.setLabelText(Tr::tr("Open Compile Output when building"));

    maxCharCount.setSettingsKey("ProjectExplorer/Settings/MaxBuildOutputLines");
    maxCharCount.setRange(1, Core::Constants::DEFAULT_MAX_CHAR_COUNT);
    maxCharCount.setDefaultValue(Core::Constants::DEFAULT_MAX_CHAR_COUNT);
    maxCharCount.setToSettingsTransformation([](const QVariant &v) { return v.toInt() / 100; });
    maxCharCount.setFromSettingsTransformation([](const QVariant &v) { return v.toInt() * 100; });

    setLayouter([this] {
        using namespace Layouting;
        const QString msg = Tr::tr("Limit output to %1 characters");
        const QStringList parts = msg.split("%1") << QString() << QString();
        return Column {
            wrapOutput,
            popUp,
            Row { parts.at(0), maxCharCount, parts.at(1), st },
            st
        };
    });

    readSettings();
}

// CompileOutputSettingsPage

class CompileOutputSettingsPage final : public Core::IOptionsPage
{
public:
    CompileOutputSettingsPage()
    {
        setId(OPTIONS_PAGE_ID);
        setDisplayName(Tr::tr("Compile Output"));
        setCategory(Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &compileOutputSettings(); });
    }
};

const CompileOutputSettingsPage settingsPage;

} // ProjectExplorer::Internal
