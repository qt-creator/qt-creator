/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmakestep.h"
#include "ui_qmakestep.h"

#include "qmakeparser.h"
#include "qmakebuildconfiguration.h"
#include "qmakeproject.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakekitinformation.h"
#include "qmakenodes.h"

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <qtsupport/debugginghelperbuildtask.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

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
    AbstractProcessStep(bsl, Core::Id(QMAKE_BS_ID)),
    m_forced(false),
    m_needToRunQMake(false),
    m_linkQmlDebuggingLibrary(DebugLink),
    m_useQtQuickCompiler(false),
    m_separateDebugInfo(false)
{
    ctor();
}

QMakeStep::QMakeStep(BuildStepList *bsl, Core::Id id) :
    AbstractProcessStep(bsl, id),
    m_forced(false),
    m_linkQmlDebuggingLibrary(DebugLink),
    m_useQtQuickCompiler(false),
    m_separateDebugInfo(false)
{
    ctor();
}

QMakeStep::QMakeStep(BuildStepList *bsl, QMakeStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_forced(bs->m_forced),
    m_userArgs(bs->m_userArgs),
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
}

QMakeStep::~QMakeStep()
{
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
QString QMakeStep::allArguments(bool shorted)
{
    QmakeBuildConfiguration *bc = qmakeBuildConfiguration();
    QStringList arguments;
    if (bc->subNodeBuild())
        arguments << bc->subNodeBuild()->path().toUserOutput();
    else if (shorted)
        arguments << project()->projectFilePath().fileName();
    else
        arguments << project()->projectFilePath().toUserOutput();

    arguments << QLatin1String("-r");
    bool userProvidedMkspec = false;
    for (QtcProcess::ConstArgIterator ait(m_userArgs); ait.next(); ) {
        if (ait.value() == QLatin1String("-spec")) {
            if (ait.next()) {
                userProvidedMkspec = true;
                break;
            }
        }
    }
    FileName specArg = mkspec();
    if (!userProvidedMkspec && !specArg.isEmpty())
        arguments << QLatin1String("-spec") << specArg.toUserOutput();

    // Find out what flags we pass on to qmake
    arguments << bc->configCommandLineArguments();

    arguments << deducedArguments().toArguments();

    QString args = QtcProcess::joinArgs(arguments);
    // User arguments
    QtcProcess::addArgs(&args, m_userArgs);
    return args;
}

QMakeStepConfig QMakeStep::deducedArguments()
{
    ProjectExplorer::Kit *kit = target()->kit();
    QMakeStepConfig config;
    ProjectExplorer::ToolChain *tc
            = ProjectExplorer::ToolChainKitInformation::toolChain(kit);
    ProjectExplorer::Abi targetAbi;
    if (tc)
        targetAbi = tc->targetAbi();

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());

    config.archConfig = QMakeStepConfig::targetArchFor(targetAbi, version);
    config.osType = QMakeStepConfig::osTypeFor(targetAbi, version);
    if (linkQmlDebuggingLibrary() && version) {
        config.linkQmlDebuggingQQ1 = true;
        if (version->qtVersion().majorVersion >= 5)
            config.linkQmlDebuggingQQ2 = true;
    }

    if (useQtQuickCompiler() && version)
        config.useQtQuickCompiler = true;

    if (separateDebugInfo())
        config.separateDebugInfo = true;

    return config;
}


