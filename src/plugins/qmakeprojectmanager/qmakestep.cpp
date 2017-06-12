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

#include "qmakestep.h"
#include "ui_qmakestep.h"

#include "makestep.h"
#include "qmakebuildconfiguration.h"
#include "qmakekitinformation.h"
#include "qmakenodes.h"
#include "qmakeparser.h"
#include "qmakeproject.h"
#include "qmakeprojectmanagerconstants.h"

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/variablechooser.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

#include <QDir>
#include <QMessageBox>

using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;
using namespace QtSupport;
using namespace ProjectExplorer;
using namespace Utils;

namespace {
const char QMAKE_BS_ID[] = "QtProjectManager.QMakeBuildStep";

const char QMAKE_ARGUMENTS_KEY[] = "QtProjectManager.QMakeBuildStep.QMakeArguments";
const char QMAKE_FORCED_KEY[] = "QtProjectManager.QMakeBuildStep.QMakeForced";
const char QMAKE_USE_QTQUICKCOMPILER[] = "QtProjectManager.QMakeBuildStep.UseQtQuickCompiler";
const char QMAKE_SEPARATEDEBUGINFO_KEY[] = "QtProjectManager.QMakeBuildStep.SeparateDebugInfo";
const char QMAKE_QMLDEBUGLIBAUTO_KEY[] = "QtProjectManager.QMakeBuildStep.LinkQmlDebuggingLibraryAuto";
const char QMAKE_QMLDEBUGLIB_KEY[] = "QtProjectManager.QMakeBuildStep.LinkQmlDebuggingLibrary";
}

QMakeStep::QMakeStep(BuildStepList *bsl) :
    AbstractProcessStep(bsl, Core::Id(QMAKE_BS_ID))
{
    ctor();
}

QMakeStep::QMakeStep(BuildStepList *bsl, Core::Id id) :
    AbstractProcessStep(bsl, id)
{
    ctor();
}

