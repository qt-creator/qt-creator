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

#include <QCheckBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QJsonObject>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSpacerItem>

using namespace ProjectExplorer;

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

static const char QBS_REMOVE_FIRST[] = "Qbs.RemoveFirst";
static const char QBS_DRY_RUN[] = "Qbs.DryRun";
static const char QBS_KEEP_GOING[] = "Qbs.DryKeepGoing";

namespace QbsProjectManager {
namespace Internal {

class QbsInstallStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
public:
    QbsInstallStepConfigWidget(QbsInstallStep *step);

private:
    void updateState();

    void changeRemoveFirst(bool rf) { m_step->setRemoveFirst(rf); }
    void changeDryRun(bool dr) { m_step->setDryRun(dr); }
    void changeKeepGoing(bool kg) { m_step->setKeepGoing(kg); }

private:
    QbsInstallStep *m_step;
    bool m_ignoreChange;

    QCheckBox *m_dryRunCheckBox;
    QCheckBox *m_keepGoingCheckBox;
    QCheckBox *m_removeFirstCheckBox;
    QPlainTextEdit *m_commandLineTextEdit;
    QLabel *m_installRootValueLabel;
};

// --------------------------------------------------------------------
// QbsInstallStep:
// --------------------------------------------------------------------

QbsInstallStep::QbsInstallStep(BuildStepList *bsl, Core::Id id)
    : BuildStep(bsl, id)
{
    setDisplayName(tr("Qbs Install"));

    const QbsBuildConfiguration * const bc = buildConfig();
    connect(bc, &QbsBuildConfiguration::qbsConfigurationChanged, this, &QbsInstallStep::changed);
    if (bc->qbsStep()) {
        connect(bc->qbsStep(), &QbsBuildStep::qbsBuildOptionsChanged,
                this, &QbsInstallStep::changed);
    }
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
    request.insert("clean-install-root", m_cleanInstallRoot);
    request.insert("keep-going", m_keepGoing);
    request.insert("dry-run", m_dryRun);
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

bool QbsInstallStep::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::BuildStep::fromMap(map))
        return false;

    m_cleanInstallRoot = map.value(QBS_REMOVE_FIRST, false).toBool();
    m_dryRun = map.value(QBS_DRY_RUN, false).toBool();
    m_keepGoing = map.value(QBS_KEEP_GOING, false).toBool();
    return true;
}

QVariantMap QbsInstallStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QBS_REMOVE_FIRST, m_cleanInstallRoot);
    map.insert(QBS_DRY_RUN, m_dryRun);
    map.insert(QBS_KEEP_GOING, m_keepGoing);
    return map;
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

void QbsInstallStep::setRemoveFirst(bool rf)
{
    if (m_cleanInstallRoot == rf)
        return;
    m_cleanInstallRoot = rf;
    emit changed();
}

void QbsInstallStep::setDryRun(bool dr)
{
    if (m_dryRun == dr)
        return;
    m_dryRun = dr;
    emit changed();
}

void QbsInstallStep::setKeepGoing(bool kg)
{
    if (m_keepGoing == kg)
        return;
    m_keepGoing = kg;
    emit changed();
}

QbsBuildStepData QbsInstallStep::stepData() const
{
    QbsBuildStepData data;
    data.command = "install";
    data.dryRun = dryRun();
    data.keepGoing = keepGoing();
    data.noBuild = true;
    data.cleanInstallRoot = removeFirst();
    data.isInstallStep = true;
    auto bs = static_cast<QbsBuildConfiguration *>(target()->activeBuildConfiguration())->qbsStep();
    if (bs)
        data.installRoot = bs->installRoot();
    return data;
};

// --------------------------------------------------------------------
// QbsInstallStepConfigWidget:
// --------------------------------------------------------------------

