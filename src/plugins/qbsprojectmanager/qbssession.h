/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#pragma once

#include <utils/fileutils.h>
#include <utils/optional.h>

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QVariant>

#include <functional>

namespace ProjectExplorer { class Target; }

namespace QbsProjectManager {
namespace Internal {

class ErrorInfoItem
{
public:
    ErrorInfoItem(const QJsonObject &data);
    ErrorInfoItem(const QString &msg) : description(msg) {}

    QString toString() const;

    QString description;
    Utils::FilePath filePath;
    int line = -1;
};

class ErrorInfo
{
public:
    ErrorInfo() = default;
    ErrorInfo(const QJsonObject &data);
    ErrorInfo(const QString &msg);

    QString toString() const;
    bool hasError() const { return !items.isEmpty(); }

    QList<ErrorInfoItem> items;
};

template<typename Data> class SynchronousRequestResult
{
public:
    ErrorInfo error() const { return m_error; }
    SynchronousRequestResult(const Data &d, const ErrorInfo &e = {}) : m_error(e), m_data(d) {}
    SynchronousRequestResult(const ErrorInfo &e) : SynchronousRequestResult(Data(), e) {}

protected:
    const ErrorInfo m_error;
    const Data m_data;
};

class FileChangeResult : public SynchronousRequestResult<QStringList>
{
public:
    using SynchronousRequestResult::SynchronousRequestResult;
    QStringList failedFiles () const { return m_data; }
};

class RunEnvironmentResult : public SynchronousRequestResult<QProcessEnvironment>
{
public:
    using SynchronousRequestResult::SynchronousRequestResult;
    QProcessEnvironment environment() { return m_data; }
};

// TODO: Put the helper function somewhere else. E.g. in qbsnodes.cpp we don't want a session include.
QStringList arrayToStringList(const QJsonValue &array);

using WorkerFunction = std::function<void(const QJsonObject &)>;
void forAllProducts(const QJsonObject &projectData, const WorkerFunction &productFunction);

enum class ArtifactType { Source, Generated, All };
void forAllArtifacts(const QJsonObject &product, ArtifactType type,
                     const WorkerFunction &artifactFunction);
void forAllArtifacts(const QJsonObject &group, const WorkerFunction &artifactFunction);

class Location
{
public:
    Location(const Utils::FilePath &p, int l) : filePath(p), line (l) {}
    const Utils::FilePath filePath;
    const int line;
};
Location locationFromObject(const QJsonObject &o); // Project, Product or Group

class QbsSession : public QObject
{
    Q_OBJECT
public:
    explicit QbsSession(QObject *parent = nullptr);
    ~QbsSession() override;

    enum class State { Initializing, Active, ShuttingDown, Inactive };
    enum class Error { QbsFailedToStart, QbsQuit, ProtocolError, VersionMismatch };

    State state() const;
    Utils::optional<Error> lastError() const;
    static QString errorString(Error error);
    QJsonObject projectData() const;

    void sendRequest(const QJsonObject &request);
    void cancelCurrentJob();
    void quit();
    void requestFilesGeneratedFrom(const QHash<QString, QStringList> &sourceFilesPerProduct);
    QStringList filesGeneratedFrom(const QString &sourceFile) const;
    FileChangeResult addFiles(const QStringList &files, const QString &product,
                              const QString &group);
    FileChangeResult removeFiles(const QStringList &files, const QString &product,
                                 const QString &group);
    RunEnvironmentResult getRunEnvironment(const QString &product,
            const QProcessEnvironment &baseEnv,
            const QStringList &config);

    static void insertRequestedModuleProperties(QJsonObject &request);

    class BuildGraphInfo
    {
    public:
        Utils::FilePath bgFilePath;
        QVariantMap overriddenProperties;
        QVariantMap profileData;
        QVariantMap requestedProperties;
        ErrorInfo error;
    };
    static BuildGraphInfo getBuildGraphInfo(const Utils::FilePath &bgFilePath,
                                            const QStringList &requestedProperties);

signals:
    void errorOccurred(Error lastError);
    void projectResolved(const ErrorInfo &error);
    void projectBuilt(const ErrorInfo &error);
    void projectCleaned(const ErrorInfo &error);
    void projectInstalled(const ErrorInfo &error);
    void newGeneratedFilesForSources(const QHash<QString, QStringList> &generatedFiles);
    void taskStarted(const QString &description, int maxProgress);
    void maxProgressChanged(int maxProgress);
    void taskProgress(int progress);
    void commandDescription(const QString &description);
    void processResult(
            const Utils::FilePath &executable,
            const QStringList &arguments,
            const Utils::FilePath &workingDir,
            const QStringList &stdOut,
            const QStringList &stdErr,
            bool success);
    void fileListUpdated();

private:
    void initialize();
    void sendQuitPacket();
    void handlePacket(const QJsonObject &packet);
    void sendQueuedRequest();
    void sendRequestNow(const QJsonObject &request);
    ErrorInfo getErrorInfo(const QJsonObject &packet);
    void setProjectDataFromReply(const QJsonObject &packet, bool withBuildSystemFiles);
    void setError(Error error);
    void setInactive();
    FileChangeResult updateFileList(const char *action, const QStringList &files,
                                    const QString &product, const QString &group);
    void handleFileListUpdated(const QJsonObject &reply);

    class Private;
    Private * const d;
};

} // namespace Internal
} // namespace QbsProjectManager
