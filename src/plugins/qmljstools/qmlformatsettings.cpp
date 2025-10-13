// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlformatsettings.h"
#include "qmljstoolstr.h"

#include <coreplugin/messagemanager.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qtsupport/qtversionmanager.h>

#include <utils/commandline.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>
#include <utils/temporaryfile.h>

#include <QLoggingCategory>
#include <QStandardPaths>
#include <QTemporaryDir>

using namespace QtSupport;
using namespace Utils;
using namespace ProjectExplorer;

namespace QmlJSTools {

static Q_LOGGING_CATEGORY(qmlformatlog, "qtc.qmljstools.qmlformat", QtWarningMsg)

class QmlFormatProcess : public QObject
{
    Q_OBJECT

public:
    QmlFormatProcess(const CommandLine &cmd);

signals:
    void qmlformatIniCreated(const FilePath &qmlformatIniFile);

public:
    Process m_process;
    TemporaryFile m_logFile;
    QTemporaryDir m_tempDir;
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
        if (!qtVersion->qmakeFilePath().isLocal())
            continue;

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
        generateQmlFormatIniContent();
    }
}

QmlFormatSettings::QmlFormatSettings()
{
    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsLoaded,
            this, &QmlFormatSettings::evaluateLatestQmlFormat);
}

QmlFormatSettings::~QmlFormatSettings() = default;

QmlFormatSettings& QmlFormatSettings::instance()
{
    static QmlFormatSettings instance;
    return instance;
}

FilePath QmlFormatSettings::globalQmlFormatIniFile()
{
    return FilePath::fromString(
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/.qmlformat.ini") ;
}

FilePath QmlFormatSettings::currentQmlFormatIniFile(const FilePath &path)
{
    const FilePath iniFile = path.searchHereAndInParents(".qmlformat.ini", QDir::Files);
    if (!iniFile.isEmpty())
        return iniFile;
    return globalQmlFormatIniFile();
}

FilePath QmlFormatSettings::latestQmlFormatPath() const
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
        Core::MessageManager::writeSilently(Tr::tr("No qmlformat executable found."));
        return;
    }

    CommandLine cmd(m_latestQmlFormat);
    cmd.addArg("--write-defaults");

    if (cmd.executable().isEmpty()) {
        Core::MessageManager::writeSilently(Tr::tr("No qmlformat executable found."));
        return;
    }

    auto qmlFormatProcess = new QmlFormatProcess(cmd);

    connect(qmlFormatProcess, &QmlFormatProcess::qmlformatIniCreated,
            this, &QmlFormatSettings::qmlformatIniCreated);

    qmlFormatProcess->m_process.start();
}

QmlFormatProcess::QmlFormatProcess(const CommandLine &cmd)
    : m_logFile("qmlformat.qtc.log")
{
    m_logFile.setAutoRemove(false);

    m_process.setCommand(cmd);
    m_process.setWorkingDirectory(FilePath::fromString(m_tempDir.path()));
    m_process.setProcessMode(ProcessMode::Writer);

    if (m_logFile.open()) {
        connect(&m_process, &Process::readyReadStandardOutput, [this] {
            const QString output = m_process.readAllStandardOutput();
            if (!output.isEmpty()) {
                qCInfo(qmlformatlog) << "qmlformat stdout is written to: " << m_logFile.filePath();
                QTextStream(&m_logFile) << "STDOUT: " << output << '\n';
            }
        });
        connect(&m_process, &Process::readyReadStandardError, [this] {
            const QString output = m_process.readAllStandardError();
            if (!output.isEmpty()) {
                qCInfo(qmlformatlog) << "qmlformat stderr is written to: " << m_logFile.filePath();
                QTextStream(&m_logFile) << "STDERR: " << output << '\n';
            }
        });
    }

    connect(&m_process, &Process::done, this, [this] {
        ProcessResultData result = m_process.resultData();
        FilePath qmlformatIniFile = FilePath::fromString(m_tempDir.filePath(".qmlformat.ini"));
        if (result.m_exitStatus == QProcess::NormalExit && result.m_exitCode == 0)
            emit qmlformatIniCreated(qmlformatIniFile);
        else
            Core::MessageManager::writeSilently(
                Tr::tr("Failed to generate qmlformat.ini file."));
        deleteLater();
    });
}

} // namespace QmlJSTools

#include "qmlformatsettings.moc"