bool QMakeStep::init()
{
    QmakeBuildConfiguration *qt4bc = qmakeBuildConfiguration();
    const QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(target()->kit());

    if (!qtVersion)
        return false;

    QString args = allArguments();
    QString workingDirectory;

    if (qt4bc->subNodeBuild())
        workingDirectory = qt4bc->subNodeBuild()->buildDir();
    else
        workingDirectory = qt4bc->buildDirectory().toString();

    FileName program = qtVersion->qmakeCommand();

    QString makefile = workingDirectory;

    if (qt4bc->subNodeBuild()) {
        if (!qt4bc->subNodeBuild()->makefile().isEmpty())
            makefile.append(qt4bc->subNodeBuild()->makefile());
        else
            makefile.append(QLatin1String("/Makefile"));
    } else if (!qt4bc->makefile().isEmpty()) {
        makefile.append(QLatin1Char('/'));
        makefile.append(qt4bc->makefile());
    } else {
        makefile.append(QLatin1String("/Makefile"));
    }

    // Check whether we need to run qmake
    bool makefileOutDated = (qt4bc->compareToImportFrom(makefile) != QmakeBuildConfiguration::MakefileMatches);
    if (m_forced || makefileOutDated)
        m_needToRunQMake = true;
    m_forced = false;

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(qt4bc->macroExpander());
    pp->setWorkingDirectory(workingDirectory);
    pp->setCommand(program.toString());
    pp->setArguments(args);
    pp->setEnvironment(qt4bc->environment());
    pp->resolveAll();

    setOutputParser(new QMakeParser);

    QmakeProFileNode *node = static_cast<QmakeProject *>(qt4bc->target()->project())->rootQmakeProjectNode();
    if (qt4bc->subNodeBuild())
        node = qt4bc->subNodeBuild();
    QString proFile = node->path().toString();

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

    m_scriptTemplate = node->projectType() == ScriptTemplate;

    return AbstractProcessStep::init();
}

