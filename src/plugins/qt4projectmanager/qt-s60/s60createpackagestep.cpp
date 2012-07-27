/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "s60createpackagestep.h"

#include "qt4projectmanagerconstants.h"
#include "qt4buildconfiguration.h"
#include "qt4nodes.h"
#include "qt4project.h"
#include "s60createpackageparser.h"
#include "abldparser.h"
#include "sbsv2parser.h"
#include "passphraseforkeydialog.h"
#include "s60certificateinfo.h"
#include "s60certificatedetailsdialog.h"
#include "symbianqtversion.h"
#include "symbianidevicefactory.h"

#include <app/app_version.h>

#include <utils/checkablemessagebox.h>
#include <utils/fileutils.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/profileinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/task.h>

#include <qtsupport/qtprofileinformation.h>

#include <QDir>
#include <QTimer>
#include <QCryptographicHash>

#include <QSettings>
#include <QMessageBox>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
    const char * const SIGN_BS_ID = "Qt4ProjectManager.S60SignBuildStep";
    const char * const SIGNMODE_KEY("Qt4ProjectManager.S60CreatePackageStep.SignMode");
    const char * const CERTIFICATE_KEY("Qt4ProjectManager.S60CreatePackageStep.Certificate");
    const char * const KEYFILE_KEY("Qt4ProjectManager.S60CreatePackageStep.Keyfile");
    const char * const SMART_INSTALLER_KEY("Qt4ProjectManager.S60CreatorPackageStep.SmartInstaller");
    const char * const PATCH_WARNING_SHOWN_KEY("Qt4ProjectManager.S60CreatorPackageStep.PatchWarningShown");
    const char * const SUPPRESS_PATCH_WARNING_DIALOG_KEY("Qt4ProjectManager.S60CreatorPackageStep.SuppressPatchWarningDialog");

    const char * const MAKE_PASSPHRASE_ARGUMENT("QT_SIS_PASSPHRASE=");
    const char * const MAKE_KEY_ARGUMENT("QT_SIS_KEY=");
    const char * const MAKE_CERTIFICATE_ARGUMENT("QT_SIS_CERTIFICATE=");
}

S60CreatePackageStep::S60CreatePackageStep(ProjectExplorer::BuildStepList *bsl) :
    BuildStep(bsl, Core::Id(SIGN_BS_ID)),
    m_signingMode(SignSelf),
    m_createSmartInstaller(false),
    m_outputParserChain(0),
    m_process(0),
    m_timer(0),
    m_eventLoop(0),
    m_futureInterface(0),
    m_passphrases(0),
    m_parser(0),
    m_suppressPatchWarningDialog(false),
    m_patchWarningDialog(0)
{
    ctor_package();
}

S60CreatePackageStep::S60CreatePackageStep(ProjectExplorer::BuildStepList *bsl, S60CreatePackageStep *bs) :
    BuildStep(bsl, bs),
    m_signingMode(bs->m_signingMode),
    m_customSignaturePath(bs->m_customSignaturePath),
    m_customKeyPath(bs->m_customKeyPath),
    m_passphrase(bs->m_passphrase),
    m_createSmartInstaller(bs->m_createSmartInstaller),
    m_outputParserChain(0),
    m_timer(0),
    m_eventLoop(0),
    m_futureInterface(0),
    m_passphrases(0),
    m_parser(0),
    m_suppressPatchWarningDialog(false),
    m_patchWarningDialog(0)
{
    ctor_package();
}

S60CreatePackageStep::S60CreatePackageStep(ProjectExplorer::BuildStepList *bsl, const Core::Id id) :
    BuildStep(bsl, id),
    m_signingMode(SignSelf),
    m_createSmartInstaller(false),
    m_outputParserChain(0),
    m_timer(0),
    m_eventLoop(0),
    m_futureInterface(0),
    m_passphrases(0),
    m_parser(0),
    m_suppressPatchWarningDialog(false),
    m_patchWarningDialog(0)
{
    ctor_package();
}

