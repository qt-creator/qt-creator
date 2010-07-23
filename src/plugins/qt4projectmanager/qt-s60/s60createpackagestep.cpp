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

#include "s60createpackagestep.h"

#include "qt4projectmanagerconstants.h"
#include "qt4buildconfiguration.h"
#include "qt4nodes.h"
#include "qt4project.h"
#include "abldparser.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/task.h>

#include <QtCore/QDir>
#include <QtCore/QTimer>

using namespace Qt4ProjectManager::Internal;

namespace {
    const char * const SIGN_BS_ID = "Qt4ProjectManager.S60SignBuildStep";
    const char * const SIGNMODE_KEY("Qt4ProjectManager.S60CreatePackageStep.SignMode");
    const char * const CERTIFICATE_KEY("Qt4ProjectManager.S60CreatePackageStep.Certificate");
    const char * const KEYFILE_KEY("Qt4ProjectManager.S60CreatePackageStep.Keyfile");
    const char * const SMART_INSTALLER_KEY("Qt4ProjectManager.S60CreatorPackageStep.SmartInstaller");
}

S60CreatePackageStep::S60CreatePackageStep(ProjectExplorer::BuildConfiguration *bc) :
    BuildStep(bc, QLatin1String(SIGN_BS_ID)),
    m_signingMode(SignSelf),
    m_createSmartInstaller(false),
    m_outputParserChain(0),
    m_process(0),
    m_timer(0),
    m_eventLoop(0),
    m_futureInterface(0)
{
    ctor_package();
}

S60CreatePackageStep::S60CreatePackageStep(ProjectExplorer::BuildConfiguration *bc, S60CreatePackageStep *bs) :
    BuildStep(bc, bs),
    m_signingMode(bs->m_signingMode),
    m_customSignaturePath(bs->m_customSignaturePath),
    m_customKeyPath(bs->m_customKeyPath),
    m_createSmartInstaller(bs->m_createSmartInstaller),
    m_outputParserChain(0),
    m_timer(0),
    m_eventLoop(0),
    m_futureInterface(0)
{
    ctor_package();
}

S60CreatePackageStep::S60CreatePackageStep(ProjectExplorer::BuildConfiguration *bc, const QString &id) :
    BuildStep(bc, id),
    m_signingMode(SignSelf),
    m_createSmartInstaller(false),
    m_outputParserChain(0),
    m_timer(0),
    m_eventLoop(0),
    m_futureInterface(0)
{
    ctor_package();
}

void S60CreatePackageStep::ctor_package()
{
    setDisplayName(tr("Create SIS Package", "Create SIS package build step name"));
}

S60CreatePackageStep::~S60CreatePackageStep()
{
}

QVariantMap S60CreatePackageStep::toMap() const
{
    QVariantMap map(BuildStep::toMap());
    map.insert(QLatin1String(SIGNMODE_KEY), (int)m_signingMode);
    map.insert(QLatin1String(CERTIFICATE_KEY), m_customSignaturePath);
    map.insert(QLatin1String(KEYFILE_KEY), m_customKeyPath);
    map.insert(QLatin1String(SMART_INSTALLER_KEY), m_createSmartInstaller);
    return map;
}

bool S60CreatePackageStep::fromMap(const QVariantMap &map)
{
    m_signingMode = (SigningMode)map.value(QLatin1String(SIGNMODE_KEY)).toInt();
    m_customSignaturePath = map.value(QLatin1String(CERTIFICATE_KEY)).toString();
    m_customKeyPath = map.value(QLatin1String(KEYFILE_KEY)).toString();
    m_createSmartInstaller = map.value(QLatin1String(SMART_INSTALLER_KEY), false).toBool();
    return BuildStep::fromMap(map);
}

Qt4BuildConfiguration *S60CreatePackageStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
}