QMakeStep::QMakeStep(BuildStepList *bsl, QMakeStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_userArgs(bs->m_userArgs),
    m_extraArgs(bs->m_extraArgs),
    m_forced(bs->m_forced),
    m_linkQmlDebuggingLibrary(bs->m_linkQmlDebuggingLibrary),
    m_useQtQuickCompiler(bs->m_useQtQuickCompiler),
    m_separateDebugInfo(bs->m_separateDebugInfo)
{
    ctor();
}

void QMakeStep::ctor()
{
    //: QMakeStep default display name
    setDefaultDisplayName(tr("qmake"));

    connect(&m_inputWatcher, &QFutureWatcher<bool>::canceled,
            this, [this]() {
                if (m_commandFuture)
                    m_commandFuture->cancel();
            });
    connect(&m_commandWatcher, &QFutureWatcher<bool>::finished, this, &QMakeStep::runNextCommand);
}

QmakeBuildConfiguration *QMakeStep::qmakeBuildConfiguration() const
{
    return static_cast<QmakeBuildConfiguration *>(buildConfiguration());
}

///
/// Returns all arguments
/// That is: possbile subpath
/// spec
/// config arguemnts
/// moreArguments
/// user arguments
QString QMakeStep::allArguments(const BaseQtVersion *v, bool shorted) const
{
    QTC_ASSERT(v, return QString());
    QmakeBuildConfiguration *bc = qmakeBuildConfiguration();
    QStringList arguments;
    if (bc->subNodeBuild())
        arguments << bc->subNodeBuild()->filePath().toUserOutput();
    else if (shorted)
        arguments << project()->projectFilePath().fileName();
    else
        arguments << project()->projectFilePath().toUserOutput();

    if (v->qtVersion() < QtVersionNumber(5, 0, 0))
        arguments << "-r";
    bool userProvidedMkspec = false;
    for (QtcProcess::ConstArgIterator ait(m_userArgs); ait.next(); ) {
        if (ait.value() == "-spec") {
            if (ait.next()) {
                userProvidedMkspec = true;
                break;
            }
        }
    }
    FileName specArg = mkspec();
    if (!userProvidedMkspec && !specArg.isEmpty())
        arguments << "-spec" << specArg.toUserOutput();

    // Find out what flags we pass on to qmake
    arguments << bc->configCommandLineArguments();

    arguments << deducedArguments().toArguments();

    QString args = QtcProcess::joinArgs(arguments);
    // User arguments
    QtcProcess::addArgs(&args, m_userArgs);
    foreach (QString arg, m_extraArgs)
        QtcProcess::addArgs(&args, arg);
    return args;
}

QMakeStepConfig QMakeStep::deducedArguments() const
{
    ProjectExplorer::Kit *kit = target()->kit();
    QMakeStepConfig config;
    ProjectExplorer::ToolChain *tc
            = ProjectExplorer::ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    ProjectExplorer::Abi targetAbi;
    if (tc)
        targetAbi = tc->targetAbi();

    BaseQtVersion *version = QtKitInformation::qtVersion(target()->kit());

    config.archConfig = QMakeStepConfig::targetArchFor(targetAbi, version);
    config.osType = QMakeStepConfig::osTypeFor(targetAbi, version);
    if (linkQmlDebuggingLibrary() && version && version->qtVersion().majorVersion >= 5)
        config.linkQmlDebuggingQQ2 = true;

    if (useQtQuickCompiler() && version)
        config.useQtQuickCompiler = true;

    if (separateDebugInfo())
        config.separateDebugInfo = true;

    return config;
}


bool QMakeStep::init(QList<const BuildStep *> &earlierSteps)
{
    if (m_commandFuture)
        return false;

    QmakeBuildConfiguration *qmakeBc = qmakeBuildConfiguration();
    const BaseQtVersion *qtVersion = QtKitInformation::qtVersion(target()->kit());

    if (!qtVersion) {
        emit addOutput(tr("No Qt version configured."), BuildStep::OutputFormat::ErrorMessage);
        return false;
    }

    QString workingDirectory;

    if (qmakeBc->subNodeBuild())
        workingDirectory = qmakeBc->subNodeBuild()->buildDir();
    else
        workingDirectory = qmakeBc->buildDirectory().toString();

    m_qmakeExecutable = qtVersion->qmakeCommand().toString();
    m_qmakeArguments = allArguments(qtVersion);
    m_runMakeQmake = (qtVersion->qtVersion() >= QtVersionNumber(5, 0 ,0));
    if (m_runMakeQmake) {
        m_makeExecutable = makeCommand();
        if (m_makeExecutable.isEmpty()) {
            emit addOutput(tr("Could not determine which \"make\" command to run. "
                              "Check the \"make\" step in the build configuration."),
                           BuildStep::OutputFormat::ErrorMessage);
            return false;
        }
        m_makeArguments = makeArguments();
    } else {
        m_makeExecutable.clear();
        m_makeArguments.clear();
    }

    QString makefile = workingDirectory + '/';

    if (qmakeBc->subNodeBuild()) {
        QmakeProFile *pro = qmakeBc->subNodeBuild()->proFile();
        if (pro && !pro->makefile().isEmpty())
            makefile.append(pro->makefile());
        else
            makefile.append("Makefile");
    } else if (!qmakeBc->makefile().isEmpty()) {
        makefile.append(qmakeBc->makefile());
    } else {
        makefile.append("Makefile");
    }

    // Check whether we need to run qmake
    bool makefileOutDated = (qmakeBc->compareToImportFrom(makefile) != QmakeBuildConfiguration::MakefileMatches);
    if (m_forced || makefileOutDated)
        m_needToRunQMake = true;
    m_forced = false;

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(qmakeBc->macroExpander());
    pp->setWorkingDirectory(workingDirectory);
    pp->setEnvironment(qmakeBc->environment());

    setOutputParser(new QMakeParser);

    QmakeProFileNode *node = static_cast<QmakeProject *>(qmakeBc->target()->project())->rootProjectNode();
    if (qmakeBc->subNodeBuild())
        node = qmakeBc->subNodeBuild();
    QTC_ASSERT(node, return false);
    QString proFile = node->filePath().toString();

    QList<ProjectExplorer::Task> tasks = qtVersion->reportIssues(proFile, workingDirectory);
    Utils::sort(tasks);

    if (!tasks.isEmpty()) {
        bool canContinue = true;
        foreach (const ProjectExplorer::Task &t, tasks) {
            addTask(t);
            if (t.type == Task::Error)
                canContinue = false;
        }
        if (!canContinue) {
            emitFaultyConfigurationMessage();
            return false;
        }
    }

    m_scriptTemplate = node->projectType() == ProjectType::ScriptTemplate;

    return AbstractProcessStep::init(earlierSteps);
}

void QMakeStep::run(QFutureInterface<bool> &fi)
{
    m_inputFuture = fi;
    m_inputWatcher.setFuture(m_inputFuture.future());

    fi.setProgressRange(0, static_cast<int>(State::POST_PROCESS));
    fi.setProgressValue(0);
    if (m_scriptTemplate) {
        reportRunResult(fi, true);
        return;
    }

    if (!m_needToRunQMake) {
        emit addOutput(tr("Configuration unchanged, skipping qmake step."), BuildStep::OutputFormat::NormalMessage);
        reportRunResult(fi, true);
        return;
    }

    m_needToRunQMake = false;

    m_nextState = State::RUN_QMAKE;
    runNextCommand();
}

void QMakeStep::setForced(bool b)
{
    m_forced = b;
}

bool QMakeStep::forced()
{
    return m_forced;
}

ProjectExplorer::BuildStepConfigWidget *QMakeStep::createConfigWidget()
{
    return new QMakeStepConfigWidget(this);
}

bool QMakeStep::immutable() const
{
    return false;
}

void QMakeStep::processStartupFailed()
{
    m_needToRunQMake = true;
    AbstractProcessStep::processStartupFailed();
}

bool QMakeStep::processSucceeded(int exitCode, QProcess::ExitStatus status)
{
    bool result = AbstractProcessStep::processSucceeded(exitCode, status);
    if (!result)
        m_needToRunQMake = true;
    QmakeProject *project = static_cast<QmakeProject *>(qmakeBuildConfiguration()->target()->project());
    project->emitBuildDirectoryInitialized();
    return result;
}

void QMakeStep::startOneCommand(const QString &command, const QString &args)
{
    ProcessParameters *pp = processParameters();
    pp->setCommand(command);
    pp->setArguments(args);
    pp->resolveAll();

    QTC_ASSERT(!m_commandFuture || m_commandFuture->future().isFinished(), return);
    m_commandFuture.reset(new QFutureInterface<bool>);

    m_commandWatcher.setFuture(m_commandFuture->future());
    AbstractProcessStep::run(*m_commandFuture);
}

void QMakeStep::runNextCommand()
{
    bool wasSuccess = true;
    if (m_commandFuture) {
        if (m_commandFuture->isCanceled())
            wasSuccess = false;
        else if (m_commandFuture->isFinished())
            wasSuccess = m_commandFuture->future().result();
        else
            wasSuccess = false; // should not happen
    }

    m_commandFuture.reset();

    if (!wasSuccess)
        m_nextState = State::POST_PROCESS;

    m_inputFuture.setProgressValue(static_cast<int>(m_nextState));

    switch (m_nextState) {
    case State::IDLE:
        return;
    case State::RUN_QMAKE:
        setOutputParser(new QMakeParser);
        m_nextState = (m_runMakeQmake ? State::RUN_MAKE_QMAKE_ALL : State::POST_PROCESS);
        startOneCommand(m_qmakeExecutable, m_qmakeArguments);
        return;
    case State::RUN_MAKE_QMAKE_ALL:
        {
            GnuMakeParser *parser = new GnuMakeParser;
            parser->setWorkingDirectory(processParameters()->workingDirectory());
            setOutputParser(parser);
            m_nextState = State::POST_PROCESS;
            startOneCommand(m_makeExecutable, m_makeArguments);
        }
        return;
    case State::POST_PROCESS:
        m_nextState = State::IDLE;
        reportRunResult(m_inputFuture, wasSuccess);
        m_inputFuture = QFutureInterface<bool>();
        return;
    }
}

void QMakeStep::setUserArguments(const QString &arguments)
{
    if (m_userArgs == arguments)
        return;
    m_userArgs = arguments;

    emit userArgumentsChanged();

    qmakeBuildConfiguration()->emitQMakeBuildConfigurationChanged();
    qmakeBuildConfiguration()->emitProFileEvaluateNeeded();
}

QStringList QMakeStep::extraArguments() const
{
    return m_extraArgs;
}

void QMakeStep::setExtraArguments(const QStringList &args)
{
    if (m_extraArgs != args) {
        m_extraArgs = args;
        emit extraArgumentsChanged();
        qmakeBuildConfiguration()->emitQMakeBuildConfigurationChanged();
        qmakeBuildConfiguration()->emitProFileEvaluateNeeded();
    }
}

bool QMakeStep::linkQmlDebuggingLibrary() const
{
    return m_linkQmlDebuggingLibrary;
}

void QMakeStep::setLinkQmlDebuggingLibrary(bool enable)
{
    if (enable == m_linkQmlDebuggingLibrary)
        return;

    m_linkQmlDebuggingLibrary = enable;

    emit linkQmlDebuggingLibraryChanged();

    qmakeBuildConfiguration()->emitQMakeBuildConfigurationChanged();
    qmakeBuildConfiguration()->emitProFileEvaluateNeeded();
}

bool QMakeStep::useQtQuickCompiler() const
{
    return m_useQtQuickCompiler;
}

void QMakeStep::setUseQtQuickCompiler(bool enable)
{
    if (enable == m_useQtQuickCompiler)
        return;

    m_useQtQuickCompiler = enable;

    emit useQtQuickCompilerChanged();

    qmakeBuildConfiguration()->emitQMakeBuildConfigurationChanged();
    qmakeBuildConfiguration()->emitProFileEvaluateNeeded();
}

bool QMakeStep::separateDebugInfo() const
{
    return m_separateDebugInfo;
}

void QMakeStep::setSeparateDebugInfo(bool enable)
{
    if (enable == m_separateDebugInfo)
        return;
    m_separateDebugInfo = enable;

    emit separateDebugInfoChanged();

    qmakeBuildConfiguration()->emitQMakeBuildConfigurationChanged();
    qmakeBuildConfiguration()->emitProFileEvaluateNeeded();
}

QString QMakeStep::makeCommand() const
{
    MakeStep *ms = qobject_cast<BuildStepList *>(parent())->firstOfType<MakeStep>();
    return ms ? ms->effectiveMakeCommand() : QString();
}

QString QMakeStep::makeArguments() const
{
    QString args;
    if (QmakeBuildConfiguration *qmakeBc = qmakeBuildConfiguration()) {
        const QString makefile = qmakeBc->makefile();
        if (!makefile.isEmpty()) {
            Utils::QtcProcess::addArg(&args, "-f");
            Utils::QtcProcess::addArg(&args, makefile);
        }
    }
    Utils::QtcProcess::addArg(&args, "qmake_all");
    return args;
}

QString QMakeStep::effectiveQMakeCall() const
{
    BaseQtVersion *qtVersion = QtKitInformation::qtVersion(target()->kit());
    QString qmake = qtVersion ? qtVersion->qmakeCommand().fileName() : QString();
    if (qmake.isEmpty())
        qmake = tr("<no Qt version>");
    QString make = makeCommand();
    if (make.isEmpty())
        make = tr("<no Make step found>");

    QString result = qmake;
    if (qtVersion) {
        result += ' ' + buildConfiguration()->macroExpander()->expand(allArguments(qtVersion));
        if (qtVersion->qtVersion() >= QtVersionNumber(5, 0, 0))
            result.append(QString::fromLatin1(" && %1 %2").arg(make).arg(makeArguments()));
    }
    return result;
}

QStringList QMakeStep::parserArguments()
{
    QStringList result;
    BaseQtVersion *qt = QtKitInformation::qtVersion(target()->kit());
    QTC_ASSERT(qt, return QStringList());
    for (QtcProcess::ConstArgIterator ait(allArguments(qt)); ait.next(); ) {
        if (ait.isSimple())
            result << ait.value();
    }
    return result;
}

QString QMakeStep::userArguments()
{
    return m_userArgs;
}

FileName QMakeStep::mkspec() const
{
    QString additionalArguments = m_userArgs;
    QtcProcess::addArgs(&additionalArguments, m_extraArgs);
    for (QtcProcess::ArgIterator ait(&additionalArguments); ait.next(); ) {
        if (ait.value() == "-spec") {
            if (ait.next())
                return FileName::fromUserInput(ait.value());
        }
    }

    return QmakeProjectManager::QmakeKitInformation::effectiveMkspec(target()->kit());
}

QVariantMap QMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(QMAKE_ARGUMENTS_KEY, m_userArgs);
    map.insert(QMAKE_QMLDEBUGLIB_KEY, m_linkQmlDebuggingLibrary);
    map.insert(QMAKE_FORCED_KEY, m_forced);
    map.insert(QMAKE_USE_QTQUICKCOMPILER, m_useQtQuickCompiler);
    map.insert(QMAKE_SEPARATEDEBUGINFO_KEY, m_separateDebugInfo);
    return map;
}