void QMakeStep::run(QFutureInterface<bool> &fi)
{
    if (m_scriptTemplate) {
        fi.reportResult(true);
        return;
    }

    if (!m_needToRunQMake) {
        emit addOutput(tr("Configuration unchanged, skipping qmake step."), BuildStep::MessageOutput);
        fi.reportResult(true);
        emit finished();
        return;
    }

    m_needToRunQMake = false;
    AbstractProcessStep::run(fi);
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

void QMakeStep::setUserArguments(const QString &arguments)
{
    if (m_userArgs == arguments)
        return;
    m_userArgs = arguments;

    emit userArgumentsChanged();

    qmakeBuildConfiguration()->emitQMakeBuildConfigurationChanged();
    qmakeBuildConfiguration()->emitProFileEvaluateNeeded();
}

bool QMakeStep::linkQmlDebuggingLibrary() const
{
    if (m_linkQmlDebuggingLibrary == DoLink)
        return true;
    if (m_linkQmlDebuggingLibrary == DoNotLink)
        return false;

    const Core::Context languages = project()->projectLanguages();
    if (!languages.contains(ProjectExplorer::Constants::LANG_QMLJS))
        return false;
    return (qmakeBuildConfiguration()->buildType() & BuildConfiguration::Debug);
}

void QMakeStep::setLinkQmlDebuggingLibrary(bool enable)
{
    if ((enable && (m_linkQmlDebuggingLibrary == DoLink))
            || (!enable && (m_linkQmlDebuggingLibrary == DoNotLink)))
        return;
    m_linkQmlDebuggingLibrary = enable ? DoLink : DoNotLink;

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

QStringList QMakeStep::parserArguments()
{
    QStringList result;
    for (QtcProcess::ConstArgIterator ait(allArguments()); ait.next(); )
        if (ait.isSimple())
            result << ait.value();
    return result;
}

QString QMakeStep::userArguments()
{
    return m_userArgs;
}

FileName QMakeStep::mkspec()
{
    QString additionalArguments = m_userArgs;
    for (QtcProcess::ArgIterator ait(&additionalArguments); ait.next(); ) {
        if (ait.value() == QLatin1String("-spec")) {
            if (ait.next())
                return FileName::fromUserInput(ait.value());
        }
    }

    return QmakeProjectManager::QmakeKitInformation::effectiveMkspec(target()->kit());
}

QVariantMap QMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(QLatin1String(QMAKE_ARGUMENTS_KEY), m_userArgs);
    map.insert(QLatin1String(QMAKE_QMLDEBUGLIBAUTO_KEY), m_linkQmlDebuggingLibrary == DebugLink);
    map.insert(QLatin1String(QMAKE_QMLDEBUGLIB_KEY), m_linkQmlDebuggingLibrary == DoLink);
    map.insert(QLatin1String(QMAKE_FORCED_KEY), m_forced);
    map.insert(QLatin1String(QMAKE_USE_QTQUICKCOMPILER), m_useQtQuickCompiler);
    map.insert(QLatin1String(QMAKE_SEPARATEDEBUGINFO_KEY), m_separateDebugInfo);
    return map;
}

bool QMakeStep::fromMap(const QVariantMap &map)
{
    m_userArgs = map.value(QLatin1String(QMAKE_ARGUMENTS_KEY)).toString();
    m_forced = map.value(QLatin1String(QMAKE_FORCED_KEY), false).toBool();
    m_useQtQuickCompiler = map.value(QLatin1String(QMAKE_USE_QTQUICKCOMPILER), false).toBool();
    if (map.value(QLatin1String(QMAKE_QMLDEBUGLIBAUTO_KEY), false).toBool()) {
        m_linkQmlDebuggingLibrary = DebugLink;
    } else {
        if (map.value(QLatin1String(QMAKE_QMLDEBUGLIB_KEY), false).toBool())
            m_linkQmlDebuggingLibrary = DoLink;
        else
            m_linkQmlDebuggingLibrary = DoNotLink;
    }
    m_separateDebugInfo = map.value(QLatin1String(QMAKE_SEPARATEDEBUGINFO_KEY), false).toBool();

    return BuildStep::fromMap(map);
}

////
// QMakeStepConfigWidget
////

QMakeStepConfigWidget::QMakeStepConfigWidget(QMakeStep *step)
    : BuildStepConfigWidget(), m_ui(new Internal::Ui::QMakeStep), m_step(step),
      m_ignoreChange(false)
{
    m_ui->setupUi(this);

    m_ui->qmakeAdditonalArgumentsLineEdit->setText(m_step->userArguments());
    m_ui->qmlDebuggingLibraryCheckBox->setChecked(m_step->linkQmlDebuggingLibrary());
    m_ui->qtQuickCompilerCheckBox->setChecked(m_step->useQtQuickCompiler());
    m_ui->separateDebugInfoCheckBox->setChecked(m_step->separateDebugInfo());

    qmakeBuildConfigChanged();

    updateSummaryLabel();
    updateEffectiveQMakeCall();
    updateQmlDebuggingOption();
    updateQtQuickCompilerOption();

    connect(m_ui->qmakeAdditonalArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(qmakeArgumentsLineEdited()));
    connect(m_ui->buildConfigurationComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(buildConfigurationSelected()));
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
    connect(step, SIGNAL(userArgumentsChanged()),
            this, SLOT(userArgumentsChanged()));
    connect(step, SIGNAL(linkQmlDebuggingLibraryChanged()),
            this, SLOT(linkQmlDebuggingLibraryChanged()));
    connect(step->project(), &Project::projectLanguagesUpdated,
            this, &QMakeStepConfigWidget::linkQmlDebuggingLibraryChanged);
    connect(step, &QMakeStep::useQtQuickCompilerChanged,
            this, &QMakeStepConfigWidget::useQtQuickCompilerChanged);
    connect(step, &QMakeStep::separateDebugInfoChanged,
            this, &QMakeStepConfigWidget::separateDebugInfoChanged);
    connect(step->qmakeBuildConfiguration(), SIGNAL(qmakeBuildConfigurationChanged()),
            this, SLOT(qmakeBuildConfigChanged()));
    connect(step->target(), SIGNAL(kitChanged()), this, SLOT(qtVersionChanged()));
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(dumpUpdatedFor(Utils::FileName)),
            this, SLOT(qtVersionChanged()));
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
    bool debug = bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild;
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
    QtSupport::BaseQtVersion::QmakeBuildConfigs buildConfiguration = bc->qmakeBuildConfiguration();
    if (m_ui->buildConfigurationComboBox->currentIndex() == 0) { // debug
        buildConfiguration = buildConfiguration | QtSupport::BaseQtVersion::DebugBuild;
    } else {
        buildConfiguration = buildConfiguration & ~QtSupport::BaseQtVersion::DebugBuild;
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
    connect(question, SIGNAL(finished(int)), this, SLOT(recompileMessageBoxFinished(int)));
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
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(m_step->target()->kit());
    if (!qtVersion) {
        setSummaryText(tr("<b>qmake:</b> No Qt version set. Cannot run qmake."));
        return;
    }
    // We don't want the full path to the .pro file
    QString args = m_step->allArguments(true);
    // And we only use the .pro filename not the full path
    QString program = qtVersion->qmakeCommand().fileName();
    setSummaryText(tr("<b>qmake:</b> %1 %2").arg(program, args));
}

void QMakeStepConfigWidget::updateQmlDebuggingOption()
{
    QString warningText;
    bool supported = QtSupport::BaseQtVersion::isQmlDebuggingSupported(m_step->target()->kit(),
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
    bool supported = QtSupport::BaseQtVersion::isQtQuickCompilerSupported(m_step->target()->kit(),
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
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(m_step->target()->kit());
    QString program = tr("<No Qt version>");
    if (qtVersion)
        program = qtVersion->qmakeCommand().fileName();
    m_ui->qmakeArgumentsEdit->setPlainText(program + QLatin1Char(' ') + m_step->allArguments());
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

QMakeStepFactory::~QMakeStepFactory()
{
}

bool QMakeStepFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return false;
    if (!qobject_cast<QmakeBuildConfiguration *>(parent->parent()))
        return false;
    return id == QMAKE_BS_ID;
}

ProjectExplorer::BuildStep *QMakeStepFactory::create(BuildStepList *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new QMakeStep(parent);
}

bool QMakeStepFactory::canClone(BuildStepList *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::BuildStep *QMakeStepFactory::clone(BuildStepList *parent, ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new QMakeStep(parent, qobject_cast<QMakeStep *>(source));
}

bool QMakeStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *QMakeStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    QMakeStep *bs = new QMakeStep(parent);
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<Core::Id> QMakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        if (QmakeBuildConfiguration *bc = qobject_cast<QmakeBuildConfiguration *>(parent->parent()))
            if (!bc->qmakeStep())
                return QList<Core::Id>() << Core::Id(QMAKE_BS_ID);
    return QList<Core::Id>();
}

QString QMakeStepFactory::displayNameForId(Core::Id id) const
{
    if (id == QMAKE_BS_ID)
        return tr("qmake");
    return QString();
}


QMakeStepConfig::TargetArchConfig QMakeStepConfig::targetArchFor(const Abi &targetAbi, const BaseQtVersion *version)
{
    QMakeStepConfig::TargetArchConfig arch = QMakeStepConfig::NoArch;
    if (!version || version->type() != QLatin1String(QtSupport::Constants::DESKTOPQT))
        return arch;
    if ((targetAbi.os() == ProjectExplorer::Abi::MacOS)
            && (targetAbi.binaryFormat() == ProjectExplorer::Abi::MachOFormat)) {
        if (targetAbi.architecture() == ProjectExplorer::Abi::X86Architecture) {
            if (targetAbi.wordWidth() == 32)
                arch = QMakeStepConfig::X86;
            else if (targetAbi.wordWidth() == 64)
                arch = QMakeStepConfig::X86_64;
        } else if (targetAbi.architecture() == ProjectExplorer::Abi::PowerPCArchitecture) {
            if (targetAbi.wordWidth() == 32)
                arch = QMakeStepConfig::PPC;
            else if (targetAbi.wordWidth() == 64)
                arch = QMakeStepConfig::PPC64;
        }
    }
    return arch;
}

QMakeStepConfig::OsType QMakeStepConfig::osTypeFor(const ProjectExplorer::Abi &targetAbi, const BaseQtVersion *version)
{
    QMakeStepConfig::OsType os = QMakeStepConfig::NoOsType;
    const char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios";
    if (!version || version->type() != QLatin1String(IOSQT))
        return os;
    if ((targetAbi.os() == ProjectExplorer::Abi::MacOS)
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
        arguments << QLatin1String("CONFIG+=x86");
    else if (archConfig == X86_64)
        arguments << QLatin1String("CONFIG+=x86_64");
    else if (archConfig == PPC)
        arguments << QLatin1String("CONFIG+=ppc");
    else if (archConfig == PPC64)
        arguments << QLatin1String("CONFIG+=ppc64");

    if (osType == IphoneSimulator)
        arguments << QLatin1String("CONFIG+=iphonesimulator");
    else if (osType == IphoneOS)
        arguments << QLatin1String("CONFIG+=iphoneos");

    if (linkQmlDebuggingQQ1)
        arguments << QLatin1String("CONFIG+=declarative_debug");
    if (linkQmlDebuggingQQ2)
        arguments << QLatin1String("CONFIG+=qml_debug");

    if (useQtQuickCompiler)
        arguments << QLatin1String("CONFIG+=qtquickcompiler");

    if (separateDebugInfo)
        arguments << QLatin1String("CONFIG+=force_debug_info")
                  << QLatin1String("CONFIG+=separate_debug_info");

    return arguments;
}

