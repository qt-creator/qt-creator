// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbscleanstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagertr.h"
#include "qbsrequest.h"
#include "qbssession.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <QJsonArray>
#include <QJsonObject>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace QbsProjectManager::Internal {

// --------------------------------------------------------------------
// QbsCleanStep:
// --------------------------------------------------------------------

QbsCleanStep::QbsCleanStep(BuildStepList *bsl, Id id)
    : BuildStep(bsl, id)
{
    setDisplayName(Tr::tr("Qbs Clean"));

    dryRun.setSettingsKey("Qbs.DryRun");
    dryRun.setLabel(Tr::tr("Dry run:"), BoolAspect::LabelPlacement::InExtraLabel);

    keepGoing.setSettingsKey("Qbs.DryKeepGoing");
    keepGoing.setLabel(Tr::tr("Keep going:"), BoolAspect::LabelPlacement::InExtraLabel);

    effectiveCommand.setDisplayStyle(StringAspect::TextEditDisplay);
    effectiveCommand.setLabelText(Tr::tr("Equivalent command line:"));

    setSummaryUpdater([this] {
        QbsBuildStepData data;
        data.command = "clean";
        data.dryRun = dryRun();
        data.keepGoing = keepGoing();
        QString command = static_cast<QbsBuildConfiguration *>(buildConfiguration())
                 ->equivalentCommandLine(data);
        effectiveCommand.setValue(command);
        return Tr::tr("<b>Qbs:</b> %1").arg("clean");
    });
}

bool QbsCleanStep::init()
{
    if (buildSystem()->isParsing())
        return false;
    const auto bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());
    if (!bc)
        return false;
    m_products = bc->products();
    return true;
}

GroupItem QbsCleanStep::runRecipe()
{
    const auto onSetup = [this](QbsRequest &request) {
        QbsSession *session = static_cast<QbsBuildSystem*>(buildSystem())->session();
        if (!session) {
            emit addOutput(Tr::tr("No qbs session exists for this target."),
                           OutputFormat::ErrorMessage);
            return SetupResult::StopWithError;
        }
        QJsonObject requestData;
        requestData.insert("type", "clean-project");
        if (!m_products.isEmpty())
            requestData.insert("products", QJsonArray::fromStringList(m_products));
        requestData.insert("dry-run", dryRun());
        requestData.insert("keep-going", keepGoing());

        request.setSession(session);
        request.setRequestData(requestData);
        connect(&request, &QbsRequest::progressChanged, this, &BuildStep::progress);
        connect(&request, &QbsRequest::outputAdded, this,
                [this](const QString &output, OutputFormat format) {
            emit addOutput(output, format);
        });
        connect(&request, &QbsRequest::taskAdded, this, [this](const Task &task) {
            emit addTask(task, 1);
        });
        return SetupResult::Continue;
    };

    return QbsRequestTask(onSetup);
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

} // namespace QbsProjectManager::Internal
