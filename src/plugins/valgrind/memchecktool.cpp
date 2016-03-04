/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "memchecktool.h"
#include "memcheckengine.h"
#include "memcheckerrorview.h"
#include "valgrindsettings.h"
#include "valgrindplugin.h"

#include <debugger/analyzer/analyzerconstants.h>
#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerstartparameters.h>
#include <debugger/analyzer/analyzerutils.h>
#include <debugger/analyzer/startremotedialog.h>

#include <valgrind/valgrindsettings.h>
#include <valgrind/xmlprotocol/errorlistmodel.h>
#include <valgrind/xmlprotocol/stackmodel.h>
#include <valgrind/xmlprotocol/error.h>
#include <valgrind/xmlprotocol/frame.h>
#include <valgrind/xmlprotocol/stack.h>
#include <valgrind/xmlprotocol/suppression.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/buildconfiguration.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>

#include <utils/fancymainwindow.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLatin1String>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QString>
#include <QToolButton>

using namespace Analyzer;
using namespace ProjectExplorer;
using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Internal {

class FrameFinder;

class MemcheckErrorFilterProxyModel : public QSortFilterProxyModel
{
public:
    MemcheckErrorFilterProxyModel(QObject *parent = 0);

public:
    void setAcceptedKinds(const QList<int> &acceptedKinds);
    void setFilterExternalIssues(bool filter);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    QList<int> m_acceptedKinds;
    bool m_filterExternalIssues;
};

MemcheckErrorFilterProxyModel::MemcheckErrorFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent),
      m_filterExternalIssues(false)
{
}

void MemcheckErrorFilterProxyModel::setAcceptedKinds(const QList<int> &acceptedKinds)
{
    if (m_acceptedKinds != acceptedKinds) {
        m_acceptedKinds = acceptedKinds;
        invalidate();
    }
}

void MemcheckErrorFilterProxyModel::setFilterExternalIssues(bool filter)
{
    if (m_filterExternalIssues != filter) {
        m_filterExternalIssues = filter;
        invalidate();
    }
}

bool MemcheckErrorFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // We only deal with toplevel items.
    if (sourceParent.isValid())
        return true;

    // Because toplevel items have no parent, we can't use sourceParent to find them. we just use
    // sourceParent as an invalid index, telling the model that the index we're looking for has no
    // parent.
    QAbstractItemModel *model = sourceModel();
    QModelIndex sourceIndex = model->index(sourceRow, filterKeyColumn(), sourceParent);
    if (!sourceIndex.isValid())
        return true;

    const Error error = sourceIndex.data(ErrorListModel::ErrorRole).value<Error>();

    // Filter on kind
    if (!m_acceptedKinds.contains(error.kind()))
        return false;

    // Filter non-project stuff
    if (m_filterExternalIssues && !error.stacks().isEmpty()) {
        // ALGORITHM: look at last five stack frames, if none of these is inside any open projects,
        // assume this error was created by an external library
        QSet<QString> validFolders;
        foreach (Project *project, SessionManager::projects()) {
            validFolders << project->projectDirectory().toString();
            foreach (Target *target, project->targets()) {
                foreach (const DeployableFile &file,
                         target->deploymentData().allFiles()) {
                    if (file.isExecutable())
                        validFolders << file.remoteDirectory();
                }
                foreach (BuildConfiguration *config, target->buildConfigurations())
                    validFolders << config->buildDirectory().toString();
            }
        }

        const QVector< Frame > frames = error.stacks().first().frames();

        const int framesToLookAt = qMin(6, frames.size());

        bool inProject = false;
        for (int i = 0; i < framesToLookAt; ++i) {
            const Frame &frame = frames.at(i);
            foreach (const QString &folder, validFolders) {
                if (frame.directory().startsWith(folder)) {
                    inProject = true;
                    break;
                }
            }
        }
        if (!inProject)
            return false;
    }

    return true;
}

