// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsinstallstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsbuildstep.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagertr.h"
#include "qbsrequest.h"
#include "qbssession.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QJsonObject>
#include <QLabel>
#include <QPlainTextEdit>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace QbsProjectManager::Internal {

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

bool QbsInstallStep::init()
{
    QTC_ASSERT(!target()->buildSystem()->isParsing(), return false);
    return true;
}

GroupItem QbsInstallStep::runRecipe()
{
    const auto onSetup = [this](QbsRequest &request) {
        QbsSession *session = static_cast<QbsBuildSystem*>(buildSystem())->session();
        if (!session) {
            emit addOutput(Tr::tr("No qbs session exists for this target."),
                           OutputFormat::ErrorMessage);
            return SetupResult::StopWithError;
        }
        QJsonObject requestData;
        requestData.insert("type", "install-project");
        requestData.insert("install-root", installRoot().path());
        requestData.insert("clean-install-root", cleanInstallRoot());
        requestData.insert("keep-going", keepGoing());
        requestData.insert("dry-run", dryRun());

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

FilePath QbsInstallStep::installRoot() const
{
    const QbsBuildStep * const bs = buildConfig()->qbsStep();
    return bs ? bs->installRoot() : FilePath();
}

const QbsBuildConfiguration *QbsInstallStep::buildConfig() const
{
    return static_cast<QbsBuildConfiguration *>(target()->activeBuildConfiguration());
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
        QbsBuildStepData data;
        data.command = "install";
        data.dryRun = dryRun();
        data.keepGoing = keepGoing();
        data.noBuild = true;
        data.cleanInstallRoot = cleanInstallRoot();
        data.isInstallStep = true;
        data.installRoot = installRoot();
        commandLineTextEdit->setPlainText(buildConfig()->equivalentCommandLine(data));
    };

    connect(target(), &Target::parsingFinished, this, updateState);
    connect(buildConfig(), &QbsBuildConfiguration::qbsConfigurationChanged, this, updateState);
    connect(this, &ProjectConfiguration::displayNameChanged, this, updateState);

    connect(&dryRun, &BaseAspect::changed, this, updateState);
    connect(&keepGoing, &BaseAspect::changed, this, updateState);
    connect(&cleanInstallRoot, &BaseAspect::changed, this, updateState);

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

} // namespace QbsProjectManager::Internal
