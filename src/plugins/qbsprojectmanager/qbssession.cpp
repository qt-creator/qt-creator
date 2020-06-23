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

#include "qbssession.h"

#include "qbspmlogging.h"
#include "qbsprojectmanagerconstants.h"
#include "qbssettings.h"

#include <coreplugin/messagemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

QStringList arrayToStringList(const QJsonValue &array)
{
    return transform<QStringList>(array.toArray(),
                                  [](const QJsonValue &v) { return v.toString(); });
}

const QByteArray packetStart = "qbsmsg:";

class Packet
{
public:
    enum class Status { Incomplete, Complete, Invalid };
    Status parseInput(QByteArray &input)
    {
        if (m_expectedPayloadLength == -1) {
            const int packetStartOffset = input.indexOf(packetStart);
            if (packetStartOffset == -1)
                return Status::Incomplete;
            const int numberOffset = packetStartOffset + packetStart.length();
            const int newLineOffset = input.indexOf('\n', numberOffset);
            if (newLineOffset == -1)
                return Status::Incomplete;
            const QByteArray sizeString = input.mid(numberOffset, newLineOffset - numberOffset);
            bool isNumber;
            const int payloadLen = sizeString.toInt(&isNumber);
            if (!isNumber || payloadLen < 0)
                return Status::Invalid;
            m_expectedPayloadLength = payloadLen;
            input.remove(0, newLineOffset + 1);
        }
        const int bytesToAdd = m_expectedPayloadLength - m_payload.length();
        QTC_ASSERT(bytesToAdd >= 0, return Status::Invalid);
        m_payload += input.left(bytesToAdd);
        input.remove(0, bytesToAdd);
        return isComplete() ? Status::Complete : Status::Incomplete;
    }

    QJsonObject retrievePacket()
    {
        QTC_ASSERT(isComplete(), return QJsonObject());
        const auto packet = QJsonDocument::fromJson(QByteArray::fromBase64(m_payload)).object();
        m_payload.clear();
        m_expectedPayloadLength = -1;
        return packet;
    }

    static QByteArray createPacket(const QJsonObject &packet)
    {
        const QByteArray jsonData = QJsonDocument(packet).toJson().toBase64();
        return QByteArray(packetStart).append(QByteArray::number(jsonData.length())).append('\n')
                .append(jsonData);
    }

private:
    bool isComplete() const { return m_payload.length() == m_expectedPayloadLength; }

    QByteArray m_payload;
    int m_expectedPayloadLength = -1;
};

class PacketReader : public QObject
{
    Q_OBJECT
public:
    PacketReader(QObject *parent) : QObject(parent) {}

    void handleData(const QByteArray &data)
    {
        m_incomingData += data;
        handleData();
    }

signals:
    void packetReceived(const QJsonObject &packet);
    void errorOccurred(const QString &msg);

private:
    void handleData()
    {
        switch (m_currentPacket.parseInput(m_incomingData)) {
        case Packet::Status::Invalid:
            emit errorOccurred(tr("Received invalid input."));
            break;
        case Packet::Status::Complete:
            emit packetReceived(m_currentPacket.retrievePacket());
            handleData();
            break;
        case Packet::Status::Incomplete:
            break;
        }
    }

    QByteArray m_incomingData;
    Packet m_currentPacket;
};

class QbsSession::Private
{
public:
    QProcess *qbsProcess = nullptr;
    PacketReader *packetReader = nullptr;
    QJsonObject currentRequest;
    QJsonObject projectData;
    QEventLoop eventLoop;
    QJsonObject reply;
    QHash<QString, QStringList> generatedFilesForSources;
    optional<Error> lastError;
    State state = State::Inactive;
};

QbsSession::QbsSession(QObject *parent) : QObject(parent), d(new Private)
{
    initialize();
}