void S60CreatePackageStep::ctor_package()
{
    //: default create SIS package build step display name
    setDefaultDisplayName(tr("Create SIS Package"));
    connect(this, SIGNAL(badPassphrase()),
            this, SLOT(definePassphrase()), Qt::QueuedConnection);
    connect(this, SIGNAL(warnAboutPatching()),
            this, SLOT(handleWarnAboutPatching()), Qt::QueuedConnection);

    m_passphrases = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                                  QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
                                  QLatin1String("QtCreatorKeys"), this);
}

S60CreatePackageStep::~S60CreatePackageStep()
{
    delete m_patchWarningDialog;
}

QVariantMap S60CreatePackageStep::toMap() const
{
    QVariantMap map(BuildStep::toMap());
    map.insert(QLatin1String(SIGNMODE_KEY), static_cast<int>(m_signingMode));
    map.insert(QLatin1String(CERTIFICATE_KEY), m_customSignaturePath);
    map.insert(QLatin1String(KEYFILE_KEY), m_customKeyPath);
    map.insert(QLatin1String(SMART_INSTALLER_KEY), m_createSmartInstaller);
    map.insert(QLatin1String(SUPPRESS_PATCH_WARNING_DIALOG_KEY), m_suppressPatchWarningDialog);
    return map;
}

bool S60CreatePackageStep::fromMap(const QVariantMap &map)
{
    m_signingMode = static_cast<SigningMode>(map.value(QLatin1String(SIGNMODE_KEY), static_cast<int>(SignSelf)).toInt());
    m_customSignaturePath = map.value(QLatin1String(CERTIFICATE_KEY)).toString();
    setCustomKeyPath(map.value(QLatin1String(KEYFILE_KEY)).toString());
    m_createSmartInstaller = map.value(QLatin1String(SMART_INSTALLER_KEY), false).toBool();
    m_suppressPatchWarningDialog = map.value(QLatin1String(SUPPRESS_PATCH_WARNING_DIALOG_KEY),
                                             false).toBool();
    return BuildStep::fromMap(map);
}

Qt4BuildConfiguration *S60CreatePackageStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration());
}

bool S60CreatePackageStep::init()
{
    Qt4Project *pro = qobject_cast<Qt4Project *>(project());
    ProjectExplorer::ToolChain *tc
            = ProjectExplorer::ToolChainProfileInformation::toolChain(target()->profile());

    QList<Qt4ProFileNode *> nodes = pro->allProFiles();

    SymbianQtVersion *sqv
            = dynamic_cast<SymbianQtVersion *>(QtSupport::QtProfileInformation::qtVersion(target()->profile()));
    if (!sqv) {
        emit addOutput(tr("The selected target is not configured with a Symbian Qt"), BuildStep::ErrorOutput);
        return false;
    }
    if (!tc) {
        emit addOutput(ProjectExplorer::ToolChainProfileInformation::msgNoToolChainInTarget(), BuildStep::ErrorOutput);
        return false;
    }
    m_isBuildWithSymbianSbsV2 = sqv->isBuildWithSymbianSbsV2();

    m_workingDirectories.clear();
    QStringList projectCapabilities;
    foreach (Qt4ProFileNode *node, nodes) {
        projectCapabilities += node->symbianCapabilities();
        m_workingDirectories << node->buildDir();
    }
    projectCapabilities.removeDuplicates();

    m_makeCmd = tc->makeCommand();
    if (!QFileInfo(m_makeCmd).isAbsolute()) {
        // Try to detect command in environment
        const QString tmp = qt4BuildConfiguration()->environment().searchInPath(m_makeCmd);
        if (tmp.isEmpty()) {
            emit addOutput(tr("Could not find make command '%1' in the build environment").arg(m_makeCmd), BuildStep::ErrorOutput);
            return false;
        }
        m_makeCmd = tmp;
    }

    if (signingMode() == SignCustom && !validateCustomSigningResources(projectCapabilities))
        return false;

    m_environment = qt4BuildConfiguration()->environment();

    m_cancel = false;

    return true;
}

