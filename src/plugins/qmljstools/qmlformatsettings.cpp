// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlformatsettings.h"

#include <utils/commandline.h>
#include <utils/qtcprocess.h>
#include <utils/temporaryfile.h>
#include <utils/processinterface.h>
#include <utils/filepath.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QTemporaryDir>
#include <QStandardPaths>
#include <QLoggingCategory>

using namespace QtSupport;
using namespace Utils;
using namespace ProjectExplorer;
namespace QmlJSTools {

static Q_LOGGING_CATEGORY(qmlformatlog, "qtc.qmljstools.qmlformat", QtWarningMsg)
class QmlFormatProcess : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QmlFormatProcess)
public:
    QmlFormatProcess();
    ~QmlFormatProcess();
    void setWorkingDirectory(const Utils::FilePath &workingDirectory);
    Utils::FilePath workingDirectory() const;

    void setCommandLine(const Utils::CommandLine &cmd);
    Utils::CommandLine cmd() const;

    void run();

private:

signals:
    void finished(Utils::ProcessResultData);
private:
    Utils::Process *m_process = nullptr;
    Utils::FilePath m_workingDirectory;
    Utils::CommandLine m_cmd;
    Utils::TemporaryFile m_logFile;
};

// TODO: Mostly borrowed from qmllsclientsettings.cpp
// Unify them.
void QmlFormatSettings::evaluateLatestQmlFormat()
{
    if (!QtVersionManager::isLoaded())
        return;

    const QtVersions versions = QtVersionManager::versions();
    FilePath latestQmlFormat;
    QVersionNumber latestVersion;
    int latestUniqueId = std::numeric_limits<int>::min();

    for (QtVersion *qtVersion : versions) {
        const QVersionNumber version = qtVersion->qtVersion();
        const int uniqueId = qtVersion->uniqueId();

        // note: break ties between qt kits of same versions by selecting the qt kit with the highest id
        if (std::tie(version, uniqueId) < std::tie(latestVersion, latestUniqueId))
            continue;

        const FilePath qmlformat
            = QmlJS::ModelManagerInterface::qmlformatForBinPath(qtVersion->hostBinPath(), version);
        if (!qmlformat.isExecutableFile())
            continue;

        latestVersion = version;
        latestQmlFormat = qmlformat;
        latestUniqueId = uniqueId;
    }

    if (m_latestQmlFormat != latestQmlFormat || m_latestVersion != latestVersion) {
        m_latestQmlFormat = latestQmlFormat;
        m_latestVersion = latestVersion;
        emit versionEvaluated();
    }
}

QmlFormatSettings::QmlFormatSettings()
{
    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsLoaded,
            this, &QmlFormatSettings::evaluateLatestQmlFormat);
    connect(this, &QmlFormatSettings::versionEvaluated, this, &QmlFormatSettings::generateQmlFormatIniContent);
}

QmlFormatSettings& QmlFormatSettings::instance()
{
    static QmlFormatSettings instance;
    return instance;
}

Utils::FilePath QmlFormatSettings::globalQmlFormatIniFile()
{
    return Utils::FilePath::fromString(
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/qmlformat.ini") ;
}

Utils::FilePath QmlFormatSettings::currentQmlFormatIniFile(const Utils::FilePath &path)
{
    Utils::FilePath dir = path.isDir() ? path : path.parentDir();
    const QString settingsFileName = ".qmlformat.ini";
    while (dir.exists()) {
        Utils::FilePath iniFile = dir.pathAppended(settingsFileName);
        if (iniFile.exists()) {
            return iniFile;
        }
        dir = dir.parentDir();
    }
    return globalQmlFormatIniFile();
}

Utils::FilePath QmlFormatSettings::latestQmlFormatPath() const
{
    return m_latestQmlFormat;
}

QVersionNumber QmlFormatSettings::latestQmlFormatVersion() const
{
    return m_latestVersion;
}

void QmlFormatSettings::generateQmlFormatIniContent()
{
    if (m_latestQmlFormat.isEmpty() || !m_latestQmlFormat.isExecutableFile()) {
        qCWarning(qmlformatlog) << "No executable set for QmlFormat";
        return;
    }
    std::shared_ptr<QTemporaryDir> tempDir = std::make_shared<QTemporaryDir>();
    Utils::CommandLine cmd(m_latestQmlFormat);
    cmd.addArg("--write-defaults");
    std::shared_ptr<QmlFormatProcess> qmlformat = std::make_shared<QmlFormatProcess>();
    qmlformat->setWorkingDirectory(Utils::FilePath::fromString(tempDir->path()));
    qmlformat->setCommandLine(cmd);

    connect(
        qmlformat.get(),
        &QmlFormatProcess::finished,
        [tempDir, qmlformat, this](Utils::ProcessResultData result) {
            Utils::FilePath qmlformatIniFile = Utils::FilePath::fromString(
                tempDir->filePath(".qmlformat.ini"));
            if (result.m_exitStatus == QProcess::NormalExit && result.m_exitCode == 0) {
                emit qmlformatIniCreated(qmlformatIniFile);
            } else {
                qCWarning(qmlformatlog) << "Failed to generate QmlFormat ini file:"
                           << qmlformat->cmd().executable();
            }
        });

    qmlformat->run();
}

QmlFormatProcess::QmlFormatProcess()
    : m_logFile("qmlformat.qtc.log")
{
    m_logFile.setAutoRemove(false);
    m_logFile.open();
    m_process = new Process;
    m_process->setProcessMode(ProcessMode::Writer);
    connect (m_process, &Process::done, [this] {
        emit finished(m_process->resultData());
    });

    connect(m_process, &Process::readyReadStandardOutput, [this] {
        const QString output = m_process->readAllStandardOutput();
        if (!output.isEmpty()) {
            qCInfo(qmlformatlog) << "qmlformat stdout is written to: " << m_logFile.fileName();
            QTextStream(&m_logFile) << "STDOUT: " << output << '\n';
        }
    });
    connect(m_process, &Process::readyReadStandardError, [this] {
        const QString output = m_process->readAllStandardError();
        if (!output.isEmpty()) {
            qCInfo(qmlformatlog) << "qmlformat stderr is written to: " << m_logFile.fileName();
            QTextStream(&m_logFile) << "STDERR: " << output << '\n';
        }
    });
}

QmlFormatProcess::~QmlFormatProcess()
{
    delete m_process;
}

void QmlFormatProcess::run()
{
    if (!m_process)
        return;

    if (m_cmd.executable().isEmpty()) {
        qCWarning(qmlformatlog) << "No executable set for QmlFormatProcess";
        return;
    }
    m_process->setCommand(m_cmd);
    m_process->setWorkingDirectory(m_workingDirectory);
    m_process->start();
}

void QmlFormatProcess::setWorkingDirectory(const Utils::FilePath &workingDirectory)
{
    m_workingDirectory = workingDirectory;
}

Utils::FilePath QmlFormatProcess::workingDirectory() const
{
    return m_workingDirectory;
}

void QmlFormatProcess::setCommandLine(const Utils::CommandLine &cmd)
{
    m_cmd = cmd;
}

Utils::CommandLine QmlFormatProcess::cmd() const
{
    return m_cmd;
}

} // namespace QmlJSTools

#include "qmlformatsettings.moc"
