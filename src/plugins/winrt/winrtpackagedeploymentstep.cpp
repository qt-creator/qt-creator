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

#include "winrtpackagedeploymentstep.h"

#include "winrtconstants.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deployablefile.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/fancylineedit.h>

#include <QLabel>
#include <QLayout>
#include <QRegularExpression>
#include <QToolButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace WinRt {
namespace Internal {

const char ARGUMENTS_KEY[] = "WinRt.BuildStep.Deploy.Arguments";
const char DEFAULTARGUMENTS_KEY[] = "WinRt.BuildStep.Deploy.DefaultArguments";

class WinRtArgumentsAspect final : public ProjectConfigurationAspect
{
    Q_DECLARE_TR_FUNCTIONS(WinRt::Internal::WinRtArgumentsAspect)

public:
    WinRtArgumentsAspect() = default;

    void addToLayout(LayoutBuilder &builder) final;

    void fromMap(const QVariantMap &map) final;
    void toMap(QVariantMap &map) const final;

    void setValue(const QString &value);
    QString value() const { return m_value; }

    void setDefaultValue(const QString &value) { m_defaultValue = value; }
    QString defaultValue() const { return m_defaultValue; }

    void restoreDefaultValue();

private:
    FancyLineEdit *m_lineEdit = nullptr;
    QString m_value;
    QString m_defaultValue;
};

class WinRtPackageDeploymentStep final : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(WinRt::Internal::WinRtPackageDeploymentStep)

public:
    WinRtPackageDeploymentStep(BuildStepList *bsl, Utils::Id id);

    QString defaultWinDeployQtArguments() const;

    void raiseError(const QString &errorMessage);
    void raiseWarning(const QString &warningMessage);

private:
    bool init() override;
    void doRun() override;
    bool processSucceeded(int exitCode, QProcess::ExitStatus status) override;
    void stdOutput(const QString &line) override;

    bool parseIconsAndExecutableFromManifest(QString manifestFileName, QStringList *items, QString *executable);

    WinRtArgumentsAspect *m_argsAspect = nullptr;
    QString m_targetFilePath;
    QString m_targetDirPath;
    QString m_executablePathInManifest;
    QString m_mappingFileContent;
    QString m_manifestFileName;
    bool m_createMappingFile = false;
};

void WinRtArgumentsAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(!m_lineEdit);
    auto label = new QLabel(tr("Arguments:"));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    builder.addItem(label);

    auto *layout = new QHBoxLayout();
    m_lineEdit = new Utils::FancyLineEdit();
    if (!m_value.isEmpty())
        m_lineEdit->setText(m_value);
    else if (!m_defaultValue.isEmpty())
        m_lineEdit->setText(m_defaultValue);
    connect(m_lineEdit, &Utils::FancyLineEdit::textEdited,
            this, &WinRtArgumentsAspect::setValue);
    layout->addWidget(m_lineEdit);

    auto restoreDefaultButton = new QToolButton();
    restoreDefaultButton->setText(tr("Restore Default Arguments"));
    connect(restoreDefaultButton, &QToolButton::clicked,
            this, &WinRtArgumentsAspect::restoreDefaultValue);
    layout->addWidget(restoreDefaultButton);
    builder.addItem(layout);
}

void WinRtArgumentsAspect::fromMap(const QVariantMap &map)
{
    m_defaultValue = map.value(DEFAULTARGUMENTS_KEY).toString();
    m_value = map.value(ARGUMENTS_KEY).toString();
}

void WinRtArgumentsAspect::toMap(QVariantMap &map) const
{
    map.insert(DEFAULTARGUMENTS_KEY, m_defaultValue);
    map.insert(ARGUMENTS_KEY, m_value);
}

void WinRtArgumentsAspect::setValue(const QString &value)
{
    if (value == m_value)
        return;

    m_value = value;
    if (m_lineEdit)
        m_lineEdit->setText(value);
    emit changed();
}

void WinRtArgumentsAspect::restoreDefaultValue()
{
    if (m_defaultValue == m_value)
        return;

    setValue(m_defaultValue);
}

WinRtPackageDeploymentStep::WinRtPackageDeploymentStep(BuildStepList *bsl, Utils::Id id)
    : AbstractProcessStep(bsl, id)
{
    setDisplayName(tr("Run windeployqt"));

    m_argsAspect = addAspect<WinRtArgumentsAspect>();
    m_argsAspect->setDefaultValue(defaultWinDeployQtArguments());
    m_argsAspect->setValue(defaultWinDeployQtArguments());
}

