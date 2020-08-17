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

#include "qbsinstallstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsbuildstep.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbssession.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>

#include <QJsonObject>
#include <QLabel>
#include <QPlainTextEdit>

using namespace ProjectExplorer;

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

const char QBS_REMOVE_FIRST[] = "Qbs.RemoveFirst";
const char QBS_DRY_RUN[] = "Qbs.DryRun";
const char QBS_KEEP_GOING[] = "Qbs.DryKeepGoing";

class QbsInstallStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
public:
    QbsInstallStepConfigWidget(QbsInstallStep *step);
};

// --------------------------------------------------------------------
// QbsInstallStep:
// --------------------------------------------------------------------

QbsInstallStep::QbsInstallStep(BuildStepList *bsl, Core::Id id)
    : BuildStep(bsl, id)
{
    setDisplayName(tr("Qbs Install"));

    const auto labelPlacement = BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel;
    m_dryRun = addAspect<BoolAspect>();
    m_dryRun->setSettingsKey(QBS_DRY_RUN);
    m_dryRun->setLabel(tr("Dry run"), labelPlacement);

    m_keepGoing = addAspect<BoolAspect>();
    m_keepGoing->setSettingsKey(QBS_KEEP_GOING);
    m_keepGoing->setLabel(tr("Keep going"), labelPlacement);

    m_cleanInstallRoot = addAspect<BoolAspect>();
    m_cleanInstallRoot->setSettingsKey(QBS_REMOVE_FIRST);
    m_cleanInstallRoot->setLabel(tr("Remove first"), labelPlacement);
}

QbsInstallStep::~QbsInstallStep()
{
    doCancel();
    if (m_session)
        m_session->disconnect(this);
}

bool QbsInstallStep::init()
{
    QTC_ASSERT(!buildSystem()->isParsing() && !m_session, return false);
    return true;
}

void QbsInstallStep::doRun()
{
    m_session = static_cast<QbsBuildSystem *>(buildSystem())->session();

    QJsonObject request;
    request.insert("type", "install");
    request.insert("install-root", installRoot());
    request.insert("clean-install-root", m_cleanInstallRoot->value());
    request.insert("keep-going", m_keepGoing->value());
    request.insert("dry-run", m_dryRun->value());
    m_session->sendRequest(request);

    m_maxProgress = 0;
    connect(m_session, &QbsSession::projectInstalled, this, &QbsInstallStep::installDone);
    connect(m_session, &QbsSession::taskStarted, this, &QbsInstallStep::handleTaskStarted);
    connect(m_session, &QbsSession::taskProgress, this, &QbsInstallStep::handleProgress);
    connect(m_session, &QbsSession::errorOccurred, this, [this] {
        installDone(ErrorInfo(tr("Installing canceled: Qbs session failed.")));
    });
}

ProjectExplorer::BuildStepConfigWidget *QbsInstallStep::createConfigWidget()
{
    return new QbsInstallStepConfigWidget(this);
}

void QbsInstallStep::doCancel()
{
    if (m_session)
        m_session->cancelCurrentJob();
}

QString QbsInstallStep::installRoot() const
{
    const QbsBuildStep * const bs = buildConfig()->qbsStep();
    return bs ? bs->installRoot().toString() : QString();
}

const QbsBuildConfiguration *QbsInstallStep::buildConfig() const
{
    return static_cast<QbsBuildConfiguration *>(buildConfiguration());
}

void QbsInstallStep::installDone(const ErrorInfo &error)
{
    m_session->disconnect(this);
    m_session = nullptr;

    for (const ErrorInfoItem &item : error.items)
        createTaskAndOutput(Task::Error, item.description, item.filePath, item.line);

    emit finished(!error.hasError());
}

void QbsInstallStep::handleTaskStarted(const QString &desciption, int max)
{
    m_description = desciption;
    m_maxProgress = max;
}