bool QMakeStep::fromMap(const QVariantMap &map)
{
    m_userArgs = map.value(QMAKE_ARGUMENTS_KEY).toString();
    m_forced = map.value(QMAKE_FORCED_KEY, false).toBool();
    m_useQtQuickCompiler = map.value(QMAKE_USE_QTQUICKCOMPILER, false).toBool();

    // QMAKE_QMLDEBUGLIBAUTO_KEY was used in versions 2.3 to 3.5 (both included) to automatically
    // change the qml_debug CONFIG flag based no the qmake build configuration.
    if (map.value(QMAKE_QMLDEBUGLIBAUTO_KEY, false).toBool()) {
        m_linkQmlDebuggingLibrary =
                project()->projectLanguages().contains(
                    ProjectExplorer::Constants::QMLJS_LANGUAGE_ID) &&
                (qmakeBuildConfiguration()->qmakeBuildConfiguration() & BaseQtVersion::DebugBuild);
    } else {
        m_linkQmlDebuggingLibrary = map.value(QMAKE_QMLDEBUGLIB_KEY, false).toBool();
    }
    m_separateDebugInfo = map.value(QMAKE_SEPARATEDEBUGINFO_KEY, false).toBool();

    return BuildStep::fromMap(map);
}

////
// QMakeStepConfigWidget
////