bool S60CreatePackageStep::init()
{
    Qt4Project *pro = qobject_cast<Qt4Project *>(buildConfiguration()->target()->project());

    QList<Qt4ProFileNode *> nodes = pro->leafProFiles();

    m_workingDirectories.clear();
    foreach (Qt4ProFileNode *node, nodes)
        m_workingDirectories << node->buildDir();

    m_makeCmd = qt4BuildConfiguration()->makeCommand();
    if (!QFileInfo(m_makeCmd).isAbsolute()) {
        // Try to detect command in environment
        const QString tmp = buildConfiguration()->environment().searchInPath(m_makeCmd);
        if (tmp.isEmpty()) {
            emit addOutput(tr("Could not find make command: %1 in the build environment").arg(m_makeCmd), BuildStep::ErrorOutput);
            return false;
        }
        m_makeCmd = tmp;
    }

    m_environment = qt4BuildConfiguration()->environment();

    m_args.clear();
    if (m_createSmartInstaller)
        m_args << QLatin1String("installer_sis");
    else
        m_args << QLatin1String("sis");
    if (signingMode() == SignCustom) {
        m_args << QLatin1String("QT_SIS_CERTIFICATE=") + QDir::toNativeSeparators(customSignaturePath())
               << QLatin1String("QT_SIS_KEY=") + QDir::toNativeSeparators(customKeyPath());
    }

    delete m_outputParserChain;
    m_outputParserChain = new ProjectExplorer::GnuMakeParser;
    m_outputParserChain->appendOutputParser(new Qt4ProjectManager::AbldParser);
    connect(m_outputParserChain, SIGNAL(addOutput(QString, ProjectExplorer::BuildStep::OutputFormat)),
            this, SLOT(outputAdded(QString, ProjectExplorer::BuildStep::OutputFormat)));
    connect(m_outputParserChain, SIGNAL(addTask(ProjectExplorer::Task)),
            this, SLOT(taskAdded(ProjectExplorer::Task)));

    return true;
}

void S60CreatePackageStep::run(QFutureInterface<bool> &fi)
{
    m_futureInterface = &fi;

    if (m_workingDirectories.isEmpty()) {
        fi.reportResult(true);
        return;
    }

    // Setup everything...
    m_process = new QProcess();
    m_process->setEnvironment(m_environment.toStringList());

    connect(m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(processReadyReadStdOutput()),
            Qt::DirectConnection);
    connect(m_process, SIGNAL(readyReadStandardError()),
            this, SLOT(processReadyReadStdError()),
            Qt::DirectConnection);

    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(slotProcessFinished(int, QProcess::ExitStatus)),
            Qt::DirectConnection);

    m_timer = new QTimer();
    connect(m_timer, SIGNAL(timeout()), this, SLOT(checkForCancel()), Qt::DirectConnection);
    m_timer->start(500);
    m_eventLoop = new QEventLoop;

    bool returnValue = false;
    if (startProcess())
        returnValue = m_eventLoop->exec();

    // Finished
    m_timer->stop();
    delete m_timer;
    m_timer = 0;

    delete m_process;
    m_process = 0;
    delete m_eventLoop;
    m_eventLoop = 0;
    fi.reportResult(returnValue);
    m_futureInterface = 0;

    return;
}

void S60CreatePackageStep::slotProcessFinished(int, QProcess::ExitStatus)
{
    QString line = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (!line.isEmpty())
        stdError(line);

    line = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (!line.isEmpty())
        stdOutput(line);

    bool returnValue = false;
    if (m_process->exitStatus() == QProcess::NormalExit && m_process->exitCode() == 0) {
        emit addOutput(tr("The process \"%1\" exited normally.")
                       .arg(QDir::toNativeSeparators(m_makeCmd)),
                       BuildStep::MessageOutput);
        returnValue = true;
    } else if (m_process->exitStatus() == QProcess::NormalExit) {
        emit addOutput(tr("The process \"%1\" exited with code %2.")
                       .arg(QDir::toNativeSeparators(m_makeCmd), QString::number(m_process->exitCode())),
                       BuildStep::ErrorMessageOutput);
    } else {
        emit addOutput(tr("The process \"%1\" crashed.").arg(QDir::toNativeSeparators(m_makeCmd)), BuildStep::ErrorMessageOutput);
    }

    if (m_workingDirectories.isEmpty() || !returnValue) {
        m_eventLoop->exit(returnValue);
    } else {
        // Success, do next
        if (!startProcess())
            m_eventLoop->exit(returnValue);
    }
}

