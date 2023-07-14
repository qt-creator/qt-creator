// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsinstallstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsbuildstep.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagertr.h"
#include "qbssession.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QJsonObject>
#include <QLabel>
#include <QPlainTextEdit>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsInstallStep:
// --------------------------------------------------------------------

QbsInstallStep::QbsInstallStep(BuildStepList *bsl, Id id)
    : BuildStep(bsl, id)
{
    setDisplayName(Tr::tr("Qbs Install"));
    setSummaryText(Tr::tr("<b>Qbs:</b> %1").arg("install"));

    const auto labelPlacement = BoolAspect::LabelPlacement::AtCheckBox;
    dryRun.setSettingsKey("Qbs.DryRun");
    dryRun.setLabel(Tr::tr("Dry run"), labelPlacement);

    keepGoing.setSettingsKey("Qbs.DryKeepGoing");
    keepGoing.setLabel(Tr::tr("Keep going"), labelPlacement);

    cleanInstallRoot.setSettingsKey("Qbs.RemoveFirst");
    cleanInstallRoot.setLabel(Tr::tr("Remove first"), labelPlacement);
}

QbsInstallStep::~QbsInstallStep()
{
    doCancel();
    if (m_session)
        m_session->disconnect(this);
}

bool QbsInstallStep::init()
{
    QTC_ASSERT(!target()->buildSystem()->isParsing() && !m_session, return false);
    return true;
}

void QbsInstallStep::doRun()
{
    m_session = static_cast<QbsBuildSystem *>(target()->buildSystem())->session();

    QJsonObject request;
    request.insert("type", "install-project");
    request.insert("install-root", installRoot().path());
    request.insert("clean-install-root", cleanInstallRoot());
    request.insert("keep-going", keepGoing());
    request.insert("dry-run", dryRun());
    m_session->sendRequest(request);

    m_maxProgress = 0;
    connect(m_session, &QbsSession::projectInstalled, this, &QbsInstallStep::installDone);
    connect(m_session, &QbsSession::taskStarted, this, &QbsInstallStep::handleTaskStarted);
    connect(m_session, &QbsSession::taskProgress, this, &QbsInstallStep::handleProgress);
    connect(m_session, &QbsSession::errorOccurred, this, [this] {
        installDone(ErrorInfo(Tr::tr("Installing canceled: Qbs session failed.")));
    });
}

void QbsInstallStep::doCancel()
{
    if (m_session)
        m_session->cancelCurrentJob();
}

FilePath QbsInstallStep::installRoot() const
{
    const QbsBuildStep * const bs = buildConfig()->qbsStep();
    return bs ? bs->installRoot() : FilePath();
}

const QbsBuildConfiguration *QbsInstallStep::buildConfig() const
{
    return static_cast<QbsBuildConfiguration *>(target()->activeBuildConfiguration());
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
                                         const FilePath &file, int line)
{
    emit addOutput(message, OutputFormat::Stdout);
    emit addTask(CompileTask(type, message, file, line), 1);
}

QbsBuildStepData QbsInstallStep::stepData() const
{
    QbsBuildStepData data;
    data.command = "install";
    data.dryRun = dryRun();
    data.keepGoing = keepGoing();
    data.noBuild = true;
    data.cleanInstallRoot = cleanInstallRoot();
    data.isInstallStep = true;
    auto bs = static_cast<QbsBuildConfiguration *>(target()->activeBuildConfiguration())->qbsStep();
    if (bs)
        data.installRoot = bs->installRoot();
    return data;
}

QWidget *QbsInstallStep::createConfigWidget()
{
    auto widget = new QWidget;

    auto installRootValueLabel = new QLabel(installRoot().toUserOutput());

    auto commandLineKeyLabel = new QLabel(Tr::tr("Equivalent command line:"));
    commandLineKeyLabel->setAlignment(Qt::AlignTop);

    auto commandLineTextEdit = new QPlainTextEdit(widget);
    commandLineTextEdit->setReadOnly(true);
    commandLineTextEdit->setTextInteractionFlags(Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);
    commandLineTextEdit->setMinimumHeight(QFontMetrics(widget->font()).height() * 8);

    using namespace Layouting;
    Form {
        Tr::tr("Install root:"), installRootValueLabel, br,
        Tr::tr("Flags:"), dryRun, keepGoing, cleanInstallRoot, br,
        commandLineKeyLabel, commandLineTextEdit
     }.attachTo(widget);

    const auto updateState = [this, commandLineTextEdit, installRootValueLabel] {
        installRootValueLabel->setText(installRoot().toUserOutput());
        commandLineTextEdit->setPlainText(buildConfig()->equivalentCommandLine(stepData()));
    };

    connect(target(), &Target::parsingFinished, this, updateState);
    connect(this, &ProjectConfiguration::displayNameChanged, this, updateState);

    connect(&dryRun, &BaseAspect::changed, this, updateState);
    connect(&keepGoing, &BaseAspect::changed, this, updateState);
    connect(&cleanInstallRoot, &BaseAspect::changed, this, updateState);

    const QbsBuildConfiguration * const bc = buildConfig();
    connect(bc, &QbsBuildConfiguration::qbsConfigurationChanged, this, updateState);
    if (bc->qbsStep())
        connect(bc->qbsStep(), &QbsBuildStep::qbsBuildOptionsChanged, this, updateState);

    updateState();

    return widget;
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
    setDisplayName(Tr::tr("Qbs Install"));
}

} // namespace Internal
} // namespace QbsProjectManager