void S60CreatePackageStep::definePassphrase()
{
    Q_ASSERT(!m_cancel);
    PassphraseForKeyDialog *passwordDialog
            = new PassphraseForKeyDialog(QFileInfo(customKeyPath()).fileName());
    if (passwordDialog->exec()) {
        QString newPassphrase = passwordDialog->passphrase();
        setPassphrase(newPassphrase);
        if (passwordDialog->savePassphrase())
            savePassphraseForKey(m_keyId, newPassphrase);
    } else {
        m_cancel = true;
    }
    delete passwordDialog;

    m_waitCondition.wakeAll();
}

void S60CreatePackageStep::packageWasPatched(const QString &package, const QStringList &changes)
{
    m_packageChanges.append(qMakePair(package, changes));
}

void S60CreatePackageStep::handleWarnAboutPatching()
{
    if (!m_suppressPatchWarningDialog && !m_packageChanges.isEmpty()) {
        if (m_patchWarningDialog){
            m_patchWarningDialog->raise();
            return;
        }

        m_patchWarningDialog = new Utils::CheckableMessageBox(0);
        connect(m_patchWarningDialog, SIGNAL(finished(int)), this, SLOT(packageWarningDialogDone()));

        QString title;
        QString changedText;
        const QString url = QString::fromLatin1("qthelp://com.nokia.qtcreator.%1%2%3/doc/creator-run-settings.html#capabilities-and-signing").
                arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR).arg(IDE_VERSION_RELEASE);
        if (m_packageChanges.count() == 1) {
            title = tr("Package Modified");
            changedText = tr("<p>Qt modified your package <b>%1</b>.</p>").arg(m_packageChanges.at(0).first);
        } else {
            title = tr("Packages Modified");
            changedText = tr("<p>Qt modified some of your packages.</p>");
        }
        const QString text =
            tr("%1<p><em>These changes were not part of your build profile</em> but are required to "
               "make sure the <em>self-signed</em> package can be installed successfully on a device.</p>"
               "<p>Check the Issues pane for more details on the modifications made.</p>"
               "<p>Please see the <a href=\"%2\">documentation</a> for other signing options which "
               "remove the need for this patching.</p>").arg(changedText, url);
        m_patchWarningDialog->setWindowTitle(title);
        m_patchWarningDialog->setText(text);
        m_patchWarningDialog->setCheckBoxText(tr("Ignore patching for this packaging step."));
        m_patchWarningDialog->setIconPixmap(QMessageBox::standardIcon(QMessageBox::Warning));
        m_patchWarningDialog->setChecked(m_suppressPatchWarningDialog);
        m_patchWarningDialog->setStandardButtons(QDialogButtonBox::Ok);
        m_patchWarningDialog->open();
    }
}

void S60CreatePackageStep::savePassphraseForKey(const QString &keyId, const QString &passphrase)
{
    m_passphrases->beginGroup(QLatin1String("keys"));
    if (passphrase.isEmpty())
        m_passphrases->remove(keyId);
    else
        m_passphrases->setValue(keyId, obfuscatePassphrase(passphrase, keyId));
    m_passphrases->endGroup();
}

QString S60CreatePackageStep::loadPassphraseForKey(const QString &keyId)
{
    if (keyId.isEmpty())
        return QString();
    m_passphrases->beginGroup(QLatin1String("keys"));
    QString passphrase = elucidatePassphrase(m_passphrases->value(keyId, QByteArray()).toByteArray(), keyId);
    m_passphrases->endGroup();
    return passphrase;
}

QByteArray S60CreatePackageStep::obfuscatePassphrase(const QString &passphrase, const QString &key) const
{
    QByteArray byteArray = passphrase.toUtf8();
    char *data = byteArray.data();
    const QChar *keyData = key.data();
    int keyDataSize = key.size();
    for (int i = 0; i <byteArray.size(); ++i)
        data[i] = data[i]^keyData[i%keyDataSize].toAscii();
    return byteArray.toBase64();
}