void QbsSession::initialize()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("QT_FORCE_STDERR_LOGGING", "1");
    d->packetReader = new PacketReader(this);
    d->qbsProcess = new QProcess(this);
    d->qbsProcess->setProcessEnvironment(env);
    connect(d->qbsProcess, &QProcess::readyReadStandardOutput, this, [this] {
        d->packetReader->handleData(d->qbsProcess->readAllStandardOutput());
    });
    connect(d->qbsProcess, &QProcess::readyReadStandardError, this, [this] {
        qCDebug(qbsPmLog) << "[qbs stderr]: " << d->qbsProcess->readAllStandardError();
    });
    connect(d->qbsProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        d->eventLoop.exit(1);
        if (state() == State::ShuttingDown || state() == State::Inactive)
            return;
        switch (e) {
        case QProcess::FailedToStart:
            setError(Error::QbsFailedToStart);
            break;
        case QProcess::WriteError:
        case QProcess::ReadError:
            setError(Error::ProtocolError);
            break;
        case QProcess::Crashed:
        case QProcess::Timedout:
        case QProcess::UnknownError:
            break;
        }
    });
    connect(d->qbsProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this] {
        d->qbsProcess->deleteLater();
        switch (state()) {
        case State::Inactive:
            break;
        case State::ShuttingDown:
            setInactive();
            break;
        case State::Active:
            setError(Error::QbsQuit);
            break;
        case State::Initializing:
            setError(Error::ProtocolError);
            break;
        }
        d->qbsProcess = nullptr;
    });
    connect(d->packetReader, &PacketReader::errorOccurred, this, [this](const QString &msg) {
        qCDebug(qbsPmLog) << "session error" << msg;
        setError(Error::ProtocolError);
    });
    connect(d->packetReader, &PacketReader::packetReceived, this, &QbsSession::handlePacket);
    d->state = State::Initializing;
    const FilePath qbsExe = QbsSettings::qbsExecutableFilePath();
    if (qbsExe.isEmpty() || !qbsExe.exists()) {
        QTimer::singleShot(0, this, [this] { setError(Error::QbsFailedToStart); });
        return;
    }
    d->qbsProcess->start(qbsExe.toString(), {"session"});
}

void QbsSession::sendQuitPacket()
{
    d->qbsProcess->write(Packet::createPacket(QJsonObject{qMakePair(QString("type"),
                                                          QJsonValue("quit"))}));
}

QbsSession::~QbsSession()
{
    if (d->packetReader)
        d->packetReader->disconnect(this);
    if (d->qbsProcess) {
        d->qbsProcess->disconnect(this);
        quit();
        if (d->qbsProcess->state() == QProcess::Running && !d->qbsProcess->waitForFinished(10000))
            d->qbsProcess->terminate();
        if (d->qbsProcess->state() == QProcess::Running && !d->qbsProcess->waitForFinished(10000))
            d->qbsProcess->kill();
        d->qbsProcess->waitForFinished(1000);
    }
    delete d;
}

QbsSession::State QbsSession::state() const
{
    return d->state;
}

optional<QbsSession::Error> QbsSession::lastError() const
{
    return d->lastError;
}

QString QbsSession::errorString(QbsSession::Error error)
{
    switch (error) {
    case Error::QbsQuit:
        return tr("The qbs process quit unexpectedly.");
    case Error::QbsFailedToStart:
        return tr("The qbs process failed to start.");
    case Error::ProtocolError:
        return tr("The qbs process sent invalid data.");
    case Error::VersionMismatch:
        return tr("The qbs API level is not compatible with what Qt Creator expects.");
    }
    return QString(); // For dumb compilers.
}

QJsonObject QbsSession::projectData() const
{
    return d->projectData;
}

void QbsSession::sendRequest(const QJsonObject &request)
{
    QTC_ASSERT(d->currentRequest.isEmpty(),
               qDebug() << request.value("type").toString()
               << d->currentRequest.value("type").toString(); return);
    d->currentRequest = request;
    const QString logLevelFromEnv = qEnvironmentVariable("QBS_LOG_LEVEL");
    if (!logLevelFromEnv.isEmpty())
        d->currentRequest.insert("log-level", logLevelFromEnv);
    if (!qEnvironmentVariableIsEmpty(Constants::QBS_PROFILING_ENV))
        d->currentRequest.insert("log-time", true);
    if (d->state == State::Active)
        sendQueuedRequest();
    else if (d->state == State::Inactive)
        initialize();
}

void QbsSession::cancelCurrentJob()
{
    if (d->state == State::Active)
        sendRequest(QJsonObject{qMakePair(QString("type"), QJsonValue("cancel-job"))});
}

void QbsSession::quit()
{
    if (d->state == State::ShuttingDown || d->state == State::Inactive)
        return;
    d->state = State::ShuttingDown;
    sendQuitPacket();
}

void QbsSession::requestFilesGeneratedFrom(const QHash<QString, QStringList> &sourceFilesPerProduct)
{
    QJsonObject request;
    request.insert("type", "get-generated-files-for-sources");
    QJsonArray products;
    for (auto it = sourceFilesPerProduct.cbegin(); it != sourceFilesPerProduct.cend(); ++it) {
        QJsonObject product;
        product.insert("full-display-name", it.key());
        QJsonArray requests;
        for (const QString &sourceFile : it.value())
            requests << QJsonObject({qMakePair(QString("source-file"), sourceFile)});
        product.insert("requests", requests);
        products << product;
    }
    request.insert("products", products);
    sendRequest(request);
}