static void initKindFilterAction(QAction *action, const QVariantList &kinds)
{
    action->setCheckable(true);
    action->setData(kinds);
}

class MemcheckTool : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(Valgrind::Internal::MemcheckTool)

public:
    MemcheckTool(QObject *parent);

    QWidget *createWidgets();

    MemcheckRunControl *createRunControl(ProjectExplorer::RunConfiguration *runConfiguration,
                                         Core::Id runMode);

private:
    void settingsDestroyed(QObject *settings);
    void maybeActiveRunConfigurationChanged();

    void engineStarting(const MemcheckRunControl *engine);
    void engineFinished();
    void loadingExternalXmlLogFileFinished();

    void parserError(const Valgrind::XmlProtocol::Error &error);
    void internalParserError(const QString &errorString);
    void updateErrorFilter();

    void loadExternalXmlLogFile();

    void setBusyCursor(bool busy);

    void clearErrorView();
    void updateFromSettings();
    int updateUiAfterFinishedHelper();

private:
    ValgrindBaseSettings *m_settings;
    QMenu *m_filterMenu;

    FrameFinder *m_frameFinder;
    Valgrind::XmlProtocol::ErrorListModel *m_errorModel;
    MemcheckErrorFilterProxyModel *m_errorProxyModel;
    MemcheckErrorView *m_errorView;

    QList<QAction *> m_errorFilterActions;
    QAction *m_filterProjectAction;
    QList<QAction *> m_suppressionActions;
    QAction *m_suppressionSeparator;
    QAction *m_loadExternalLogFile;
    QAction *m_goBack;
    QAction *m_goNext;
};

MemcheckTool::MemcheckTool(QObject *parent)
  : QObject(parent)
{
    m_settings = ValgrindPlugin::globalSettings();
    m_errorModel = 0;
    m_errorProxyModel = 0;
    m_errorView = 0;
    m_filterMenu = 0;

    setObjectName(QLatin1String("MemcheckTool"));

    m_filterProjectAction = new QAction(tr("External Errors"), this);
    m_filterProjectAction->setToolTip(
        tr("Show issues originating outside currently opened projects."));
    m_filterProjectAction->setCheckable(true);

    m_suppressionSeparator = new QAction(tr("Suppressions"), this);
    m_suppressionSeparator->setSeparator(true);
    m_suppressionSeparator->setToolTip(
        tr("These suppression files were used in the last memory analyzer run."));

    QAction *a = new QAction(tr("Definite Memory Leaks"), this);
    initKindFilterAction(a, { Leak_DefinitelyLost, Leak_IndirectlyLost });
    m_errorFilterActions.append(a);

    a = new QAction(tr("Possible Memory Leaks"), this);
    initKindFilterAction(a, { Leak_PossiblyLost, Leak_StillReachable });
    m_errorFilterActions.append(a);

    a = new QAction(tr("Use of Uninitialized Memory"), this);
    initKindFilterAction(a, { InvalidRead, InvalidWrite, InvalidJump, Overlap,
                              InvalidMemPool, UninitCondition, UninitValue,
                              SyscallParam, ClientCheck });
    m_errorFilterActions.append(a);

    a = new QAction(tr("Invalid Calls to \"free()\""), this);
    initKindFilterAction(a, { InvalidFree,  MismatchedFree });
    m_errorFilterActions.append(a);


    using namespace std::placeholders;

    AnalyzerManager::registerToolbar(MemcheckPerspectiveId, createWidgets());

    ActionDescription desc;
    desc.setToolTip(tr("Valgrind Analyze Memory uses the "
         "Memcheck tool to find memory leaks."));

    if (!Utils::HostOsInfo::isWindowsHost()) {
        desc.setText(tr("Valgrind Memory Analyzer"));
        desc.setPerspectiveId(MemcheckPerspectiveId);
        desc.setRunControlCreator(std::bind(&MemcheckTool::createRunControl, this, _1, _2));
        desc.setToolMode(DebugMode);
        desc.setRunMode(MEMCHECK_RUN_MODE);
        desc.setMenuGroup(Analyzer::Constants::G_ANALYZER_TOOLS);
        AnalyzerManager::registerAction("Memcheck.Local", desc);

        desc.setText(tr("Valgrind Memory Analyzer with GDB"));
        desc.setToolTip(tr("Valgrind Analyze Memory with GDB uses the "
            "Memcheck tool to find memory leaks.\nWhen a problem is detected, "
            "the application is interrupted and can be debugged."));
        desc.setPerspectiveId(MemcheckPerspectiveId);
        desc.setRunControlCreator(std::bind(&MemcheckTool::createRunControl, this, _1, _2));
        desc.setToolMode(DebugMode);
        desc.setRunMode(MEMCHECK_WITH_GDB_RUN_MODE);
        desc.setMenuGroup(Analyzer::Constants::G_ANALYZER_TOOLS);
        AnalyzerManager::registerAction("MemcheckWithGdb.Local", desc);
    }

    desc.setText(tr("Valgrind Memory Analyzer (External Remote Application)"));
    desc.setPerspectiveId(MemcheckPerspectiveId);
    desc.setCustomToolStarter([this](ProjectExplorer::RunConfiguration *runConfig) {
        StartRemoteDialog dlg;
        if (dlg.exec() != QDialog::Accepted)
            return;
        ValgrindRunControl *rc = createRunControl(runConfig, MEMCHECK_RUN_MODE);
        QTC_ASSERT(rc, return);
        const auto runnable = dlg.runnable();
        rc->setRunnable(runnable);
        AnalyzerConnection connection;
        connection.connParams = dlg.sshParams();
        rc->setConnection(connection);
        rc->setDisplayName(runnable.executable);
        ProjectExplorerPlugin::startRunControl(rc, MEMCHECK_RUN_MODE);
    });
    desc.setMenuGroup(Analyzer::Constants::G_ANALYZER_REMOTE_TOOLS);
    AnalyzerManager::registerAction("Memcheck.Remote", desc);
}

