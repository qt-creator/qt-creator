// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbscleanstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagertr.h"
#include "qbssession.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonObject>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsCleanStep:
// --------------------------------------------------------------------

QbsCleanStep::QbsCleanStep(BuildStepList *bsl, Utils::Id id)
    : BuildStep(bsl, id)
{
    setDisplayName(Tr::tr("Qbs Clean"));

    m_dryRunAspect = addAspect<BoolAspect>();
    m_dryRunAspect->setSettingsKey("Qbs.DryRun");
    m_dryRunAspect->setLabel(Tr::tr("Dry run:"), BoolAspect::LabelPlacement::InExtraLabel);

    m_keepGoingAspect = addAspect<BoolAspect>();
    m_keepGoingAspect->setSettingsKey("Qbs.DryKeepGoing");
    m_keepGoingAspect->setLabel(Tr::tr("Keep going:"), BoolAspect::LabelPlacement::InExtraLabel);

    auto effectiveCommandAspect = addAspect<StringAspect>();
    effectiveCommandAspect->setDisplayStyle(StringAspect::TextEditDisplay);
    effectiveCommandAspect->setLabelText(Tr::tr("Equivalent command line:"));

    setSummaryUpdater([this, effectiveCommandAspect] {
        QbsBuildStepData data;
        data.command = "clean";
        data.dryRun = m_dryRunAspect->value();
        data.keepGoing = m_keepGoingAspect->value();
        QString command = static_cast<QbsBuildConfiguration *>(buildConfiguration())
                 ->equivalentCommandLine(data);
        effectiveCommandAspect->setValue(command);
        return Tr::tr("<b>Qbs:</b> %1").arg("clean");
    });
}

QbsCleanStep::~QbsCleanStep()
{
    doCancel();
    if (m_session)
        m_session->disconnect(this);
}

void QbsCleanStep::dropSession()
{
    if (m_session) {
        doCancel();
        m_session->disconnect(this);
        m_session = nullptr;
    }
}

bool QbsCleanStep::init()
{
    if (buildSystem()->isParsing() || m_session)
        return false;
    const auto bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());
    if (!bc)
        return false;
    m_products = bc->products();
    return true;
}

void QbsCleanStep::doRun()
{
    m_session = static_cast<QbsBuildSystem*>(buildSystem())->session();
    if (!m_session) {
        emit addOutput(Tr::tr("No qbs session exists for this target."), OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    QJsonObject request;
    request.insert("type", "clean-project");
    if (!m_products.isEmpty())
        request.insert("products", QJsonArray::fromStringList(m_products));
    request.insert("dry-run", m_dryRunAspect->value());
    request.insert("keep-going", m_keepGoingAspect->value());
    m_session->sendRequest(request);
    m_maxProgress = 0;
    connect(m_session, &QbsSession::projectCleaned, this, &QbsCleanStep::cleaningDone);
    connect(m_session, &QbsSession::taskStarted, this, &QbsCleanStep::handleTaskStarted);
    connect(m_session, &QbsSession::taskProgress, this, &QbsCleanStep::handleProgress);
    connect(m_session, &QbsSession::errorOccurred, this, [this] {
        cleaningDone(ErrorInfo(Tr::tr("Cleaning canceled: Qbs session failed.")));
    });
}

void QbsCleanStep::doCancel()
{
    if (m_session)
        m_session->cancelCurrentJob();
}

void QbsCleanStep::cleaningDone(const ErrorInfo &error)
{
    m_session->disconnect(this);
    m_session = nullptr;

    for (const ErrorInfoItem &item : error.items)
        createTaskAndOutput(Task::Error, item.description, item.filePath.toString(), item.line);
    emit finished(!error.hasError());
}

void QbsCleanStep::handleTaskStarted(const QString &desciption, int max)
{
    Q_UNUSED(desciption)
    m_maxProgress = max;
}

void QbsCleanStep::handleProgress(int value)
{
    if (m_maxProgress > 0)
        emit progress(value * 100 / m_maxProgress, m_description);
}

void QbsCleanStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type, const QString &message, const QString &file, int line)
{
    emit addOutput(message, OutputFormat::Stdout);
    emit addTask(CompileTask(type, message, Utils::FilePath::fromString(file), line), 1);
}

// --------------------------------------------------------------------
// QbsCleanStepFactory:
// --------------------------------------------------------------------

QbsCleanStepFactory::QbsCleanStepFactory()
{
    registerStep<QbsCleanStep>(Constants::QBS_CLEANSTEP_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    setSupportedConfiguration(Constants::QBS_BC_ID);
    setDisplayName(Tr::tr("Qbs Clean"));
}

} // namespace Internal
} // namespace QbsProjectManager