QString S60CreatePackageStep::elucidatePassphrase(QByteArray obfuscatedPassphrase, const QString &key) const
{
    QByteArray byteArray = QByteArray::fromBase64(obfuscatedPassphrase);
    if (byteArray.isEmpty())
        return QString();

    char *data = byteArray.data();
    const QChar *keyData = key.data();
    int keyDataSize = key.size();
    for (int i = 0; i < byteArray.size(); ++i)
        data[i] = data[i]^keyData[i%keyDataSize].toAscii();
    return QString::fromUtf8(byteArray.data());
}

void S60CreatePackageStep::run(QFutureInterface<bool> &fi)
{
    if (m_workingDirectories.isEmpty()) {
        fi.reportResult(true);
        return;
    }

    m_timer = new QTimer();
    connect(m_timer, SIGNAL(timeout()), this, SLOT(checkForCancel()), Qt::DirectConnection);
    m_timer->start(500);
    m_eventLoop = new QEventLoop;

    bool returnValue = false;
    if (!createOnePackage()) {
        fi.reportResult(false);
        return;
    }

    Q_ASSERT(!m_futureInterface);
    m_futureInterface = &fi;
    returnValue = m_eventLoop->exec();

    // Finished
    m_timer->stop();
    delete m_timer;
    m_timer = 0;

    delete m_process;
    m_process = 0;
    delete m_eventLoop;
    m_eventLoop = 0;

    m_futureInterface = 0;

    if (returnValue)
        emit warnAboutPatching();
    fi.reportResult(returnValue);
}

bool S60CreatePackageStep::createOnePackage()
{
    // Setup arguments:
    m_args.clear();
    if (m_createSmartInstaller) {
        if (signingMode() == NotSigned)
            m_args << QLatin1String("unsigned_installer_sis");
        else
            m_args << QLatin1String("installer_sis");
    } else if (signingMode() == NotSigned)
        m_args << QLatin1String("unsigned_sis");
    else
        m_args << QLatin1String("sis");

    if (signingMode() == SignCustom) {
        m_args << QLatin1String(MAKE_CERTIFICATE_ARGUMENT) + QDir::toNativeSeparators(customSignaturePath())
               << QLatin1String(MAKE_KEY_ARGUMENT) + QDir::toNativeSeparators(customKeyPath());

        setPassphrase(loadPassphraseForKey(m_keyId));

        if (!passphrase().isEmpty())
            m_args << QLatin1String(MAKE_PASSPHRASE_ARGUMENT) + passphrase();
    }

    // Setup working directory:
    QString workingDirectory = m_workingDirectories.first();
    QDir wd(workingDirectory);
    if (!wd.exists())
        wd.mkpath(wd.absolutePath());


    // Setup process...
    Q_ASSERT(!m_process);
    m_process = new QProcess();
    m_process->setEnvironment(m_environment.toStringList());

    connect(m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(processReadyReadStdOutput()),
            Qt::DirectConnection);
    connect(m_process, SIGNAL(readyReadStandardError()),
            this, SLOT(processReadyReadStdError()),
            Qt::DirectConnection);

    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(packageDone(int,QProcess::ExitStatus)),
            Qt::DirectConnection);

    m_process->setWorkingDirectory(wd.absolutePath());

    // Setup parsers:
    Q_ASSERT(!m_outputParserChain);
    if (!m_isBuildWithSymbianSbsV2) {
        m_outputParserChain = new Qt4ProjectManager::AbldParser;
        m_outputParserChain->appendOutputParser(new ProjectExplorer::GnuMakeParser);
    } else {
        m_outputParserChain = new ProjectExplorer::GnuMakeParser();
    }
    Q_ASSERT(!m_parser);
    m_parser = new S60CreatePackageParser(wd.absolutePath());
    m_outputParserChain->appendOutputParser(m_parser);
    m_outputParserChain->setWorkingDirectory(wd.absolutePath());

    connect(m_outputParserChain, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat)),
            this, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat)));
    connect(m_outputParserChain, SIGNAL(addTask(ProjectExplorer::Task)),
            this, SIGNAL(addTask(ProjectExplorer::Task)), Qt::DirectConnection);

    connect(m_parser, SIGNAL(packageWasPatched(QString,QStringList)),
            this, SLOT(packageWasPatched(QString,QStringList)), Qt::DirectConnection);

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
                        m_args.join(QLatin1String(" ")),
                        workingDirectory),
                   BuildStep::MessageOutput);
    return true;
}