void MemcheckTool::settingsDestroyed(QObject *settings)
{
    QTC_ASSERT(m_settings == settings, return);
    m_settings = ValgrindPlugin::globalSettings();
}

void MemcheckTool::updateFromSettings()
{
    foreach (QAction *action, m_errorFilterActions) {
        bool contained = true;
        foreach (const QVariant &v, action->data().toList()) {
            bool ok;
            int kind = v.toInt(&ok);
            if (ok && !m_settings->visibleErrorKinds().contains(kind))
                contained = false;
        }
        action->setChecked(contained);
    }

    m_filterProjectAction->setChecked(!m_settings->filterExternalIssues());
    m_errorView->settingsChanged(m_settings);

    connect(m_settings, &ValgrindBaseSettings::visibleErrorKindsChanged,
            m_errorProxyModel, &MemcheckErrorFilterProxyModel::setAcceptedKinds);
    m_errorProxyModel->setAcceptedKinds(m_settings->visibleErrorKinds());

    connect(m_settings, &ValgrindBaseSettings::filterExternalIssuesChanged,
            m_errorProxyModel, &MemcheckErrorFilterProxyModel::setFilterExternalIssues);
    m_errorProxyModel->setFilterExternalIssues(m_settings->filterExternalIssues());
}

void MemcheckTool::maybeActiveRunConfigurationChanged()
{
    ValgrindBaseSettings *settings = 0;
    if (Project *project = SessionManager::startupProject())
        if (Target *target = project->activeTarget())
            if (RunConfiguration *rc = target->activeRunConfiguration())
                if (IRunConfigurationAspect *aspect = rc->extraAspect(ANALYZER_VALGRIND_SETTINGS))
                    settings = qobject_cast<ValgrindBaseSettings *>(aspect->currentSettings());

    if (!settings) // fallback to global settings
        settings = ValgrindPlugin::globalSettings();

    if (m_settings == settings)
        return;

    // disconnect old settings class if any
    if (m_settings) {
        m_settings->disconnect(this);
        m_settings->disconnect(m_errorProxyModel);
    }

    // now make the new settings current, update and connect input widgets
    m_settings = settings;
    QTC_ASSERT(m_settings, return);
    connect(m_settings, &ValgrindBaseSettings::destroyed, this, &MemcheckTool::settingsDestroyed);

    updateFromSettings();
}

