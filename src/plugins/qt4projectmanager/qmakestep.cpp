/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmakestep.h"

#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qt4projectmanager.h"
#include "makestep.h"
#include "qtversionmanager.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QCoreApplication>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;

QMakeStep::QMakeStep(Qt4Project *project)
    : AbstractMakeStep(project), m_pro(project), m_forced(false)
{
}

QMakeStep::~QMakeStep()
{
}

QStringList QMakeStep::arguments(const QString &buildConfiguration)
{
    QStringList additonalArguments = m_values.value(buildConfiguration).qmakeArgs;
    ProjectExplorer::BuildConfiguration *bc = m_pro->buildConfiguration(buildConfiguration);
    QStringList arguments;
    arguments << project()->file()->fileName();
    arguments << "-r";

    if (!additonalArguments.contains("-spec"))
        arguments << "-spec" << m_pro->qtVersion(bc)->mkspec();

#ifdef Q_OS_WIN
    ToolChain::ToolChainType type = m_pro->toolChainType(bc);
    if (type == ToolChain::GCC_MAEMO)
        arguments << QLatin1String("-unix");
#endif

    if (bc->value("buildConfiguration").isValid()) {
        QStringList configarguments;
        QtVersion::QmakeBuildConfigs defaultBuildConfiguration = m_pro->qtVersion(bc)->defaultBuildConfig();
        QtVersion::QmakeBuildConfigs projectBuildConfiguration = QtVersion::QmakeBuildConfig(bc->value("buildConfiguration").toInt());
        if ((defaultBuildConfiguration & QtVersion::BuildAll) && !(projectBuildConfiguration & QtVersion::BuildAll))
            configarguments << "CONFIG-=debug_and_release";
        if (!(defaultBuildConfiguration & QtVersion::BuildAll) && (projectBuildConfiguration & QtVersion::BuildAll))
            configarguments << "CONFIG+=debug_and_release";
        if ((defaultBuildConfiguration & QtVersion::DebugBuild) && !(projectBuildConfiguration & QtVersion::DebugBuild))
            configarguments << "CONFIG+=release";
        if (!(defaultBuildConfiguration & QtVersion::DebugBuild) && (projectBuildConfiguration & QtVersion::DebugBuild))
            configarguments << "CONFIG+=debug";
        if (!configarguments.isEmpty())
            arguments << configarguments;
    } else {
        qWarning()<< "The project should always have a qmake build configuration set";
    }

    if (!additonalArguments.isEmpty())
        arguments << additonalArguments;

    return arguments;
}

bool QMakeStep::init(const QString &name)
{
    m_buildConfiguration = name;
    ProjectExplorer::BuildConfiguration *bc = m_pro->buildConfiguration(name);
    const QtVersion *qtVersion = m_pro->qtVersion(bc);


    if (!qtVersion->isValid()) {
#if defined(Q_WS_MAC)
        emit addToOutputWindow(tr("\n<font color=\"#ff0000\"><b>No valid Qt version set. Set one in Preferences </b></font>\n"));
#else
        emit addToOutputWindow(tr("\n<font color=\"#ff0000\"><b>No valid Qt version set. Set one in Tools/Options </b></font>\n"));
#endif
        return false;
    }

    QStringList args = arguments(name);
    QString workingDirectory = m_pro->buildDirectory(bc);

    QString program = qtVersion->qmakeCommand();

    // Check wheter we need to run qmake
    m_needToRunQMake = true;
    if (QDir(workingDirectory).exists(QLatin1String("Makefile"))) {
        QString qmakePath = QtVersionManager::findQMakeBinaryFromMakefile(workingDirectory);
        if (qtVersion->qmakeCommand() == qmakePath) {
            m_needToRunQMake = !m_pro->compareBuildConfigurationToImportFrom(bc, workingDirectory);
        }
    }

    if (m_forced) {
        m_forced = false;
        m_needToRunQMake = true;
    }

    setEnabled(m_needToRunQMake);
    setWorkingDirectory(workingDirectory);
    setCommand(program);
    setArguments(args);
    setEnvironment(m_pro->environment(bc));

    setBuildParser(ProjectExplorer::Constants::BUILD_PARSER_QMAKE);
    return AbstractMakeStep::init(name);
}