bool S60CreatePackageStep::validateCustomSigningResources(const QStringList &capabilitiesInPro)
{
    Q_ASSERT(signingMode() == SignCustom);

    QString errorString;
    if (customSignaturePath().isEmpty())
        errorString = tr("No certificate file specified. Please specify one in the project settings.");
    else if (!QFileInfo(customSignaturePath()).exists())
        errorString = tr("Certificate file \"%1\" does not exist. "
                         "Please specify an existing certificate file in the project settings.").arg(customSignaturePath());

    if (customKeyPath().isEmpty())
        errorString = tr("No key file specified. Please specify one in the project settings.");
    else if (!QFileInfo(customKeyPath()).exists())
        errorString = tr("Key file \"%1\" does not exist. "
                         "Please specify an existing key file in the project settings.").arg(customKeyPath());

    if (!errorString.isEmpty()) {
        reportPackageStepIssue(errorString, true);
        return false;
    }
    QScopedPointer<S60CertificateInfo> certInfoPtr(new S60CertificateInfo(customSignaturePath()));
    S60CertificateInfo::CertificateState certState = certInfoPtr.data()->validateCertificate();
    switch (certState) {
    case S60CertificateInfo::CertificateError:
        reportPackageStepIssue(certInfoPtr.data()->errorString(), true);
        return false;
    case S60CertificateInfo::CertificateWarning:
        reportPackageStepIssue(certInfoPtr.data()->errorString(), false);
        break;
    default:
        break;
    }

    QStringList unsupportedCaps;
    if (certInfoPtr.data()->compareCapabilities(capabilitiesInPro, unsupportedCaps)) {
        if (!unsupportedCaps.isEmpty()) {
            QString message = tr("The package created will not install on a "
                                 "device as some of the defined capabilities "
                                 "are not supported by the certificate: %1")
                    .arg(unsupportedCaps.join(QLatin1String(" ")));
            reportPackageStepIssue(message, true);
            return false;
        }

    } else
        reportPackageStepIssue(certInfoPtr.data()->errorString(), false);
    return true;
}

