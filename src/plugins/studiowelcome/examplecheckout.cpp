// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "examplecheckout.h"

#include "studiowelcomeplugin.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>

#include <utils/archive.h>
#include <utils/algorithm.h>
#include <utils/networkaccessmanager.h>
#include <utils/qtcassert.h>

#include <private/qqmldata_p.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <projectexplorer/projectexplorer.h>

#include <qmldesigner/qmldesignerplugin.h>

#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQuickItem>
#include <QQuickWidget>

#include <algorithm>

using namespace Utils;

static bool enableDownload()
{
    const QString lastQDSVersionEntry = "QML/Designer/EnableWelcomePageDownload";
    return Core::ICore::settings()->value(lastQDSVersionEntry, false).toBool();
}

void ExampleCheckout::registerTypes()
{
    static bool once = []() {
        FileDownloader::registerQmlType();
        FileExtractor::registerQmlType();
        return true;
    }();

    QTC_ASSERT(once, ;);
}

void FileDownloader::registerQmlType()
{
    qmlRegisterType<FileDownloader>("ExampleCheckout", 1, 0, "FileDownloader");
}

FileDownloader::FileDownloader(QObject *parent)
    : QObject(parent)
{}

FileDownloader::~FileDownloader()
{
    if (m_tempFile.exists())
        m_tempFile.remove();
}

void FileDownloader::start()
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics("exampleDownload:" + name());
    m_tempFile.setFileName(QDir::tempPath() + "/" + name() + ".XXXXXX" + ".zip");
    m_tempFile.open(QIODevice::WriteOnly);

    auto request = QNetworkRequest(m_url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::UserVerifiedRedirectPolicy);
    m_reply = Utils::NetworkAccessManager::instance()->get(request);

    QNetworkReply::connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        m_tempFile.write(m_reply->readAll());
    });

    QNetworkReply::connect(m_reply,
                           &QNetworkReply::downloadProgress,
                           this,
                           [this](qint64 current, qint64 max) {
                               if (max == 0)
                                   return;

                               m_progress = current * 100 / max;
                               emit progressChanged();
                           });

    QNetworkReply::connect(m_reply, &QNetworkReply::redirected, [this](const QUrl &) {
        emit m_reply->redirectAllowed();
    });

    QNetworkReply::connect(m_reply, &QNetworkReply::finished, this, [this]() {
        if (m_reply->error()) {
            if (m_tempFile.exists())
                m_tempFile.remove();

            if (m_reply->error() != QNetworkReply::OperationCanceledError) {
                qWarning() << Q_FUNC_INFO << m_url << m_reply->errorString();
                emit downloadFailed();
            } else {
                emit downloadCanceled();
            }
        } else {
            m_tempFile.flush();
            m_tempFile.close();
            m_finished = true;
            emit tempFileChanged();
            emit finishedChanged();
        }

        m_reply = nullptr;
    });
}

void FileDownloader::cancel()
{
    if (m_reply)
        m_reply->abort();
}

void FileDownloader::setUrl(const QUrl &url)
{
    m_url = url;
    emit nameChanged();

    probeUrl();
}

QUrl FileDownloader::url() const
{
    return m_url;
}

bool FileDownloader::finished() const
{
    return m_finished;
}

bool FileDownloader::error() const
{
    return m_error;
}

QString FileDownloader::name() const
{
    const QFileInfo fileInfo(m_url.path());
    return fileInfo.baseName();
}

QString FileDownloader::completeBaseName() const
{
    const QFileInfo fileInfo(m_url.path());
    return fileInfo.completeBaseName();
}

int FileDownloader::progress() const
{
    return m_progress;
}

QString FileDownloader::tempFile() const
{
    return QFileInfo(m_tempFile).canonicalFilePath();
}

QDateTime FileDownloader::lastModified() const
{
    return m_lastModified;
}

bool FileDownloader::available() const
{
    return m_available;
}