QbsInstallStepConfigWidget::QbsInstallStepConfigWidget(QbsInstallStep *step) :
    BuildStepConfigWidget(step), m_step(step), m_ignoreChange(false)
{
    connect(m_step, &ProjectExplorer::ProjectConfiguration::displayNameChanged,
            this, &QbsInstallStepConfigWidget::updateState);
    connect(m_step, &QbsInstallStep::changed,
            this, &QbsInstallStepConfigWidget::updateState);

    setContentsMargins(0, 0, 0, 0);

    auto installRootLabel = new QLabel(this);

    auto flagsLabel = new QLabel(this);

    m_dryRunCheckBox = new QCheckBox(this);
    m_keepGoingCheckBox = new QCheckBox(this);
    m_removeFirstCheckBox = new QCheckBox(this);

    auto horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(m_dryRunCheckBox);
    horizontalLayout->addWidget(m_keepGoingCheckBox);
    horizontalLayout->addWidget(m_removeFirstCheckBox);
    horizontalLayout->addStretch(1);

    auto commandLineKeyLabel = new QLabel(this);
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(commandLineKeyLabel->sizePolicy().hasHeightForWidth());
    commandLineKeyLabel->setSizePolicy(sizePolicy);
    commandLineKeyLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

    m_commandLineTextEdit = new QPlainTextEdit(this);
    QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(m_commandLineTextEdit->sizePolicy().hasHeightForWidth());
    m_commandLineTextEdit->setSizePolicy(sizePolicy1);
    m_commandLineTextEdit->setReadOnly(true);
    m_commandLineTextEdit->setTextInteractionFlags(Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

    m_installRootValueLabel = new QLabel(this);

    auto formLayout = new QFormLayout(this);
    formLayout->setWidget(0, QFormLayout::LabelRole, installRootLabel);
    formLayout->setWidget(0, QFormLayout::FieldRole, m_installRootValueLabel);
    formLayout->setWidget(1, QFormLayout::LabelRole, flagsLabel);
    formLayout->setLayout(1, QFormLayout::FieldRole, horizontalLayout);
    formLayout->setWidget(2, QFormLayout::LabelRole, commandLineKeyLabel);
    formLayout->setWidget(2, QFormLayout::FieldRole, m_commandLineTextEdit);

    QWidget::setTabOrder(m_dryRunCheckBox, m_keepGoingCheckBox);
    QWidget::setTabOrder(m_keepGoingCheckBox, m_removeFirstCheckBox);
    QWidget::setTabOrder(m_removeFirstCheckBox, m_commandLineTextEdit);

    installRootLabel->setText(QbsInstallStep::tr("Install root:"));
    flagsLabel->setText(QbsInstallStep::tr("Flags:"));
    m_dryRunCheckBox->setText(QbsInstallStep::tr("Dry run"));
    m_keepGoingCheckBox->setText(QbsInstallStep::tr("Keep going"));
    m_removeFirstCheckBox->setText(QbsInstallStep::tr("Remove first"));
    commandLineKeyLabel->setText(QbsInstallStep::tr("Equivalent command line:"));
    m_installRootValueLabel->setText(QString());

    connect(m_removeFirstCheckBox, &QAbstractButton::toggled,
            this, &QbsInstallStepConfigWidget::changeRemoveFirst);
    connect(m_dryRunCheckBox, &QAbstractButton::toggled,
            this, &QbsInstallStepConfigWidget::changeDryRun);
    connect(m_keepGoingCheckBox, &QAbstractButton::toggled,
            this, &QbsInstallStepConfigWidget::changeKeepGoing);

    connect(m_step->target(), &Target::parsingFinished,
            this, &QbsInstallStepConfigWidget::updateState);

    updateState();
    setSummaryText(QbsInstallStep::tr("<b>Qbs:</b> %1").arg("install"));
}

void QbsInstallStepConfigWidget::updateState()
{
    if (!m_ignoreChange) {
        m_installRootValueLabel->setText(m_step->installRoot());
        m_removeFirstCheckBox->setChecked(m_step->removeFirst());
        m_dryRunCheckBox->setChecked(m_step->dryRun());
        m_keepGoingCheckBox->setChecked(m_step->keepGoing());
    }

    QString command = m_step->buildConfig()->equivalentCommandLine(m_step->stepData());

    m_commandLineTextEdit->setPlainText(command);
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