class FrameFinder : public ErrorListModel::RelevantFrameFinder
{
public:
    Frame findRelevant(const Error &error) const
    {
        const QVector<Stack> stacks = error.stacks();
        if (stacks.isEmpty())
            return Frame();
        const Stack &stack = stacks[0];
        const QVector<Frame> frames = stack.frames();
        if (frames.isEmpty())
            return Frame();

        //find the first frame belonging to the project
        if (!m_projectFiles.isEmpty()) {
            foreach (const Frame &frame, frames) {
                if (frame.directory().isEmpty() || frame.fileName().isEmpty())
                    continue;

                //filepaths can contain "..", clean them:
                const QString f = QFileInfo(frame.filePath()).absoluteFilePath();
                if (m_projectFiles.contains(f))
                    return frame;
            }
        }

        //if no frame belonging to the project was found, return the first one that is not malloc/new
        foreach (const Frame &frame, frames) {
            if (!frame.functionName().isEmpty() && frame.functionName() != QLatin1String("malloc")
                && !frame.functionName().startsWith(QLatin1String("operator new(")))
            {
                return frame;
            }
        }

        //else fallback to the first frame
        return frames.first();
    }
    void setFiles(const QStringList &files)
    {
        m_projectFiles = files;
    }
private:
    QStringList m_projectFiles;
};


QWidget *MemcheckTool::createWidgets()
{
    QTC_ASSERT(!m_errorView, return 0);

    m_errorView = new MemcheckErrorView;
    m_errorView->setObjectName(QLatin1String("MemcheckErrorView"));
    m_errorView->setFrameStyle(QFrame::NoFrame);
    m_errorView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_errorModel = new ErrorListModel(m_errorView);
    m_frameFinder = new Internal::FrameFinder;
    m_errorModel->setRelevantFrameFinder(QSharedPointer<Internal::FrameFinder>(m_frameFinder));
    m_errorProxyModel = new MemcheckErrorFilterProxyModel(m_errorView);
    m_errorProxyModel->setSourceModel(m_errorModel);
    m_errorProxyModel->setDynamicSortFilter(true);
    m_errorView->setModel(m_errorProxyModel);
    m_errorView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    // make m_errorView->selectionModel()->selectedRows() return something
    m_errorView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_errorView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_errorView->setAutoScroll(false);
    m_errorView->setObjectName(QLatin1String("Valgrind.MemcheckTool.ErrorView"));
    m_errorView->setWindowTitle(tr("Memory Issues"));

    AnalyzerManager::registerDockWidget(MemcheckErrorDockId, m_errorView);

    AnalyzerManager::registerPerspective(MemcheckPerspectiveId, {
        { MemcheckErrorDockId, Core::Id(), Perspective::SplitVertical }
    });

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::updateRunActions,
            this, &MemcheckTool::maybeActiveRunConfigurationChanged);

    //
    // The Control Widget.
    //
    QAction *action = 0;
    QHBoxLayout *layout = new QHBoxLayout;
    QToolButton *button = 0;

    layout->setMargin(0);
    layout->setSpacing(0);

    // Load external XML log file
    action = new QAction(this);
    action->setIcon(Core::Icons::OPENFILE.icon());
    action->setToolTip(tr("Load External XML Log File"));
    connect(action, &QAction::triggered, this, &MemcheckTool::loadExternalXmlLogFile);
    button = new QToolButton;
    button->setDefaultAction(action);
    layout->addWidget(button);
    m_loadExternalLogFile = action;

    // Go to previous leak.
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Core::Icons::PREV.icon());
    action->setToolTip(tr("Go to previous leak."));
    connect(action, &QAction::triggered, m_errorView, &MemcheckErrorView::goBack);
    button = new QToolButton;
    button->setDefaultAction(action);
    layout->addWidget(button);
    m_goBack = action;

    // Go to next leak.
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Core::Icons::NEXT.icon());
    action->setToolTip(tr("Go to next leak."));
    connect(action, &QAction::triggered, m_errorView, &MemcheckErrorView::goNext);
    button = new QToolButton;
    button->setDefaultAction(action);
    layout->addWidget(button);
    m_goNext = action;

    QToolButton *filterButton = new QToolButton;
    filterButton->setIcon(Core::Icons::FILTER.icon());
    filterButton->setText(tr("Error Filter"));
    filterButton->setPopupMode(QToolButton::InstantPopup);
    filterButton->setProperty("noArrow", true);

    m_filterMenu = new QMenu(filterButton);
    foreach (QAction *filterAction, m_errorFilterActions)
        m_filterMenu->addAction(filterAction);
    m_filterMenu->addSeparator();
    m_filterMenu->addAction(m_filterProjectAction);
    m_filterMenu->addAction(m_suppressionSeparator);
    connect(m_filterMenu, &QMenu::triggered, this, &MemcheckTool::updateErrorFilter);
    filterButton->setMenu(m_filterMenu);
    layout->addWidget(filterButton);

    layout->addStretch();
    QWidget *widget = new QWidget;
    widget->setObjectName(QLatin1String("MemCheckToolBarWidget"));
    widget->setLayout(layout);
    return widget;
}