void S60CreatePackageStep::reportPackageStepIssue(const QString &message, bool isError )
{
    emit addOutput(message, isError?
                       BuildStep::ErrorMessageOutput:
                       BuildStep::MessageOutput);
    emit addTask(ProjectExplorer::Task(isError?
                                           ProjectExplorer::Task::Error:
                                           ProjectExplorer::Task::Warning,
                                       message,
                                       Utils::FileName(), -1,
                                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
}

void S60CreatePackageStep::packageWarningDialogDone()
{
    if (m_patchWarningDialog)
        m_suppressPatchWarningDialog = m_patchWarningDialog->isChecked();
    if (m_suppressPatchWarningDialog) {
        m_patchWarningDialog->deleteLater();
        m_patchWarningDialog = 0;
    }
}

void S60CreatePackageStep::packageDone(int exitCode, QProcess::ExitStatus status)
{
    QString line = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (!line.isEmpty())
        stdError(line);

    line = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (!line.isEmpty())
        stdOutput(line);

    if (status == QProcess::NormalExit && exitCode == 0) {
        emit addOutput(tr("The process \"%1\" exited normally.")
                       .arg(QDir::toNativeSeparators(m_makeCmd)),
                       BuildStep::MessageOutput);
    } else if (status == QProcess::NormalExit) {
        emit addOutput(tr("The process \"%1\" exited with code %2.")
                       .arg(QDir::toNativeSeparators(m_makeCmd), QString::number(exitCode)),
                       BuildStep::ErrorMessageOutput);
    } else {
        emit addOutput(tr("The process \"%1\" crashed.").arg(QDir::toNativeSeparators(m_makeCmd)), BuildStep::ErrorMessageOutput);
    }

    bool needPassphrase = m_parser->needPassphrase();

    // Clean up:
    delete m_outputParserChain;
    m_outputParserChain = 0;
    m_parser = 0;
    delete m_process;
    m_process = 0;

    // Process next directories:
    if (needPassphrase) {
        emit badPassphrase();
        QMutexLocker locker(&m_mutex);
        m_waitCondition.wait(&m_mutex);
    } else {
        if (status != QProcess::NormalExit || exitCode != 0) {
            m_eventLoop->exit(false);
            return;
        }

        m_workingDirectories.removeFirst();
        if (m_workingDirectories.isEmpty()) {
            m_eventLoop->exit(true);
            return;
        }
    }

    if (m_cancel || !createOnePackage())
        m_eventLoop->exit(false);
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
    emit addOutput(line, BuildStep::NormalOutput, BuildStep::DontAppendNewline);
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
    emit addOutput(line, BuildStep::ErrorOutput, BuildStep::DontAppendNewline);
}

void S60CreatePackageStep::checkForCancel()
{
    if (m_futureInterface->isCanceled()
         && m_timer && m_timer->isActive()) {
        m_timer->stop();
        if (m_process) {
            m_process->terminate();
            m_process->waitForFinished(5000); //while waiting, the process can be killed
            if (m_process)
                m_process->kill();
        }
        if (m_eventLoop)
            m_eventLoop->exit(false);
    }
}

QString S60CreatePackageStep::generateKeyId(const QString &keyPath) const
{
    if (keyPath.isEmpty())
        return QString();

    Utils::FileReader reader;
    if (!reader.fetch(keyPath, QIODevice::Text)) {
        emit addOutput(reader.errorString(), BuildStep::ErrorOutput);
        return QString();
    }

    //key file is quite small in size
    return QLatin1String(QCryptographicHash::hash(reader.data(),
                                                  QCryptographicHash::Md5).toHex());
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
    m_keyId = generateKeyId(m_customKeyPath);
}

QString S60CreatePackageStep::passphrase() const
{
    return m_passphrase;
}

void S60CreatePackageStep::setPassphrase(const QString &passphrase)
{
    if (passphrase.isEmpty())
        return;
    m_passphrase = passphrase;
}

QString S60CreatePackageStep::keyId() const
{
    return m_keyId;
}

void S60CreatePackageStep::setKeyId(const QString &keyId)
{
    m_keyId = keyId;
}

bool S60CreatePackageStep::createsSmartInstaller() const
{
    return m_createSmartInstaller;
}

void S60CreatePackageStep::setCreatesSmartInstaller(bool value)
{
    m_createSmartInstaller = value;
    qt4BuildConfiguration()->emitS60CreatesSmartInstallerChanged();
}

void S60CreatePackageStep::resetPassphrases()
{
    m_passphrases->beginGroup(QLatin1String("keys"));
    QStringList keys = m_passphrases->allKeys();
    foreach (const QString &key, keys) {
        m_passphrases->setValue(key, QString());
    }
    m_passphrases->remove(QString());
    m_passphrases->endGroup();
}

// #pragma mark -- S60SignBuildStepFactory

S60CreatePackageStepFactory::S60CreatePackageStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

S60CreatePackageStepFactory::~S60CreatePackageStepFactory()
{
}

bool S60CreatePackageStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    return canHandle(parent) &&  id == Core::Id(SIGN_BS_ID);
}

ProjectExplorer::BuildStep *S60CreatePackageStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new S60CreatePackageStep(parent);
}

bool S60CreatePackageStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::BuildStep *S60CreatePackageStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new S60CreatePackageStep(parent, static_cast<S60CreatePackageStep *>(source));
}