bool S60CreatePackageStep::startProcess()
{
    QString workingDirectory = m_workingDirectories.takeFirst();
    QDir wd(workingDirectory);
    if (!wd.exists())
        wd.mkpath(wd.absolutePath());

    m_process->setWorkingDirectory(workingDirectory);

    // Go for it!
    m_process->start(m_makeCmd, m_args);
    if (!m_process->waitForStarted()) {
        emit addOutput(tr("Could not start process \"%1\" in %2")
                       .arg(QDir::toNativeSeparators(m_makeCmd),
                            workingDirectory),
                       BuildStep::ErrorMessageOutput);
        return false;
    }
    emit addOutput(tr("Starting: \"%1\" %2 in %3\n")
                   .arg(QDir::toNativeSeparators(m_makeCmd),
                        m_args.join(" "),
                        workingDirectory),
                   BuildStep::MessageOutput);
    return true;
}

void S60CreatePackageStep::processReadyReadStdOutput()
{
    m_process->setReadChannel(QProcess::StandardOutput);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
        stdOutput(line);
    }
}

void S60CreatePackageStep::stdOutput(const QString &line)
{
    if (m_outputParserChain)
        m_outputParserChain->stdOutput(line);
    emit addOutput(line, BuildStep::NormalOutput);
}

void S60CreatePackageStep::processReadyReadStdError()
{
    m_process->setReadChannel(QProcess::StandardError);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
        stdError(line);
    }
}

void S60CreatePackageStep::stdError(const QString &line)
{
    if (m_outputParserChain)
        m_outputParserChain->stdError(line);
    emit addOutput(line, BuildStep::ErrorOutput);
}

void S60CreatePackageStep::checkForCancel()
{
    if (m_futureInterface->isCanceled() && m_timer->isActive()) {
        m_timer->stop();
        m_process->terminate();
        m_process->waitForFinished(5000);
        m_process->kill();
    }
}

void S60CreatePackageStep::taskAdded(const ProjectExplorer::Task &task)
{
    ProjectExplorer::Task editable(task);
    QString filePath = QDir::cleanPath(task.file.trimmed());
    if (!filePath.isEmpty() && !QDir::isAbsolutePath(filePath)) {
        // TODO which kind of tasks do we get from package building?
        // No absoulte path
    }
    emit addTask(editable);
}

void S60CreatePackageStep::outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format)
{
    emit addOutput(string, format);
}


bool S60CreatePackageStep::immutable() const
{
    return false;
}

ProjectExplorer::BuildStepConfigWidget *S60CreatePackageStep::createConfigWidget()
{
    return new S60CreatePackageStepConfigWidget(this);
}

S60CreatePackageStep::SigningMode S60CreatePackageStep::signingMode() const
{
    return m_signingMode;
}

void S60CreatePackageStep::setSigningMode(SigningMode mode)
{
    m_signingMode = mode;
}

QString S60CreatePackageStep::customSignaturePath() const
{
    return m_customSignaturePath;
}

void S60CreatePackageStep::setCustomSignaturePath(const QString &path)
{
    m_customSignaturePath = path;
}

QString S60CreatePackageStep::customKeyPath() const
{
    return m_customKeyPath;
}

void S60CreatePackageStep::setCustomKeyPath(const QString &path)
{
    m_customKeyPath = path;
}

bool S60CreatePackageStep::createsSmartInstaller() const
{
    return m_createSmartInstaller;
}

void S60CreatePackageStep::setCreatesSmartInstaller(bool value)
{
    m_createSmartInstaller = value;
    static_cast<Qt4BuildConfiguration *>(buildConfiguration())->emitS60CreatesSmartInstallerChanged();
}

// #pragma mark -- S60SignBuildStepFactory

S60CreatePackageStepFactory::S60CreatePackageStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

S60CreatePackageStepFactory::~S60CreatePackageStepFactory()
{
}

bool S60CreatePackageStepFactory::canCreate(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, const QString &id) const
{
    if (type != ProjectExplorer::BuildStep::Deploy)
        return false;
    if (parent->target()->id() != Constants::S60_DEVICE_TARGET_ID)
        return false;
    return (id == QLatin1String(SIGN_BS_ID));
}

ProjectExplorer::BuildStep *S60CreatePackageStepFactory::create(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, const QString &id)
{
    if (!canCreate(parent, type, id))
        return 0;
    return new S60CreatePackageStep(parent);
}