MemcheckRunControl *MemcheckTool::createRunControl(RunConfiguration *runConfiguration,
                                                   Core::Id runMode)
{
    m_frameFinder->setFiles(runConfiguration ? runConfiguration->target()
        ->project()->files(Project::AllFiles) : QStringList());

    MemcheckRunControl *runControl = 0;
    if (runMode == MEMCHECK_RUN_MODE)
        runControl = new MemcheckRunControl(runConfiguration, runMode);
    else
        runControl = new MemcheckWithGdbRunControl(runConfiguration);
    connect(runControl, &MemcheckRunControl::starting,
            this, [this, runControl]() { engineStarting(runControl); });
    connect(runControl, &MemcheckRunControl::parserError, this, &MemcheckTool::parserError);
    connect(runControl, &MemcheckRunControl::internalParserError, this, &MemcheckTool::internalParserError);
    connect(runControl, &MemcheckRunControl::finished, this, &MemcheckTool::engineFinished);
    return runControl;
}

void MemcheckTool::engineStarting(const MemcheckRunControl *runControl)
{
    setBusyCursor(true);
    clearErrorView();
    m_loadExternalLogFile->setDisabled(true);

    QString dir;
    if (RunConfiguration *rc = runControl->runConfiguration())
        dir = rc->target()->project()->projectDirectory().toString() + QLatin1Char('/');

    const QString name = Utils::FileName::fromString(runControl->executable()).fileName();

    m_errorView->setDefaultSuppressionFile(dir + name + QLatin1String(".supp"));

    foreach (const QString &file, runControl->suppressionFiles()) {
        QAction *action = m_filterMenu->addAction(Utils::FileName::fromString(file).fileName());
        action->setToolTip(file);
        connect(action, &QAction::triggered, this, [this, file]() {
            Core::EditorManager::openEditorAt(file, 0);
        });
        m_suppressionActions.append(action);
    }
}