void QbsInstallStep::handleProgress(int value)
{
    if (m_maxProgress > 0)
        emit progress(value * 100 / m_maxProgress, m_description);
}

void QbsInstallStep::createTaskAndOutput(Task::TaskType type, const QString &message,
                                         const Utils::FilePath &file, int line)
{
    emit addOutput(message, OutputFormat::Stdout);
    emit addTask(CompileTask(type, message, file, line), 1);
}

QbsBuildStepData QbsInstallStep::stepData() const
{
    QbsBuildStepData data;
    data.command = "install";
    data.dryRun = m_dryRun->value();
    data.keepGoing = m_keepGoing->value();
    data.noBuild = true;
    data.cleanInstallRoot = m_cleanInstallRoot->value();
    data.isInstallStep = true;
    auto bs = static_cast<QbsBuildConfiguration *>(target()->activeBuildConfiguration())->qbsStep();
    if (bs)
        data.installRoot = bs->installRoot();
    return data;
}

// --------------------------------------------------------------------
// QbsInstallStepConfigWidget:
// --------------------------------------------------------------------

QbsInstallStepConfigWidget::QbsInstallStepConfigWidget(QbsInstallStep *step) :
    BuildStepConfigWidget(step)
{
    setSummaryText(QbsInstallStep::tr("<b>Qbs:</b> %1").arg("install"));

    auto installRootValueLabel = new QLabel(step->installRoot());

    auto commandLineKeyLabel = new QLabel(QbsInstallStep::tr("Equivalent command line:"));
    commandLineKeyLabel->setAlignment(Qt::AlignTop);

    auto commandLineTextEdit = new QPlainTextEdit(this);
    commandLineTextEdit->setReadOnly(true);
    commandLineTextEdit->setTextInteractionFlags(Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);
    commandLineTextEdit->setMinimumHeight(QFontMetrics(font()).height() * 8);

    LayoutBuilder builder(this);
    builder.addItems(new QLabel(QbsInstallStep::tr("Install root:")), installRootValueLabel);

    builder.startNewRow();
    builder.addItem(new QLabel(QbsInstallStep::tr("Flags:")));
    step->m_dryRun->addToLayout(builder);
    step->m_keepGoing->addToLayout(builder);
    step->m_cleanInstallRoot->addToLayout(builder);

    builder.startNewRow();
    builder.addItems(commandLineKeyLabel, commandLineTextEdit);

    const auto updateState = [this, step, commandLineTextEdit, installRootValueLabel] {
        QString installRoot = step->installRoot();
        installRootValueLabel->setText(installRoot);

        QString command = step->buildConfig()->equivalentCommandLine(step->stepData());
        commandLineTextEdit->setPlainText(command);
    };

    connect(step->target(), &Target::parsingFinished, this, updateState);
    connect(step, &ProjectConfiguration::displayNameChanged, this, updateState);

    connect(step->m_dryRun, &BoolAspect::changed, this, updateState);
    connect(step->m_keepGoing, &BoolAspect::changed, this, updateState);
    connect(step->m_cleanInstallRoot, &BoolAspect::changed, this, updateState);

    const QbsBuildConfiguration * const bc = step->buildConfig();
    connect(bc, &QbsBuildConfiguration::qbsConfigurationChanged, this, updateState);
    if (bc->qbsStep())
        connect(bc->qbsStep(), &QbsBuildStep::qbsBuildOptionsChanged, this, updateState);

    updateState();
}

// --------------------------------------------------------------------
// QbsInstallStepFactory:
// --------------------------------------------------------------------

QbsInstallStepFactory::QbsInstallStepFactory()
{
    registerStep<QbsInstallStep>(Constants::QBS_INSTALLSTEP_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    setSupportedDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    setSupportedProjectType(Constants::PROJECT_ID);
    setDisplayName(QbsInstallStep::tr("Qbs Install"));
}

} // namespace Internal
} // namespace QbsProjectManager