QStringList QbsSession::filesGeneratedFrom(const QString &sourceFile) const
{
    return d->generatedFilesForSources.value(sourceFile);
}

FileChangeResult QbsSession::addFiles(const QStringList &files, const QString &product,
                                      const QString &group)
{
    return updateFileList("add-files", files, product, group);
}

FileChangeResult QbsSession::removeFiles(const QStringList &files, const QString &product,
                                         const QString &group)
{
    return updateFileList("remove-files", files, product, group);
}

RunEnvironmentResult QbsSession::getRunEnvironment(
        const QString &product,
        const QProcessEnvironment &baseEnv,
        const QStringList &config)
{
    d->reply = QJsonObject();
    QJsonObject request;
    request.insert("type", "get-run-environment");
    request.insert("product", product);
    QJsonObject inEnv;
    const QStringList baseEnvKeys = baseEnv.keys();
    for (const QString &key : baseEnvKeys)
        inEnv.insert(key, baseEnv.value(key));
    request.insert("base-environment", inEnv);
    request.insert("config", QJsonArray::fromStringList(config));
    sendRequest(request);
    QTimer::singleShot(10000, this, [this] { d->eventLoop.exit(1); });
    if (d->eventLoop.exec(QEventLoop::ExcludeUserInputEvents) == 1)
        return RunEnvironmentResult(ErrorInfo(tr("Request timed out.")));
    QProcessEnvironment env;
    const QJsonObject outEnv = d->reply.value("full-environment").toObject();
    for (auto it = outEnv.begin(); it != outEnv.end(); ++it)
        env.insert(it.key(), it.value().toString());
    return RunEnvironmentResult(env, getErrorInfo(d->reply));
}

void QbsSession::insertRequestedModuleProperties(QJsonObject &request)
{
    request.insert("module-properties", QJsonArray::fromStringList({
        "cpp.commonCompilerFlags",
        "cpp.compilerVersionMajor",
        "cpp.compilerVersionMinor",
        "cpp.cLanguageVersion",
        "cpp.cxxLanguageVersion",
        "cpp.cxxStandardLibrary",
        "cpp.defines",
        "cpp.distributionIncludePaths",
        "cpp.driverFlags",
        "cpp.enableExceptions",
        "cpp.enableRtti",
        "cpp.exceptionHandlingModel",
        "cpp.frameworkPaths",
        "cpp.includePaths",
        "cpp.machineType",
        "cpp.minimumDarwinVersion",
        "cpp.minimumDarwinVersionCompilerFlag",
        "cpp.platformCommonCompilerFlags",
        "cpp.platformDriverFlags",
        "cpp.platformDefines",
        "cpp.positionIndependentCode",
        "cpp.systemFrameworkPaths",
        "cpp.systemIncludePaths",
        "cpp.target",
        "cpp.targetArch",
        "cpp.useCPrecompiledHeader",
        "cpp.useCxxPrecompiledHeader",
        "cpp.useObjcPrecompiledHeader",
        "cpp.useObjcxxPrecompiledHeader",
        "qbs.targetOS",
        "qbs.toolchain",
        "Qt.core.enableKeywords",
        "Qt.core.version",
    }));
}

// TODO: We can do better: Give out a (managed) session pointer here. Then we can re-use it
//       if the user chooses the imported project, saving the second build graph load.
QbsSession::BuildGraphInfo QbsSession::getBuildGraphInfo(const FilePath &bgFilePath,
                                                         const QStringList &requestedProperties)
{
    const QFileInfo bgFi = bgFilePath.toFileInfo();
    QDir buildRoot = bgFi.dir();
    buildRoot.cdUp();
    QJsonObject request;
    request.insert("type", "resolve-project");
    request.insert("restore-behavior", "restore-only");
    request.insert("configuration-name", bgFi.completeBaseName());
    if (QbsSettings::useCreatorSettingsDirForQbs())
        request.insert("settings-directory", QbsSettings::qbsSettingsBaseDir());
    request.insert("build-root", buildRoot.path());
    request.insert("error-handling-mode", "relaxed");
    request.insert("data-mode", "only-if-changed");
    request.insert("module-properties", QJsonArray::fromStringList(requestedProperties));
    QbsSession session(nullptr);
    session.sendRequest(request);
    QJsonObject reply;
    BuildGraphInfo bgInfo;
    bgInfo.bgFilePath = bgFilePath;
    QTimer::singleShot(10000, &session, [&session] { session.d->eventLoop.exit(1); });
    connect(&session, &QbsSession::errorOccurred, [&] {
        bgInfo.error = ErrorInfo(tr("Failed to load qbs build graph."));
    });
    connect(&session, &QbsSession::projectResolved, [&](const ErrorInfo &error) {
        bgInfo.error = error;
        session.d->eventLoop.quit();
    });
    if (session.d->eventLoop.exec(QEventLoop::ExcludeUserInputEvents) == 1) {
        bgInfo.error = ErrorInfo(tr("Request timed out."));
        return bgInfo;
    }
    if (bgInfo.error.hasError())
        return bgInfo;
    bgInfo.profileData = session.projectData().value("profile-data").toObject().toVariantMap();
    bgInfo.overriddenProperties = session.projectData().value("overridden-properties").toObject()
            .toVariantMap();
    QStringList props = requestedProperties;
    forAllProducts(session.projectData(), [&](const QJsonObject &product) {
        if (props.empty())
            return;
        for (auto it = props.begin(); it != props.end();) {
            const QVariant value = product.value("module-properties").toObject().value(*it);
            if (value.isValid()) {
                bgInfo.requestedProperties.insert(*it, value);
                it = props.erase(it);
            } else {
                ++it;
            }
        }
    });
    return bgInfo;
}