QMakeStepConfigWidget::QMakeStepConfigWidget(QMakeStep *step)
    : BuildStepConfigWidget(), m_ui(new Internal::Ui::QMakeStep), m_step(step)
{
    m_ui->setupUi(this);

    m_ui->qmakeAdditonalArgumentsLineEdit->setText(m_step->userArguments());
    m_ui->qmlDebuggingLibraryCheckBox->setChecked(m_step->linkQmlDebuggingLibrary());
    m_ui->qtQuickCompilerCheckBox->setChecked(m_step->useQtQuickCompiler());
    m_ui->separateDebugInfoCheckBox->setChecked(m_step->separateDebugInfo());
    const QPixmap warning = Utils::Icons::WARNING.pixmap();
    m_ui->qmlDebuggingWarningIcon->setPixmap(warning);
    m_ui->qtQuickCompilerWarningIcon->setPixmap(warning);

    qmakeBuildConfigChanged();

    updateSummaryLabel();
    updateEffectiveQMakeCall();
    updateQmlDebuggingOption();
    updateQtQuickCompilerOption();

    connect(m_ui->qmakeAdditonalArgumentsLineEdit, &QLineEdit::textEdited,
            this, &QMakeStepConfigWidget::qmakeArgumentsLineEdited);
    connect(m_ui->buildConfigurationComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &QMakeStepConfigWidget::buildConfigurationSelected);
    connect(m_ui->qmlDebuggingLibraryCheckBox, &QCheckBox::toggled,
            this, &QMakeStepConfigWidget::linkQmlDebuggingLibraryChecked);
    connect(m_ui->qmlDebuggingLibraryCheckBox, &QCheckBox::clicked,
            this, &QMakeStepConfigWidget::askForRebuild);
    connect(m_ui->qtQuickCompilerCheckBox, &QAbstractButton::toggled,
            this, &QMakeStepConfigWidget::useQtQuickCompilerChecked);
    connect(m_ui->qtQuickCompilerCheckBox, &QCheckBox::clicked,
            this, &QMakeStepConfigWidget::askForRebuild);
    connect(m_ui->separateDebugInfoCheckBox, &QAbstractButton::toggled,
            this, &QMakeStepConfigWidget::separateDebugInfoChecked);
    connect(m_ui->separateDebugInfoCheckBox, &QCheckBox::clicked,
            this, &QMakeStepConfigWidget::askForRebuild);
    connect(step, &QMakeStep::userArgumentsChanged,
            this, &QMakeStepConfigWidget::userArgumentsChanged);
    connect(step, &QMakeStep::linkQmlDebuggingLibraryChanged,
            this, &QMakeStepConfigWidget::linkQmlDebuggingLibraryChanged);
    connect(step->project(), &Project::projectLanguagesUpdated,
            this, &QMakeStepConfigWidget::linkQmlDebuggingLibraryChanged);
    connect(step, &QMakeStep::useQtQuickCompilerChanged,
            this, &QMakeStepConfigWidget::useQtQuickCompilerChanged);
    connect(step, &QMakeStep::separateDebugInfoChanged,
            this, &QMakeStepConfigWidget::separateDebugInfoChanged);
    connect(step->qmakeBuildConfiguration(), &QmakeBuildConfiguration::qmakeBuildConfigurationChanged,
            this, &QMakeStepConfigWidget::qmakeBuildConfigChanged);
    connect(step->target(), &Target::kitChanged, this, &QMakeStepConfigWidget::qtVersionChanged);
    connect(QtVersionManager::instance(), &QtVersionManager::dumpUpdatedFor,
            this, &QMakeStepConfigWidget::qtVersionChanged);
    auto chooser = new Core::VariableChooser(m_ui->qmakeAdditonalArgumentsLineEdit);
    chooser->addMacroExpanderProvider([step] { return step->macroExpander(); });
    chooser->addSupportedWidget(m_ui->qmakeAdditonalArgumentsLineEdit);
}