bool S60CreatePackageStepFactory::canClone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, ProjectExplorer::BuildStep *source) const
{
    return canCreate(parent, type, source->id());
}

ProjectExplorer::BuildStep *S60CreatePackageStepFactory::clone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, type, source))
        return 0;
    return new S60CreatePackageStep(parent, static_cast<S60CreatePackageStep *>(source));
}

bool S60CreatePackageStepFactory::canRestore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, type, id);
}

ProjectExplorer::BuildStep *S60CreatePackageStepFactory::restore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, const QVariantMap &map)
{
    if (!canRestore(parent, type, map))
        return 0;
    S60CreatePackageStep *bs(new S60CreatePackageStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QStringList S60CreatePackageStepFactory::availableCreationIds(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type) const
{
    if (type != ProjectExplorer::BuildStep::Deploy)
        return QStringList();
    if (parent->target()->id() == Constants::S60_DEVICE_TARGET_ID)
        return QStringList() << QLatin1String(SIGN_BS_ID);
    return QStringList();
}

QString S60CreatePackageStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(SIGN_BS_ID))
        return tr("Create SIS Package");
    return QString();
}

// #pragma mark -- S60SignBuildStepConfigWidget

S60CreatePackageStepConfigWidget::S60CreatePackageStepConfigWidget(S60CreatePackageStep *signStep)
    : BuildStepConfigWidget(), m_signStep(signStep)
{
    m_ui.setupUi(this);
    m_ui.signaturePath->setExpectedKind(Utils::PathChooser::File);
    m_ui.keyFilePath->setExpectedKind(Utils::PathChooser::File);
    updateUi();
    connect(m_ui.customCertificateButton, SIGNAL(clicked()),
            this, SLOT(updateFromUi()));
    connect(m_ui.selfSignedButton, SIGNAL(clicked()),
            this, SLOT(updateFromUi()));
    connect(m_ui.signaturePath, SIGNAL(changed(QString)),
            this, SLOT(updateFromUi()));
    connect(m_ui.keyFilePath, SIGNAL(changed(QString)),
            this, SLOT(updateFromUi()));
    connect(m_ui.smartInstaller, SIGNAL(clicked()),
            this, SLOT(updateFromUi()));
}

void S60CreatePackageStepConfigWidget::updateUi()
{
    bool selfSigned = m_signStep->signingMode() == S60CreatePackageStep::SignSelf;
    m_ui.selfSignedButton->setChecked(selfSigned);
    m_ui.customCertificateButton->setChecked(!selfSigned);
    m_ui.signaturePath->setEnabled(!selfSigned);
    m_ui.keyFilePath->setEnabled(!selfSigned);
    m_ui.signaturePath->setPath(m_signStep->customSignaturePath());
    m_ui.keyFilePath->setPath(m_signStep->customKeyPath());
    m_ui.smartInstaller->setChecked(m_signStep->createsSmartInstaller());
    emit updateSummary();
}

void S60CreatePackageStepConfigWidget::updateFromUi()
{
    bool selfSigned = m_ui.selfSignedButton->isChecked();
    m_signStep->setSigningMode(selfSigned ? S60CreatePackageStep::SignSelf
        : S60CreatePackageStep::SignCustom);
    m_signStep->setCustomSignaturePath(m_ui.signaturePath->path());
    m_signStep->setCustomKeyPath(m_ui.keyFilePath->path());
    m_signStep->setCreatesSmartInstaller(m_ui.smartInstaller->isChecked());
    updateUi();
}

QString S60CreatePackageStepConfigWidget::summaryText() const
{
    QString text;
    if (m_signStep->signingMode() == S60CreatePackageStep::SignSelf) {
        text = tr("self-signed");
    } else {
        text = tr("signed with certificate %1 and key file %2")
               .arg(m_signStep->customSignaturePath())
               .arg(m_signStep->customKeyPath());
    }
    if (m_signStep->createsSmartInstaller())
        return tr("<b>Create SIS Package:</b> %1, using Smart Installer").arg(text);
    return tr("<b>Create SIS Package:</b> %1").arg(text);
}

QString S60CreatePackageStepConfigWidget::displayName() const
{
    return m_signStep->displayName();
}

void S60CreatePackageStepConfigWidget::init()
{
}