void FileDownloader::probeUrl()
{
    if (!enableDownload()) {
        m_available = false;
        emit availableChanged();
        return;
    }

    auto request = QNetworkRequest(m_url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::UserVerifiedRedirectPolicy);
    QNetworkReply *reply = Utils::NetworkAccessManager::instance()->head(request);

    QNetworkReply::connect(reply, &QNetworkReply::redirected, [reply](const QUrl &) {
        emit reply->redirectAllowed();
    });

    QNetworkReply::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error())
            return;

        m_lastModified = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime();
        emit lastModifiedChanged();

        m_available = true;
        emit availableChanged();
    });

    QNetworkReply::connect(reply,
                           &QNetworkReply::errorOccurred,
                           this,
                           [this](QNetworkReply::NetworkError) {
                               QQmlData *data = QQmlData::get(this, false);
                               if (!data) {
                                   qDebug() << Q_FUNC_INFO << "FileDownloader is nullptr.";
                                   return;
                               }

                               if (QQmlData::wasDeleted(this)) {
                                   qDebug() << Q_FUNC_INFO << "FileDownloader was deleted.";
                                   return;
                               }

                               m_available = false;
                               emit availableChanged();
                           });
}


FileExtractor::FileExtractor(QObject *parent)
    : QObject(parent)
{
    m_targetPath = Utils::FilePath::fromString(
        StudioWelcome::Internal::StudioWelcomePlugin::examplesPathSetting());

    m_timer.setInterval(100);
    m_timer.setSingleShot(false);

    QObject::connect(this, &FileExtractor::targetFolderExistsChanged, this, [this]() {
        if (targetFolderExists()) {
            m_birthTime = QFileInfo(m_targetPath.toString() + "/" + m_archiveName).birthTime();
        } else
            m_birthTime = QDateTime();

        emit birthTimeChanged();
    });

    const ExtensionSystem::PluginSpec *pluginSpec
        = Utils::findOrDefault(ExtensionSystem::PluginManager::plugins(),
                               Utils::equal(&ExtensionSystem::PluginSpec::name,
                                            QString("StudioWelcome")));

    if (!pluginSpec)
        return;

    ExtensionSystem::IPlugin *plugin = pluginSpec->plugin();

    if (!plugin)
        return;

    auto studioWelcomePlugin = qobject_cast<StudioWelcome::Internal::StudioWelcomePlugin *>(plugin);

    if (studioWelcomePlugin) {
        QObject::connect(studioWelcomePlugin,
                         &StudioWelcome::Internal::StudioWelcomePlugin::examplesDownloadPathChanged,
                         this,
                         [this](const QString &path) {
                             m_targetPath = Utils::FilePath::fromString(path);
                             emit targetPathChanged();
                             emit targetFolderExistsChanged();
                         });
    }
}

FileExtractor::~FileExtractor() {}

void FileExtractor::registerQmlType()
{
    qmlRegisterType<FileExtractor>("ExampleCheckout", 1, 0, "FileExtractor");
}

QString FileExtractor::targetPath() const
{
    return m_targetPath.toUserOutput();
}

void FileExtractor::browse()
{
    const FilePath path =
            FileUtils::getExistingDirectory(nullptr, tr("Choose Directory"), m_targetPath);

    if (!path.isEmpty())
        m_targetPath = path;

    emit targetPathChanged();
    emit targetFolderExistsChanged();
}

void FileExtractor::setSourceFile(QString &sourceFilePath)
{
    m_sourceFile = Utils::FilePath::fromString(sourceFilePath);
    emit targetFolderExistsChanged();
}

void FileExtractor::setArchiveName(QString &filePath)
{
    m_archiveName = filePath;
    emit targetFolderExistsChanged();
}

const QString FileExtractor::detailedText()
{
    return m_detailedText;
}

bool FileExtractor::finished() const
{
    return m_finished;
}

QString FileExtractor::currentFile() const
{
    return m_currentFile;
}

QString FileExtractor::size() const
{
    return m_size;
}

QString FileExtractor::count() const
{
    return m_count;
}

bool FileExtractor::targetFolderExists() const
{
    return QFileInfo::exists(m_targetPath.toString() + "/" + m_archiveName);
}

int FileExtractor::progress() const
{
    return m_progress;
}

QDateTime FileExtractor::birthTime() const
{
    return m_birthTime;
}

QString FileExtractor::archiveName() const
{
    return m_archiveName;
}

QString FileExtractor::sourceFile() const
{
    return m_sourceFile.toString();
}