bool WinRtPackageDeploymentStep::init()
{
    RunConfiguration *rc = target()->activeRunConfiguration();
    QTC_ASSERT(rc, return false);

    const BuildTargetInfo bti = rc->buildTargetInfo();
    Utils::FilePath appTargetFilePath = bti.targetFilePath;

    m_targetFilePath = appTargetFilePath.toString();
    if (m_targetFilePath.isEmpty()) {
        raiseError(tr("No executable to deploy found in %1.").arg(bti.projectFilePath.toString()));
        return false;
    }

    // ### Ideally, the file paths in applicationTargets() should already have the .exe suffix.
    // Whenever this will eventually work, we can drop appending the .exe suffix here.
    if (!m_targetFilePath.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
        m_targetFilePath.append(QLatin1String(".exe"));

    m_targetDirPath = appTargetFilePath.parentDir().toString();
    if (!m_targetDirPath.endsWith(QLatin1Char('/')))
        m_targetDirPath += QLatin1Char('/');

    const QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(target()->kit());
    if (!qt)
        return false;

    const FilePath windeployqtPath = qt->hostBinPath().resolvePath("windeployqt.exe");

    CommandLine windeployqt{windeployqtPath};
    windeployqt.addArg(QDir::toNativeSeparators(m_targetFilePath));
    windeployqt.addArgs(m_argsAspect->value(), CommandLine::Raw);

    if (qt->type() == Constants::WINRT_WINPHONEQT) {
        m_createMappingFile = true;
        windeployqt.addArgs({"-list", "mapping"});
    }

    ProcessParameters *params = processParameters();
    if (!windeployqtPath.exists()) {
        raiseError(tr("Cannot find windeployqt.exe in \"%1\".")
                       .arg(QDir::toNativeSeparators(qt->hostBinPath().toString())));
        return false;
    }
    params->setCommandLine(windeployqt);
    params->setEnvironment(buildEnvironment());

    return AbstractProcessStep::init();
}

void WinRtPackageDeploymentStep::doRun()
{
    const QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(target()->kit());
    if (!qt)
        return;

    m_manifestFileName = QStringLiteral("AppxManifest");

    if (m_createMappingFile) {
        m_mappingFileContent = QLatin1String("[Files]\n");

        QDir assetDirectory(m_targetDirPath + QLatin1String("assets"));
        if (assetDirectory.exists()) {
            QStringList iconsToDeploy;
            const QString fullManifestPath = m_targetDirPath + m_manifestFileName
                    + QLatin1String(".xml");
            if (!parseIconsAndExecutableFromManifest(fullManifestPath, &iconsToDeploy,
                                                     &m_executablePathInManifest)) {
                raiseError(tr("Cannot parse manifest file %1.").arg(fullManifestPath));
                return;
            }
            foreach (const QString &icon, iconsToDeploy) {
                m_mappingFileContent += QLatin1Char('"')
                        + QDir::toNativeSeparators(m_targetDirPath + icon) + QLatin1String("\" \"")
                        + QDir::toNativeSeparators(icon) + QLatin1String("\"\n");
            }
        }
    }

    AbstractProcessStep::doRun();
}

bool WinRtPackageDeploymentStep::processSucceeded(int exitCode, QProcess::ExitStatus status)
{
    if (m_createMappingFile) {
        QString targetInstallationPath;
        // The list holds the local file paths and the "remote" file paths
        QList<QPair<QString, QString> > installableFilesList;
        foreach (DeployableFile file, target()->deploymentData().allFiles()) {
            QString remoteFilePath = file.remoteFilePath();
            while (remoteFilePath.startsWith(QLatin1Char('/')))
                remoteFilePath.remove(0, 1);
            QString localFilePath = file.localFilePath().toString();
            if (localFilePath == m_targetFilePath) {
                if (!m_targetFilePath.endsWith(QLatin1String(".exe"))) {
                    remoteFilePath += QLatin1String(".exe");
                    localFilePath += QLatin1String(".exe");
                }
                targetInstallationPath = remoteFilePath;
            }
            installableFilesList.append(QPair<QString, QString>(localFilePath, remoteFilePath));
        }

        // if there are no INSTALLS set we just deploy the files from windeployqt,
        // the icons referenced in the manifest file and the actual build target
        QString baseDir;
        if (targetInstallationPath.isEmpty()) {
            if (!m_targetFilePath.endsWith(QLatin1String(".exe")))
                m_targetFilePath.append(QLatin1String(".exe"));
            m_mappingFileContent
                    += QLatin1Char('"') + QDir::toNativeSeparators(m_targetFilePath)
                    + QLatin1String("\" \"")
                    + QDir::toNativeSeparators(m_executablePathInManifest) + QLatin1String("\"\n");
            baseDir = m_targetDirPath;
        } else {
            baseDir = targetInstallationPath.left(targetInstallationPath.lastIndexOf(QLatin1Char('/')) + 1);
        }

        typedef QPair<QString, QString> QStringPair;
        foreach (const QStringPair &pair, installableFilesList) {
            // For the mapping file we need the remote paths relative to the application's executable
            QString relativeRemotePath;
            if (QDir(pair.second).isRelative())
                relativeRemotePath = pair.second;
            else
                relativeRemotePath = QDir(baseDir).relativeFilePath(pair.second);

            if (QDir(relativeRemotePath).isAbsolute() || relativeRemotePath.startsWith(QLatin1String(".."))) {
                raiseWarning(tr("File %1 is outside of the executable's directory. These files cannot be installed.").arg(relativeRemotePath));
                continue;
            }

            m_mappingFileContent += QLatin1Char('"') + QDir::toNativeSeparators(pair.first)
                    + QLatin1String("\" \"") + QDir::toNativeSeparators(relativeRemotePath)
                    + QLatin1String("\"\n");
        }

        const QString mappingFilePath = m_targetDirPath + m_manifestFileName
                + QLatin1String(".map");
        QFile mappingFile(mappingFilePath);
        if (!mappingFile.open(QFile::WriteOnly | QFile::Text)) {
            raiseError(tr("Cannot open mapping file %1 for writing.").arg(mappingFilePath));
            return false;
        }
        mappingFile.write(m_mappingFileContent.toUtf8());
    }

    return AbstractProcessStep::processSucceeded(exitCode, status);
}

void WinRtPackageDeploymentStep::stdOutput(const QString &line)
{
    if (m_createMappingFile)
        m_mappingFileContent += line;
    AbstractProcessStep::stdOutput(line);
}

QString WinRtPackageDeploymentStep::defaultWinDeployQtArguments() const
{
    QString args;
    QtcProcess::addArg(&args, QStringLiteral("--qmldir"));
    QtcProcess::addArg(&args, project()->projectDirectory().toUserOutput());
    return args;
}

void WinRtPackageDeploymentStep::raiseError(const QString &errorMessage)
{
    emit addOutput(errorMessage, BuildStep::OutputFormat::ErrorMessage);
    emit addTask(DeploymentTask(Task::Error, errorMessage), 1);
}

void WinRtPackageDeploymentStep::raiseWarning(const QString &warningMessage)
{
    emit addOutput(warningMessage, BuildStep::OutputFormat::NormalMessage);
    emit addTask(DeploymentTask(Task::Warning, warningMessage), 1);
}

bool WinRtPackageDeploymentStep::parseIconsAndExecutableFromManifest(QString manifestFileName, QStringList *icons, QString *executable)
{
    if (!icons->isEmpty())
        icons->clear();
    QFile manifestFile(manifestFileName);
    if (!manifestFile.open(QFile::ReadOnly))
        return false;
    const QString contents = QString::fromUtf8(manifestFile.readAll());

    QRegularExpression iconPattern(QStringLiteral("[\\\\/a-zA-Z0-9_\\-\\!]*\\.(png|jpg|jpeg)"));
    QRegularExpressionMatchIterator iterator = iconPattern.globalMatch(contents);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        const QString icon = match.captured(0);
        icons->append(icon);
    }

    const QLatin1String executablePrefix(manifestFileName.contains(QLatin1String("AppxManifest")) ? "Executable=" : "ImagePath=");
    QRegularExpression executablePattern(executablePrefix + QStringLiteral("\"([a-zA-Z0-9_-]*\\.exe)\""));
    QRegularExpressionMatch match = executablePattern.match(contents);
    if (!match.hasMatch())
        return false;
    *executable = match.captured(1);

    return true;
}

// WinRtDeployStepFactory

WinRtDeployStepFactory::WinRtDeployStepFactory()
{
    registerStep<WinRtPackageDeploymentStep>(Constants::WINRT_BUILD_STEP_DEPLOY);
    setDisplayName(QCoreApplication::translate("WinRt::Internal::WinRtDeployStepFactory", "Run windeployqt"));
    setFlags(BuildStepInfo::Unclonable);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    setSupportedDeviceTypes({Constants::WINRT_DEVICE_TYPE_LOCAL,
                             Constants::WINRT_DEVICE_TYPE_EMULATOR,
                             Constants::WINRT_DEVICE_TYPE_PHONE});
    setRepeatable(false);
}

} // namespace Internal
} // namespace WinRt