void QMakeStep::run(QFutureInterface<bool> &fi)
{
    if (qobject_cast<Qt4Project *>(project())->rootProjectNode()->projectType() == ScriptTemplate) {
        fi.reportResult(true);
        return;
    }

    if (!m_needToRunQMake) {
        emit addToOutputWindow(tr("<font color=\"#0000ff\">Configuration unchanged, skipping QMake step.</font>"));
        fi.reportResult(true);
        return;
    }
    AbstractMakeStep::run(fi);
}

QString QMakeStep::name()
{
    return Constants::QMAKESTEP;
}

QString QMakeStep::displayName()
{
    return "QMake";
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
    m_forced = true;
    AbstractProcessStep::processStartupFailed();
}

bool QMakeStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    bool result = AbstractProcessStep::processFinished(exitCode, status);
    if (!result)
        m_forced = true;
    return result;
}

void QMakeStep::setQMakeArguments(const QString &buildConfiguration, const QStringList &arguments)
{
    m_values[buildConfiguration].qmakeArgs = arguments;
    emit changed();
}

QStringList QMakeStep::qmakeArguments(const QString &buildConfiguration)
{
    return m_values[buildConfiguration].qmakeArgs;
}

void QMakeStep::restoreFromMap(const QString &buildConfiguration, const QMap<QString, QVariant> &map)
{
    m_values[buildConfiguration].qmakeArgs = map.value("qmakeArgs").toStringList();
    AbstractProcessStep::restoreFromMap(buildConfiguration, map);
}

void QMakeStep::storeIntoMap(const QString &buildConfiguration, QMap<QString, QVariant> &map)
{
    map["qmakeArgs"] = m_values.value(buildConfiguration).qmakeArgs;
    AbstractProcessStep::storeIntoMap(buildConfiguration, map);
}

void QMakeStep::addBuildConfiguration(const QString & name)
{
    m_values.insert(name, QMakeStepSettings());
}

void QMakeStep::removeBuildConfiguration(const QString & name)
{
    m_values.remove(name);
}

void QMakeStep::copyBuildConfiguration(const QString &source, const QString &dest)
{
    m_values.insert(dest, m_values.value(source));
}

QMakeStepConfigWidget::QMakeStepConfigWidget(QMakeStep *step)
    : BuildStepConfigWidget(), m_step(step)
{
    m_ui.setupUi(this);
    connect(m_ui.qmakeAdditonalArgumentsLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(qmakeArgumentsLineEditTextEdited()));
    connect(m_ui.buildConfigurationComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(buildConfigurationChanged()));
    connect(step, SIGNAL(changed()),
            this, SLOT(update()));
    connect(step->project(), SIGNAL(qtVersionChanged(ProjectExplorer::BuildConfiguration *)),
            this, SLOT(qtVersionChanged(ProjectExplorer::BuildConfiguration *)));
}

QString QMakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void QMakeStepConfigWidget::qtVersionChanged(ProjectExplorer::BuildConfiguration *bc)
{
    if (bc && bc->name() == m_buildConfiguration) {
        updateTitleLabel();
        updateEffectiveQMakeCall();
    }
}

void QMakeStepConfigWidget::updateTitleLabel()
{
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(m_step->project());
    const QtVersion *qtVersion = qt4project->qtVersion(qt4project->buildConfiguration(m_buildConfiguration));
    if (!qtVersion) {
        m_summaryText = tr("<b>QMake:</b> No Qt version set. QMake can not be run.");
        emit updateSummary();
        return;
    }

    QStringList args = m_step->arguments(m_buildConfiguration);
    // We don't want the full path to the .pro file
    int index = args.indexOf(m_step->project()->file()->fileName());
    if (index != -1)
        args[index] = QFileInfo(m_step->project()->file()->fileName()).fileName();

    // And we only use the .pro filename not the full path
    QString program = QFileInfo(qtVersion->qmakeCommand()).fileName();
    m_summaryText = tr("<b>QMake:</b> %1 %2").arg(program, args.join(QString(QLatin1Char(' '))));
    emit updateSummary();

}