bool S60CreatePackageStepFactory::canHandle(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() != Core::Id(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY))
        return false;
    Core::Id deviceType = ProjectExplorer::DeviceTypeProfileInformation::deviceTypeId(parent->target()->profile());
    if (deviceType != SymbianIDeviceFactory::deviceType())
        return false;
    return qobject_cast<Qt4Project *>(parent->target()->project());
}

bool S60CreatePackageStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *S60CreatePackageStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    S60CreatePackageStep *bs(new S60CreatePackageStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<Core::Id> S60CreatePackageStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();

    return QList<Core::Id>() << Core::Id(SIGN_BS_ID);
}

QString S60CreatePackageStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(SIGN_BS_ID))
        return tr("Create SIS Package");
    return QString();
}

// #pragma mark -- S60SignBuildStepConfigWidget

S60CreatePackageStepConfigWidget::S60CreatePackageStepConfigWidget(S60CreatePackageStep *signStep)
    : BuildStepConfigWidget(), m_signStep(signStep)
{
    m_ui.setupUi(this);
    m_ui.signaturePath->setExpectedKind(Utils::PathChooser::File);
    m_ui.signaturePath->setPromptDialogFilter(QLatin1String("*.cer *.crt *.der *.pem"));
    m_ui.keyFilePath->setExpectedKind(Utils::PathChooser::File);
    updateUi();

    bool enableCertDetails = m_signStep->signingMode() == S60CreatePackageStep::SignCustom
            && m_ui.signaturePath->isValid();
    m_ui.certificateDetails->setEnabled(enableCertDetails);

    connect(m_ui.certificateDetails, SIGNAL(clicked()),
            this, SLOT(displayCertificateDetails()));
    connect(m_ui.customCertificateButton, SIGNAL(clicked()),
            this, SLOT(updateFromUi()));
    connect(m_ui.selfSignedButton, SIGNAL(clicked()),
            this, SLOT(updateFromUi()));
    connect(m_ui.notSignedButton, SIGNAL(clicked()),
            this, SLOT(updateFromUi()));
    connect(m_ui.signaturePath, SIGNAL(changed(QString)),
            this, SLOT(signatureChanged(QString)));
    connect(m_ui.keyFilePath, SIGNAL(changed(QString)),
            this, SLOT(updateFromUi()));
    connect(m_ui.smartInstaller, SIGNAL(clicked()),
            this, SLOT(updateFromUi()));
    connect(m_ui.resetPassphrasesButton, SIGNAL(clicked()),
            this, SLOT(resetPassphrases()));
}

void S60CreatePackageStepConfigWidget::signatureChanged(QString certFile)
{
    m_ui.certificateDetails->setEnabled(m_ui.signaturePath->isValid());

    if (!certFile.isEmpty() && m_ui.keyFilePath->path().isEmpty()) {
        /*  If a cert file is selected and there is not key file inserted,
            then we check if there is a .key or .pem file in the folder with
            the same base name as the cert file. This file is probably a key
            file for this cert and the key field is then populated automatically
        */
        QFileInfo certFileInfo(certFile);
        QDir directory = QDir(certFileInfo.absolutePath());
        QString keyFile(certFileInfo.baseName() + QLatin1String(".key"));
        QString pemFile(certFileInfo.baseName() + QLatin1String(".pem"));
        QStringList files;
        QStringList keys;
        keys << keyFile << pemFile;
        files = directory.entryList(QStringList(keys),
                                    QDir::Files | QDir::NoSymLinks);

        if (files.isEmpty())
            m_ui.keyFilePath->setInitialBrowsePathBackup(certFileInfo.path());
        else
            m_ui.keyFilePath->setPath(directory.filePath(files[0]));
    }
    updateFromUi();
}