void FileExtractor::extract()
{
    const QString targetFolder = m_targetPath.toString() + "/" + m_archiveName;

    // If the target directory already exists, remove it and its content
    QDir targetDir(targetFolder);
    if (targetDir.exists())
        targetDir.removeRecursively();

    // Create a new directory to generate a proper creation date
    targetDir.mkdir(targetFolder);

    Utils::Archive *archive = new Utils::Archive(m_sourceFile, m_targetPath);
    QTC_ASSERT(archive->isValid(), delete archive; return);

    m_timer.start();
    qint64 bytesBefore = QStorageInfo(m_targetPath.toFileInfo().dir()).bytesAvailable();
    qint64 compressedSize = QFileInfo(m_sourceFile.toString()).size();

    QTimer::connect(
        &m_timer, &QTimer::timeout, this, [this, bytesBefore, targetFolder, compressedSize]() {
            static QHash<QString, int> hash;
            QDirIterator it(targetFolder, {"*.*"}, QDir::Files, QDirIterator::Subdirectories);

            int count = 0;
            while (it.hasNext()) {
                if (!hash.contains(it.fileName())) {
                    m_currentFile = it.fileName();
                    hash.insert(m_currentFile, 0);
                    emit currentFileChanged();
                }
                it.next();
                count++;
            }

            qint64 currentSize = bytesBefore
                                 - QStorageInfo(m_targetPath.toFileInfo().dir()).bytesAvailable();

            // We can not get the uncompressed size of the archive yet, that is why we use an
            // approximation. We assume a 50% compression rate.
            m_progress = std::min(100ll, currentSize * 100 / compressedSize * 2);
            emit progressChanged();

            m_size = QString::number(currentSize);
            m_count = QString::number(count);
            emit sizeChanged();
        });

    QObject::connect(archive, &Utils::Archive::outputReceived, this, [this](const QString &output) {
        m_detailedText += output;
        emit detailedTextChanged();
    });

    QObject::connect(archive, &Utils::Archive::finished, this, [this, archive](bool ret) {
        archive->deleteLater();
        m_finished = ret;
        m_timer.stop();

        m_progress = 100;
        emit progressChanged();

        emit targetFolderExistsChanged();
        emit finishedChanged();
        QTC_CHECK(ret);
    });
    archive->unarchive();
}

static Utils::FilePath tempFilePath()
{
    QStandardPaths::StandardLocation location = QStandardPaths::CacheLocation;

    return Utils::FilePath::fromString(QStandardPaths::writableLocation(location))
        .pathAppended("QtDesignStudio");
}

DataModelDownloader::DataModelDownloader(QObject * /* parent */)
{
    auto fileInfo = targetFolder().toFileInfo();
    m_birthTime = fileInfo.lastModified();
    m_exists = fileInfo.exists();

    connect(&m_fileDownloader,
            &FileDownloader::progressChanged,
            this,
            &DataModelDownloader::progressChanged);

    connect(&m_fileDownloader,
            &FileDownloader::downloadFailed,
            this,
            &DataModelDownloader::downloadFailed);
}

bool DataModelDownloader::start()
{

    if (!enableDownload()) {
        m_available = false;
        emit availableChanged();
        return false;
    }

    m_fileDownloader.setUrl(QUrl::fromUserInput(
        "https://download.qt.io/learning/examples/qtdesignstudio/dataImports.zip"));

    bool started = false;

    connect(&m_fileDownloader, &FileDownloader::availableChanged, this, [this, &started]() {

        m_available = m_fileDownloader.available();

        emit availableChanged();

        if (!m_available) {
            qWarning() << m_fileDownloader.url() << "failed to download";
            return;
        }

        if (!m_forceDownload && (m_fileDownloader.lastModified() <= m_birthTime))
            return;

        started = true;

        m_fileDownloader.start();
        connect(&m_fileDownloader, &FileDownloader::finishedChanged, this, [this]() {
            if (m_fileDownloader.finished()) {
                const Utils::FilePath archiveFile = Utils::FilePath::fromString(
                    m_fileDownloader.tempFile());
                QTC_ASSERT(Utils::Archive::supportsFile(archiveFile), return );
                auto archive = new Utils::Archive(archiveFile, tempFilePath());
                QTC_ASSERT(archive->isValid(), delete archive; return );
                QObject::connect(archive, &Utils::Archive::finished, this, [this, archive](bool ret) {
                    QTC_CHECK(ret);
                    archive->deleteLater();
                    emit finished();
                });
                archive->unarchive();
            }
        });
    });
    return started;
}

bool DataModelDownloader::exists() const
{
    return m_exists;
}

bool DataModelDownloader::available() const
{
    return m_available;
}

Utils::FilePath DataModelDownloader::targetFolder() const
{
    return Utils::FilePath::fromUserInput(tempFilePath().toString() + "/" + "dataImports");
}

void DataModelDownloader::setForceDownload(bool b)
{
    m_forceDownload = b;
}

int DataModelDownloader::progress() const
{
    return m_fileDownloader.progress();
}