void MemcheckTool::loadExternalXmlLogFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
                Core::ICore::mainWindow(),
                tr("Open Memcheck XML Log File"),
                QString(),
                tr("XML Files (*.xml);;All Files (*)"));
    if (filePath.isEmpty())
        return;

    QFile *logFile = new QFile(filePath);
    if (!logFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
        delete logFile;
        AnalyzerUtils::logToIssuesPane(Task::Error,
                tr("Memcheck: Failed to open file for reading: %1").arg(filePath));
        return;
    }

    setBusyCursor(true);
    clearErrorView();
    m_loadExternalLogFile->setDisabled(true);

    if (!m_settings || m_settings != ValgrindPlugin::globalSettings()) {
        m_settings = ValgrindPlugin::globalSettings();
        m_errorView->settingsChanged(m_settings);
        updateFromSettings();
    }

    ThreadedParser *parser = new ThreadedParser;
    connect(parser, &ThreadedParser::error, this, &MemcheckTool::parserError);
    connect(parser, &ThreadedParser::internalError, this, &MemcheckTool::internalParserError);
    connect(parser, &ThreadedParser::finished, this, &MemcheckTool::loadingExternalXmlLogFileFinished);
    connect(parser, &ThreadedParser::finished, parser, &ThreadedParser::deleteLater);

    parser->parse(logFile); // ThreadedParser owns the file
}

void MemcheckTool::parserError(const Error &error)
{
    m_errorModel->addError(error);
}

void MemcheckTool::internalParserError(const QString &errorString)
{
    AnalyzerUtils::logToIssuesPane(Task::Error,
            tr("Memcheck: Error occurred parsing Valgrind output: %1").arg(errorString));
}

void MemcheckTool::clearErrorView()
{
    QTC_ASSERT(m_errorView, return);
    m_errorModel->clear();

    qDeleteAll(m_suppressionActions);
    m_suppressionActions.clear();
    //QTC_ASSERT(filterMenu()->actions().last() == m_suppressionSeparator, qt_noop());
}

void MemcheckTool::updateErrorFilter()
{
    QTC_ASSERT(m_errorView, return);
    QTC_ASSERT(m_settings, return);

    m_settings->setFilterExternalIssues(!m_filterProjectAction->isChecked());

    QList<int> errorKinds;
    foreach (QAction *a, m_errorFilterActions) {
        if (!a->isChecked())
            continue;
        foreach (const QVariant &v, a->data().toList()) {
            bool ok;
            int kind = v.toInt(&ok);
            if (ok)
                errorKinds << kind;
        }
    }
    m_settings->setVisibleErrorKinds(errorKinds);
}

int MemcheckTool::updateUiAfterFinishedHelper()
{
    const int issuesFound = m_errorModel->rowCount();
    m_goBack->setEnabled(issuesFound > 1);
    m_goNext->setEnabled(issuesFound > 1);
    m_loadExternalLogFile->setEnabled(true);
    setBusyCursor(false);
    return issuesFound;
}

void MemcheckTool::engineFinished()
{
    const int issuesFound = updateUiAfterFinishedHelper();
    AnalyzerManager::showPermanentStatusMessage(issuesFound > 0
        ? AnalyzerManager::tr("Memory Analyzer Tool finished, %n issues were found.", 0, issuesFound)
        : AnalyzerManager::tr("Memory Analyzer Tool finished, no issues were found."));
}

void MemcheckTool::loadingExternalXmlLogFileFinished()
{
    const int issuesFound = updateUiAfterFinishedHelper();
    AnalyzerManager::showPermanentStatusMessage(issuesFound > 0
        ? AnalyzerManager::tr("Log file processed, %n issues were found.", 0, issuesFound)
        : AnalyzerManager::tr("Log file processed, no issues were found."));
}

void MemcheckTool::setBusyCursor(bool busy)
{
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_errorView->setCursor(cursor);
}

static MemcheckTool *theMemcheckTool;

void initMemcheckTool(QObject *parent)
{
    theMemcheckTool = new MemcheckTool(parent);
}

void destroyMemcheckTool()
{
    delete theMemcheckTool;
    theMemcheckTool = 0;
}

} // namespace Internal
} // namespace Valgrind