QMakeStepConfigWidget::~QMakeStepConfigWidget()
{
    delete m_ui;
}

QString QMakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

QString QMakeStepConfigWidget::additionalSummaryText() const
{
    return m_additionalSummaryText;
}

QString QMakeStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

void QMakeStepConfigWidget::qtVersionChanged()
{
    updateSummaryLabel();
    updateEffectiveQMakeCall();
    updateQmlDebuggingOption();
    updateQtQuickCompilerOption();
}

void QMakeStepConfigWidget::qmakeBuildConfigChanged()
{
    QmakeBuildConfiguration *bc = m_step->qmakeBuildConfiguration();
    bool debug = bc->qmakeBuildConfiguration() & BaseQtVersion::DebugBuild;
    m_ignoreChange = true;
    m_ui->buildConfigurationComboBox->setCurrentIndex(debug? 0 : 1);
    m_ignoreChange = false;
    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::userArgumentsChanged()
{
    if (m_ignoreChange)
        return;
    m_ui->qmakeAdditonalArgumentsLineEdit->setText(m_step->userArguments());
    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::linkQmlDebuggingLibraryChanged()
{
    if (m_ignoreChange)
        return;
    m_ui->qmlDebuggingLibraryCheckBox->setChecked(m_step->linkQmlDebuggingLibrary());

    updateSummaryLabel();
    updateEffectiveQMakeCall();
    updateQmlDebuggingOption();
}

void QMakeStepConfigWidget::useQtQuickCompilerChanged()
{
    if (m_ignoreChange)
        return;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
    updateQtQuickCompilerOption();
    updateQmlDebuggingOption();
}

void QMakeStepConfigWidget::separateDebugInfoChanged()
{
    if (m_ignoreChange)
        return;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::qmakeArgumentsLineEdited()
{
    m_ignoreChange = true;
    m_step->setUserArguments(m_ui->qmakeAdditonalArgumentsLineEdit->text());
    m_ignoreChange = false;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::buildConfigurationSelected()
{
    if (m_ignoreChange)
        return;
    QmakeBuildConfiguration *bc = m_step->qmakeBuildConfiguration();
    BaseQtVersion::QmakeBuildConfigs buildConfiguration = bc->qmakeBuildConfiguration();
    if (m_ui->buildConfigurationComboBox->currentIndex() == 0) { // debug
        buildConfiguration = buildConfiguration | BaseQtVersion::DebugBuild;
    } else {
        buildConfiguration = buildConfiguration & ~BaseQtVersion::DebugBuild;
    }
    m_ignoreChange = true;
    bc->setQMakeBuildConfiguration(buildConfiguration);
    m_ignoreChange = false;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::linkQmlDebuggingLibraryChecked(bool checked)
{
    if (m_ignoreChange)
        return;

    m_ignoreChange = true;
    m_step->setLinkQmlDebuggingLibrary(checked);
    m_ignoreChange = false;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
    updateQmlDebuggingOption();
}

void QMakeStepConfigWidget::askForRebuild()
{
    QMessageBox *question = new QMessageBox(Core::ICore::mainWindow());
    question->setWindowTitle(tr("QML Debugging"));
    question->setText(tr("The option will only take effect if the project is recompiled. Do you want to recompile now?"));
    question->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    question->setModal(true);
    connect(question, &QDialog::finished, this, &QMakeStepConfigWidget::recompileMessageBoxFinished);
    question->show();
}

void QMakeStepConfigWidget::useQtQuickCompilerChecked(bool checked)
{
    if (m_ignoreChange)
        return;

    m_ignoreChange = true;
    m_step->setUseQtQuickCompiler(checked);
    m_ignoreChange = false;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
    updateQmlDebuggingOption();
    updateQtQuickCompilerOption();
}

void QMakeStepConfigWidget::separateDebugInfoChecked(bool checked)
{
    if (m_ignoreChange)
        return;

    m_ignoreChange = true;
    m_step->setSeparateDebugInfo(checked);
    m_ignoreChange = false;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::updateSummaryLabel()
{
    BaseQtVersion *qtVersion = QtKitInformation::qtVersion(m_step->target()->kit());
    if (!qtVersion) {
        setSummaryText(tr("<b>qmake:</b> No Qt version set. Cannot run qmake."));
        return;
    }
    // We don't want the full path to the .pro file
    QString args = m_step->allArguments(qtVersion, true);
    // And we only use the .pro filename not the full path
    QString program = qtVersion->qmakeCommand().fileName();
    setSummaryText(tr("<b>qmake:</b> %1 %2").arg(program, args));
}

void QMakeStepConfigWidget::updateQmlDebuggingOption()
{
    QString warningText;
    bool supported = BaseQtVersion::isQmlDebuggingSupported(m_step->target()->kit(),
                                                                       &warningText);

    m_ui->qmlDebuggingLibraryCheckBox->setEnabled(supported);
    m_ui->debuggingLibraryLabel->setText(tr("Enable QML debugging and profiling:"));

    if (supported && m_step->linkQmlDebuggingLibrary())
        warningText = tr("Might make your application vulnerable. Only use in a safe environment.");

    m_ui->qmlDebuggingWarningText->setText(warningText);
    m_ui->qmlDebuggingWarningIcon->setVisible(!warningText.isEmpty());

    updateQtQuickCompilerOption(); // show or clear compiler warning text
}

void QMakeStepConfigWidget::updateQtQuickCompilerOption()
{
    QString warningText;
    bool supported = BaseQtVersion::isQtQuickCompilerSupported(m_step->target()->kit(),
                                                                          &warningText);
    m_ui->qtQuickCompilerCheckBox->setEnabled(supported);
    m_ui->qtQuickCompilerLabel->setText(tr("Enable Qt Quick Compiler:"));

    if (supported && m_step->useQtQuickCompiler() && m_step->linkQmlDebuggingLibrary())
        warningText = tr("Disables QML debugging. QML profiling will still work.");

    m_ui->qtQuickCompilerWarningText->setText(warningText);
    m_ui->qtQuickCompilerWarningIcon->setVisible(!warningText.isEmpty());
}

void QMakeStepConfigWidget::updateEffectiveQMakeCall()
{
    m_ui->qmakeArgumentsEdit->setPlainText(m_step->effectiveQMakeCall());
}

void QMakeStepConfigWidget::recompileMessageBoxFinished(int button)
{
    if (button == QMessageBox::Yes) {
        QmakeBuildConfiguration *bc = m_step->qmakeBuildConfiguration();
        if (!bc)
            return;

        QList<ProjectExplorer::BuildStepList *> stepLists;
        const Core::Id clean = ProjectExplorer::Constants::BUILDSTEPS_CLEAN;
        const Core::Id build = ProjectExplorer::Constants::BUILDSTEPS_BUILD;
        stepLists << bc->stepList(clean) << bc->stepList(build);
        BuildManager::buildLists(stepLists, QStringList() << ProjectExplorerPlugin::displayNameForStepId(clean)
                       << ProjectExplorerPlugin::displayNameForStepId(build));
    }
}

void QMakeStepConfigWidget::setSummaryText(const QString &text)
{
    if (text == m_summaryText)
        return;
    m_summaryText = text;
    emit updateSummary();
}

////
// QMakeStepFactory
////

QMakeStepFactory::QMakeStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

QList<BuildStepInfo> QMakeStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return {};
    if (!qobject_cast<QmakeBuildConfiguration *>(parent->parent()))
        return {};

    return {{QMAKE_BS_ID, tr("qmake"), BuildStepInfo::UniqueStep}};
}

ProjectExplorer::BuildStep *QMakeStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(id)
    return new QMakeStep(parent);
}

ProjectExplorer::BuildStep *QMakeStepFactory::clone(BuildStepList *parent, ProjectExplorer::BuildStep *source)
{
    return new QMakeStep(parent, qobject_cast<QMakeStep *>(source));
}

QMakeStepConfig::TargetArchConfig QMakeStepConfig::targetArchFor(const Abi &targetAbi, const BaseQtVersion *version)
{
    QMakeStepConfig::TargetArchConfig arch = QMakeStepConfig::NoArch;
    if (!version || version->type() != QtSupport::Constants::DESKTOPQT)
        return arch;
    if ((targetAbi.os() == ProjectExplorer::Abi::DarwinOS)
            && (targetAbi.binaryFormat() == ProjectExplorer::Abi::MachOFormat)) {
        if (targetAbi.architecture() == ProjectExplorer::Abi::X86Architecture) {
            if (targetAbi.wordWidth() == 32)
                arch = QMakeStepConfig::X86;
            else if (targetAbi.wordWidth() == 64)
                arch = QMakeStepConfig::X86_64;
        } else if (targetAbi.architecture() == ProjectExplorer::Abi::PowerPCArchitecture) {
            if (targetAbi.wordWidth() == 32)
                arch = QMakeStepConfig::PowerPC;
            else if (targetAbi.wordWidth() == 64)
                arch = QMakeStepConfig::PowerPC64;
        }
    }
    return arch;
}

QMakeStepConfig::OsType QMakeStepConfig::osTypeFor(const ProjectExplorer::Abi &targetAbi, const BaseQtVersion *version)
{
    QMakeStepConfig::OsType os = QMakeStepConfig::NoOsType;
    const char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios";
    if (!version || version->type() != IOSQT)
        return os;
    if ((targetAbi.os() == ProjectExplorer::Abi::DarwinOS)
            && (targetAbi.binaryFormat() == ProjectExplorer::Abi::MachOFormat)) {
        if (targetAbi.architecture() == ProjectExplorer::Abi::X86Architecture) {
            os = QMakeStepConfig::IphoneSimulator;
        } else if (targetAbi.architecture() == ProjectExplorer::Abi::ArmArchitecture) {
            os = QMakeStepConfig::IphoneOS;
        }
    }
    return os;
}

QStringList QMakeStepConfig::toArguments() const
{
    QStringList arguments;
    if (archConfig == X86)
        arguments << "CONFIG+=x86";
    else if (archConfig == X86_64)
        arguments << "CONFIG+=x86_64";
    else if (archConfig == PowerPC)
        arguments << "CONFIG+=ppc";
    else if (archConfig == PowerPC64)
        arguments << "CONFIG+=ppc64";

    // TODO: make that depend on the actual Qt version that is used
    if (osType == IphoneSimulator)
        arguments << "CONFIG+=iphonesimulator" << "CONFIG+=simulator" /*since Qt 5.7*/;
    else if (osType == IphoneOS)
        arguments << "CONFIG+=iphoneos" << "CONFIG+=device" /*since Qt 5.7*/;

    if (linkQmlDebuggingQQ2)
        arguments << "CONFIG+=qml_debug";

    if (useQtQuickCompiler)
        arguments << "CONFIG+=qtquickcompiler";

    if (separateDebugInfo)
        arguments << "CONFIG+=force_debug_info" << "CONFIG+=separate_debug_info";

    return arguments;
}