void QMakeStepConfigWidget::qmakeArgumentsLineEditTextEdited()
{
    Q_ASSERT(!m_buildConfiguration.isNull());
    m_step->setQMakeArguments(m_buildConfiguration,
            ProjectExplorer::Environment::parseCombinedArgString(m_ui.qmakeAdditonalArgumentsLineEdit->text()));

    static_cast<Qt4Project *>(m_step->project())->invalidateCachedTargetInformation();
    updateTitleLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::buildConfigurationChanged()
{
    ProjectExplorer::BuildConfiguration *bc = m_step->project()->buildConfiguration(m_buildConfiguration);
    QtVersion::QmakeBuildConfigs buildConfiguration = QtVersion::QmakeBuildConfig(bc->value("buildConfiguration").toInt());
    if (m_ui.buildConfigurationComboBox->currentIndex() == 0) {
        // debug
        buildConfiguration = buildConfiguration | QtVersion::DebugBuild;
    } else {
        buildConfiguration = buildConfiguration & ~QtVersion::DebugBuild;
    }
    bc->setValue("buildConfiguration", int(buildConfiguration));
    static_cast<Qt4Project *>(m_step->project())->invalidateCachedTargetInformation();
    updateTitleLabel();
    updateEffectiveQMakeCall();
    // TODO if exact parsing is the default, we need to update the code model
    // and all the Qt4ProFileNodes
    //static_cast<Qt4Project *>(m_step->project())->update();
}

QString QMakeStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

void QMakeStepConfigWidget::update()
{
    init(m_buildConfiguration);
}

void QMakeStepConfigWidget::init(const QString &buildConfiguration)
{
    m_buildConfiguration = buildConfiguration;
    QString qmakeArgs = ProjectExplorer::Environment::joinArgumentList(m_step->qmakeArguments(buildConfiguration));
    m_ui.qmakeAdditonalArgumentsLineEdit->setText(qmakeArgs);
    ProjectExplorer::BuildConfiguration *bc = m_step->project()->buildConfiguration(buildConfiguration);
    bool debug = QtVersion::QmakeBuildConfig(bc->value("buildConfiguration").toInt()) & QtVersion::DebugBuild;
    m_ui.buildConfigurationComboBox->setCurrentIndex(debug? 0 : 1);
    updateTitleLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::updateEffectiveQMakeCall()
{
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(m_step->project());
    const QtVersion *qtVersion = qt4project->qtVersion(qt4project->buildConfiguration(m_buildConfiguration));
    if (qtVersion) {
        QString program = QFileInfo(qtVersion->qmakeCommand()).fileName();
        m_ui.qmakeArgumentsEdit->setPlainText(program + QLatin1Char(' ') + ProjectExplorer::Environment::joinArgumentList(m_step->arguments(m_buildConfiguration)));
    } else {
        m_ui.qmakeArgumentsEdit->setPlainText(tr("No valid Qt version set."));
    }
}

////
// QMakeStepFactory
////

QMakeStepFactory::QMakeStepFactory()
{
}

QMakeStepFactory::~QMakeStepFactory()
{
}

bool QMakeStepFactory::canCreate(const QString & name) const
{
    return (name == Constants::QMAKESTEP);
}

ProjectExplorer::BuildStep * QMakeStepFactory::create(ProjectExplorer::Project * pro, const QString & name) const
{
    Q_UNUSED(name)
    return new QMakeStep(static_cast<Qt4Project *>(pro));
}

QStringList QMakeStepFactory::canCreateForProject(ProjectExplorer::Project *pro) const
{
    Qt4Project *project = qobject_cast<Qt4Project *>(pro);
    if (project && !project->qmakeStep())
        return QStringList() << Constants::QMAKESTEP;
    return QStringList();
}

QString QMakeStepFactory::displayNameForName(const QString &name) const
{
    Q_UNUSED(name);
    return tr("QMake");
}