void QbsSession::handlePacket(const QJsonObject &packet)
{
    const QString type = packet.value("type").toString();
    if (type == "hello") {
        QTC_CHECK(d->state == State::Initializing);
        if (packet.value("api-compat-level").toInt() > 2) {
            setError(Error::VersionMismatch);
            return;
        }
        d->state = State::Active;
        sendQueuedRequest();
    } else if (type == "project-resolved") {
        setProjectDataFromReply(packet, true);
        emit projectResolved(getErrorInfo(packet));
    } else if (type == "project-built") {
        setProjectDataFromReply(packet, false);
        emit projectBuilt(getErrorInfo(packet));
    } else if (type == "project-cleaned") {
        emit projectCleaned(getErrorInfo(packet));
    } else if (type == "install-done") {
        emit projectInstalled(getErrorInfo(packet));
    } else if (type == "log-data") {
        Core::MessageManager::write("[qbs] " + packet.value("message").toString(),
                                    Core::MessageManager::Silent);
    } else if (type == "warning") {
        const ErrorInfo errorInfo = ErrorInfo(packet.value("warning").toObject());

        // TODO: This loop occurs a lot. Factor it out.
        for (const ErrorInfoItem &item : errorInfo.items) {
            TaskHub::addTask(BuildSystemTask(Task::Warning, item.description,
                                             item.filePath, item.line));
        }
    } else if (type == "task-started") {
        emit taskStarted(packet.value("description").toString(),
                         packet.value("max-progress").toInt());
    } else if (type == "task-progress") {
        emit taskProgress(packet.value("progress").toInt());
    } else if (type == "new-max-progress") {
        emit maxProgressChanged(packet.value("max-progress").toInt());
    } else if (type == "generated-files-for-sources") {
        QHash<QString, QStringList> generatedFiles;
        for (const QJsonValue &product : packet.value("products").toArray()) {
            for (const QJsonValue &r : product.toObject().value("results").toArray()) {
                const QJsonObject result = r.toObject();
                generatedFiles[result.value("source-file").toString()]
                        << arrayToStringList(result.value("generated-files").toArray());
            }
        }
        if (generatedFiles != d->generatedFilesForSources) {
            d->generatedFilesForSources = generatedFiles;
            emit newGeneratedFilesForSources(generatedFiles);
        }
    } else if (type == "command-description") {
        emit commandDescription(packet.value("message").toString());
    } else if (type == "files-added" || type == "files-removed") {
        handleFileListUpdated(packet);
    } else if (type == "process-result") {
        emit processResult(
                    FilePath::fromString(packet.value("executable-file-path").toString()),
                    arrayToStringList(packet.value("arguments")),
                    FilePath::fromString(packet.value("working-directory").toString()),
                    arrayToStringList(packet.value("stdout")),
                    arrayToStringList(packet.value("stderr")),
                    packet.value("success").toBool());
    } else if (type == "run-environment") {
        d->reply = packet;
        d->eventLoop.quit();
    }
}

void QbsSession::sendQueuedRequest()
{
    sendRequestNow(d->currentRequest);
    d->currentRequest = QJsonObject();
}

void QbsSession::sendRequestNow(const QJsonObject &request)
{
    QTC_ASSERT(d->state == State::Active, return);
    if (!request.isEmpty())
        d->qbsProcess->write(Packet::createPacket(request));
}

ErrorInfo QbsSession::getErrorInfo(const QJsonObject &packet)
{
    return ErrorInfo(packet.value("error").toObject());
}