void S60CreatePackageStepConfigWidget::updateUi()
{
    switch(m_signStep->signingMode()) {
    case S60CreatePackageStep::SignCustom:
        m_ui.selfSignedButton->setChecked(false);
        m_ui.customCertificateButton->setChecked(true);
        m_ui.notSignedButton->setChecked(false);
        m_ui.certificateDetails->setEnabled(m_ui.signaturePath->isValid());
        break;
    case S60CreatePackageStep::NotSigned:
        m_ui.selfSignedButton->setChecked(false);
        m_ui.customCertificateButton->setChecked(false);
        m_ui.notSignedButton->setChecked(true);
        m_ui.certificateDetails->setEnabled(false);
        break;
    default:
        m_ui.selfSignedButton->setChecked(true);
        m_ui.customCertificateButton->setChecked(false);
        m_ui.notSignedButton->setChecked(false);
        m_ui.certificateDetails->setEnabled(false);
        break;
    }
    bool customSigned = m_signStep->signingMode() == S60CreatePackageStep::SignCustom;
    m_ui.signaturePath->setEnabled(customSigned);
    m_ui.keyFilePath->setEnabled(customSigned);
    m_ui.signaturePath->setPath(m_signStep->customSignaturePath());
    m_ui.keyFilePath->setPath(m_signStep->customKeyPath());
    m_ui.smartInstaller->setChecked(m_signStep->createsSmartInstaller());
    emit updateSummary();
}

void S60CreatePackageStepConfigWidget::updateFromUi()
{
    S60CreatePackageStep::SigningMode signingMode(S60CreatePackageStep::SignSelf);
    if (m_ui.selfSignedButton->isChecked())
        signingMode = S60CreatePackageStep::SignSelf;
    else if (m_ui.customCertificateButton->isChecked())
        signingMode = S60CreatePackageStep::SignCustom;
    else if (m_ui.notSignedButton->isChecked())
        signingMode = S60CreatePackageStep::NotSigned;

    m_signStep->setSigningMode(signingMode);
    m_signStep->setCustomSignaturePath(m_ui.signaturePath->path());
    m_signStep->setCustomKeyPath(m_ui.keyFilePath->path());
    m_signStep->setCreatesSmartInstaller(m_ui.smartInstaller->isChecked());
    updateUi();
}

void S60CreatePackageStepConfigWidget::displayCertificateDetails()
{
    S60CertificateInfo *certificateInformation = new S60CertificateInfo(m_ui.signaturePath->path());
    certificateInformation->devicesSupported().sort();

    S60CertificateDetailsDialog dialog;
    dialog.setText(certificateInformation->toHtml(false));
    dialog.exec();
    delete certificateInformation;
}

void S60CreatePackageStepConfigWidget::resetPassphrases()
{
    QMessageBox msgBox(QMessageBox::Question, tr("Reset Passphrases"),
                       tr("Do you want to reset all passphrases saved for keys used?"),
                       QMessageBox::Reset|QMessageBox::Cancel, this);
    if (msgBox.exec() == QMessageBox::Reset)
        m_signStep->resetPassphrases();
}

QString S60CreatePackageStepConfigWidget::summaryText() const
{
    QString text;
    switch(m_signStep->signingMode()) {
    case S60CreatePackageStep::SignCustom:
        if (!m_signStep->customSignaturePath().isEmpty()
                && !m_signStep->customKeyPath().isEmpty())
            text = tr("signed with the certificate \"%1\" using the key \"%2\"")
               .arg(QFileInfo(m_signStep->customSignaturePath()).fileName(),
                    QFileInfo(m_signStep->customKeyPath()).fileName());
        else
            text = tr("signed with a certificate and a key that need to be specified");
        break;
    case S60CreatePackageStep::NotSigned:
        text = tr("not signed");
        break;
    default:
        text = tr("self-signed");
        break;
    }
    if (m_signStep->createsSmartInstaller())
        return tr("<b>Create SIS Package:</b> %1, using Smart Installer").arg(text);
    return tr("<b>Create SIS Package:</b> %1").arg(text);
}

QString S60CreatePackageStepConfigWidget::displayName() const
{
    return m_signStep->displayName();
}