void QbsSession::setProjectDataFromReply(const QJsonObject &packet, bool withBuildSystemFiles)
{
    const QJsonObject projectData = packet.value("project-data").toObject();
    if (!projectData.isEmpty()) {
        const QJsonValue buildSystemFiles = d->projectData.value("build-system-files");
        d->projectData = projectData;
        if (!withBuildSystemFiles)
            d->projectData.insert("build-system-files", buildSystemFiles);
    }
}

void QbsSession::setError(QbsSession::Error error)
{
    d->lastError = error;
    setInactive();
    emit errorOccurred(error);
}

void QbsSession::setInactive()
{
    if (d->state == State::Inactive)
        return;
    d->state = State::Inactive;
    d->qbsProcess->disconnect(this);
    d->currentRequest = QJsonObject();
    d->packetReader->disconnect(this);
    d->packetReader->deleteLater();
    d->packetReader = nullptr;
    if (d->qbsProcess->state() == QProcess::Running)
        sendQuitPacket();
    d->qbsProcess = nullptr;
}

FileChangeResult QbsSession::updateFileList(const char *action, const QStringList &files,
                                            const QString &product, const QString &group)
{
    if (d->state != State::Active)
        return FileChangeResult(files, tr("The qbs session is not in a valid state."));
    sendRequestNow(QJsonObject{
        {"type", QLatin1String(action)},
        {"files", QJsonArray::fromStringList(files)},
        {"product", product},
        {"group", group}
    });
    return FileChangeResult(QStringList());
}

void QbsSession::handleFileListUpdated(const QJsonObject &reply)
{
    setProjectDataFromReply(reply, false);
    const QStringList failedFiles = arrayToStringList(reply.value("failed-files"));
    if (!failedFiles.isEmpty()) {
        Core::MessageManager::write(tr("Failed to update files in Qbs project: %1.\n"
                                       "The affected files are: \n\t%2")
                                    .arg(getErrorInfo(reply).toString(),
                                         failedFiles.join("\n\t")),
                                    Core::MessageManager::ModeSwitch);
    }
    emit fileListUpdated();
}

ErrorInfoItem::ErrorInfoItem(const QJsonObject &data)
{
    description = data.value("description").toString();
    const QJsonObject locationData = data.value("location").toObject();
    filePath = FilePath::fromString(locationData.value("file-path").toString());
    line = locationData.value("line").toInt(-1);
}

QString ErrorInfoItem::toString() const
{
    QString s = filePath.toUserOutput();
    if (!s.isEmpty() && line != -1)
        s.append(':').append(QString::number(line));
    if (!s.isEmpty())
        s.append(':');
    return s.append(description);
}

ErrorInfo::ErrorInfo(const QJsonObject &data)
{
    for (const QJsonValue &v : data.value("items").toArray())
        items << ErrorInfoItem(v.toObject());
}

ErrorInfo::ErrorInfo(const QString &msg)
{
    items << ErrorInfoItem(msg);
}

QString ErrorInfo::toString() const
{
    return transform<QStringList>(items, [](const ErrorInfoItem &i) { return i.toString(); })
            .join('\n');
}

void forAllProducts(const QJsonObject &project, const WorkerFunction &productFunction)
{
    for (const QJsonValue &p : project.value("products").toArray())
        productFunction(p.toObject());
    for (const QJsonValue &p : project.value("sub-projects").toArray())
        forAllProducts(p.toObject(), productFunction);
}

void forAllArtifacts(const QJsonObject &product, ArtifactType type,
                     const WorkerFunction &artifactFunction)
{
    if (type == ArtifactType::Source || type == ArtifactType::All) {
        for (const QJsonValue &g : product.value("groups").toArray())
            forAllArtifacts(g.toObject(), artifactFunction);
    }
    if (type == ArtifactType::Generated || type == ArtifactType::All) {
        for (const QJsonValue &v : product.value("generated-artifacts").toArray())
            artifactFunction(v.toObject());
    }
}

void forAllArtifacts(const QJsonObject &group, const WorkerFunction &artifactFunction)
{
    for (const QJsonValue &v : group.value("source-artifacts").toArray())
        artifactFunction(v.toObject());
    for (const QJsonValue &v : group.value("source-artifacts-from-wildcards").toArray())
        artifactFunction(v.toObject());
}

Location locationFromObject(const QJsonObject &o)
{
    const QJsonObject loc = o.value("location").toObject();
    return Location(FilePath::fromString(loc.value("file-path").toString()),
                    loc.value("line").toInt());
}

} // namespace Internal
} // namespace QbsProjectManager

#include <qbssession.moc>
