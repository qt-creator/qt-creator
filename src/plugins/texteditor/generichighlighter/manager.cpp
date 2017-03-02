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

#include "manager.h"
#include "highlightdefinition.h"
#include "highlightdefinitionhandler.h"
#include "highlighterexception.h"
#include "definitiondownloader.h"
#include "highlightersettings.h"
#include <texteditor/plaintexteditorfactory.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/algorithm.h>
#include <utils/mapreduce.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/networkaccessmanager.h>

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegExp>
#include <QFuture>
#include <QtConcurrentMap>
#include <QUrl>
#include <QSet>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QMessageBox>
#include <QXmlSimpleReader>
#include <QXmlInputSource>
#include <QNetworkRequest>
#include <QNetworkReply>

using namespace Core;

namespace TextEditor {
namespace Internal {

const char kPriority[] = "priority";
const char kName[] = "name";
const char kExtensions[] = "extensions";
const char kMimeType[] = "mimetype";
const char kVersion[] = "version";
const char kUrl[] = "url";

class MultiDefinitionDownloader : public QObject
{
    Q_OBJECT

public:
    MultiDefinitionDownloader(const QString &savePath, const QList<QString> &installedDefinitions) :
        m_installedDefinitions(installedDefinitions),
        m_downloadPath(savePath)
    {
        connect(&m_downloadWatcher, &QFutureWatcherBase::finished,
                this, &MultiDefinitionDownloader::downloadDefinitionsFinished);
    }

    ~MultiDefinitionDownloader()
    {
        if (m_downloadWatcher.isRunning())
            m_downloadWatcher.cancel();
    }

    void downloadDefinitions(const QList<QUrl> &urls);

signals:
    void finished();

private:
    void downloadReferencedDefinition(const QString &name);
    void downloadDefinitionsFinished();

    QFutureWatcher<void> m_downloadWatcher;
    QList<DefinitionDownloader *> m_downloaders;
    QList<QString> m_installedDefinitions;
    QSet<QString> m_referencedDefinitions;
    QString m_downloadPath;
};

Manager::Manager() :
    m_multiDownloader(0),
    m_hasQueuedRegistration(false)
{
    connect(&m_registeringWatcher, &QFutureWatcherBase::finished,
            this, &Manager::registerHighlightingFilesFinished);
}

Manager::~Manager()
{
    disconnect(&m_registeringWatcher);
    disconnect(m_multiDownloader);
    if (m_registeringWatcher.isRunning())
        m_registeringWatcher.cancel();
    delete m_multiDownloader;
}

Manager *Manager::instance()
{
    static Manager manager;
    return &manager;
}

QString Manager::definitionIdByName(const QString &name) const
{
    return m_register.m_idByName.value(name);
}

static bool matchesPattern(const QString &fileName, DefinitionMetaDataPtr metaData)
{
    if (metaData.isNull())
        return false;
    foreach (const QString &pattern, metaData->patterns) {
        QRegExp reg(pattern, Qt::CaseSensitive, QRegExp::Wildcard);
        if (reg.exactMatch(fileName))
            return true;
    }
    return false;
}

QString Manager::definitionIdByMimeType(const Utils::MimeType &mimeType) const
{
    QList<Utils::MimeType> queue;
    queue.append(mimeType);
    while (!queue.isEmpty()) {
        const Utils::MimeType mt = queue.takeFirst();
        const QString id = m_register.m_idByMimeType.value(mt.name());
        if (!id.isEmpty())
            return id;
        foreach (const QString &parent, mt.parentMimeTypes()) {
            const Utils::MimeType parentMt = Utils::mimeTypeForName(parent);
            if (parentMt.isValid())
                queue.append(parentMt);
        }
    }
    return QString();
}

QString Manager::definitionIdByFile(const QString &filePath) const
{
    const QString fileName = QFileInfo(filePath).fileName();
    // find best match
    QString bestId;
    int bestPriority = -1;
    auto it = m_register.m_definitionsMetaData.constBegin();
    while (it != m_register.m_definitionsMetaData.constEnd()) {
        DefinitionMetaDataPtr metaData = it.value();
        if (metaData->priority > bestPriority && matchesPattern(fileName, metaData)) {
            bestId = metaData->id;
            bestPriority = metaData->priority;
        }
        ++it;
    }
    return bestId;
}

QString Manager::definitionIdByMimeTypeAndFile(const Utils::MimeType &mimeType,
                                               const QString &filePath) const
{
    QString id = definitionIdByMimeType(mimeType);
    if (!filePath.isEmpty()) {
        QString idByFile;
        const QString fileName = QFileInfo(filePath).fileName();
        // if mime type check returned no result, or doesn't match the patterns,
        // prefer a match by pattern
        if (id.isEmpty() || !matchesPattern(fileName, m_register.m_definitionsMetaData.value(id)))
            idByFile = definitionIdByFile(filePath);
        if (!idByFile.isEmpty())
            id = idByFile;
    }
    return id;
}

DefinitionMetaDataPtr Manager::availableDefinitionByName(const QString &name) const
{
    return m_availableDefinitions.value(name);
}

QSharedPointer<HighlightDefinition> Manager::definition(const QString &id)
{
    if (!id.isEmpty() && !m_definitions.contains(id)) {
        QFile definitionFile(id);
        if (!definitionFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return QSharedPointer<HighlightDefinition>();

        QSharedPointer<HighlightDefinition> definition(new HighlightDefinition);
        HighlightDefinitionHandler handler(definition);

        QXmlInputSource source(&definitionFile);
        QXmlSimpleReader reader;
        reader.setContentHandler(&handler);
        m_isBuildingDefinition.insert(id);
        try {
            reader.parse(source);
        } catch (const HighlighterException &e) {
            MessageManager::write(
                        QCoreApplication::translate("GenericHighlighter",
                                                    "Generic highlighter error: ") + e.message(),
                        MessageManager::WithFocus);
            definition.clear();
        }
        m_isBuildingDefinition.remove(id);
        definitionFile.close();

        m_definitions.insert(id, definition);
    }

    return m_definitions.value(id);
}

DefinitionMetaDataPtr Manager::definitionMetaData(const QString &id) const
{
    return m_register.m_definitionsMetaData.value(id);
}

bool Manager::isBuildingDefinition(const QString &id) const
{
    return m_isBuildingDefinition.contains(id);
}

static const int kMaxProgress = 200;

static void processHighlightingFiles(QFutureInterface<Manager::RegisterData> &future,
                                     QStringList definitionPaths)
{
    future.setProgressRange(0, kMaxProgress);

    Manager::RegisterData data;
    // iterate through paths in order, high priority > low priority
    foreach (const QString &path, definitionPaths) {
        if (path.isEmpty())
            continue;

        QDir definitionsDir(path);
        QStringList filter(QLatin1String("*.xml"));
        definitionsDir.setNameFilters(filter);
        foreach (const QFileInfo &fileInfo, definitionsDir.entryInfoList()) {
            if (future.isCanceled())
                return;
            if (future.progressValue() < kMaxProgress - 1)
                future.setProgressValue(future.progressValue() + 1);

            const DefinitionMetaDataPtr &metaData =
                    Manager::parseMetadata(fileInfo);
            // skip failing or already existing definitions
            if (!metaData.isNull() && !data.m_idByName.contains(metaData->name)) {
                const QString id = metaData->id;
                data.m_idByName.insert(metaData->name, id);
                data.m_definitionsMetaData.insert(id, metaData);
                foreach (const QString &mt, metaData->mimeTypes) {
                    bool insert = true;
                    // check if there is already a definition registered with higher priority
                    const QString existingDefinition = data.m_idByMimeType.value(mt);
                    if (!existingDefinition.isEmpty()) {
                        // check priorities
                        DefinitionMetaDataPtr existingMetaData =
                                data.m_definitionsMetaData.value(existingDefinition);
                        if (!existingMetaData.isNull() && existingMetaData->priority > metaData->priority)
                            insert = false;
                    }
                    if (insert)
                        data.m_idByMimeType.insert(mt, id);
                }
            }
        }
    }

    future.reportResult(data);
}

void Manager::registerHighlightingFiles()
{
    if (!m_registeringWatcher.isRunning()) {
        clear();

        QStringList definitionsPaths;
        const HighlighterSettings &settings = TextEditorSettings::highlighterSettings();
        definitionsPaths.append(settings.definitionFilesPath());
        if (settings.useFallbackLocation())
            definitionsPaths.append(settings.fallbackDefinitionFilesPath());

        QFuture<RegisterData> future = Utils::runAsync(processHighlightingFiles, definitionsPaths);
        m_registeringWatcher.setFuture(future);
    } else {
        m_hasQueuedRegistration = true;
        m_registeringWatcher.cancel();
    }
}

void Manager::registerHighlightingFilesFinished()
{
    if (m_hasQueuedRegistration) {
        m_hasQueuedRegistration = false;
        registerHighlightingFiles();
    } else if (!m_registeringWatcher.isCanceled()) {
        m_register = m_registeringWatcher.result();

        emit highlightingFilesRegistered();
    }
}

DefinitionMetaDataPtr Manager::parseMetadata(const QFileInfo &fileInfo)
{
    static const QLatin1Char kSemiColon(';');
    static const QLatin1String kLanguage("language");

    QFile definitionFile(fileInfo.absoluteFilePath());
    if (!definitionFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return DefinitionMetaDataPtr();

    DefinitionMetaDataPtr metaData(new HighlightDefinitionMetaData);

    QXmlStreamReader reader(&definitionFile);
    while (!reader.atEnd() && !reader.hasError()) {
        if (reader.readNext() == QXmlStreamReader::StartElement && reader.name() == kLanguage) {
            const QXmlStreamAttributes &atts = reader.attributes();

            metaData->fileName = fileInfo.fileName();
            metaData->id = fileInfo.absoluteFilePath();
            metaData->name = atts.value(QLatin1String(kName)).toString();
            metaData->version = atts.value(QLatin1String(kVersion)).toString();
            metaData->priority = atts.value(QLatin1String(kPriority)).toString().toInt();
            metaData->patterns = atts.value(QLatin1String(kExtensions))
                                  .toString().split(kSemiColon, QString::SkipEmptyParts);

            metaData->mimeTypes = atts.value(QLatin1String(kMimeType)).
                                    toString().split(kSemiColon, QString::SkipEmptyParts);
            break;
        }
    }
    reader.clear();
    definitionFile.close();

    return metaData;
}

QList<DefinitionMetaDataPtr> Manager::parseAvailableDefinitionsList(QIODevice *device)
{
    static const QLatin1Char kSlash('/');
    static const QLatin1String kDefinition("Definition");

    m_availableDefinitions.clear();
    QXmlStreamReader reader(device);
    while (!reader.atEnd() && !reader.hasError()) {
        if (reader.readNext() == QXmlStreamReader::StartElement &&
            reader.name() == kDefinition) {
            const QXmlStreamAttributes &atts = reader.attributes();

            DefinitionMetaDataPtr metaData(new HighlightDefinitionMetaData);
            metaData->name = atts.value(QLatin1String(kName)).toString();
            metaData->version = atts.value(QLatin1String(kVersion)).toString();
            QString url = atts.value(QLatin1String(kUrl)).toString();
            metaData->url = QUrl(url);
            const int slash = url.lastIndexOf(kSlash);
            if (slash != -1)
                metaData->fileName = url.right(url.length() - slash - 1);

            m_availableDefinitions.insert(metaData->name, metaData);
        }
    }
    reader.clear();

    return m_availableDefinitions.values();
}

void Manager::downloadAvailableDefinitionsMetaData()
{
    QUrl url(QLatin1String("https://www.kate-editor.org/syntax/update-5.35.xml"));
    QNetworkRequest request(url);
    // Currently this takes a couple of seconds on Windows 7: QTBUG-10106.
    QNetworkReply *reply = Utils::NetworkAccessManager::instance()->get(request);
    connect(reply, &QNetworkReply::finished,
            this, &Manager::downloadAvailableDefinitionsListFinished);
}

void Manager::downloadAvailableDefinitionsListFinished()
{
    if (QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender())) {
        if (reply->error() == QNetworkReply::NoError)
            emit definitionsMetaDataReady(parseAvailableDefinitionsList(reply));
        else
            emit errorDownloadingDefinitionsMetaData();
        reply->deleteLater();
    }
}

void Manager::downloadDefinitions(const QList<QUrl> &urls, const QString &savePath)
{
    m_multiDownloader = new MultiDefinitionDownloader(savePath, m_register.m_idByName.keys());
    connect(m_multiDownloader, &MultiDefinitionDownloader::finished,
            this, &Manager::downloadDefinitionsFinished);
    m_multiDownloader->downloadDefinitions(urls);
}

void MultiDefinitionDownloader::downloadDefinitions(const QList<QUrl> &urls)
{
    m_downloaders.clear();
    foreach (const QUrl &url, urls) {
        DefinitionDownloader *downloader = new DefinitionDownloader(url, m_downloadPath);
        connect(downloader, &DefinitionDownloader::foundReferencedDefinition,
                this, &MultiDefinitionDownloader::downloadReferencedDefinition);
        m_downloaders.append(downloader);
    }

    QFuture<void> future = Utils::map(m_downloaders, &DefinitionDownloader::run);
    m_downloadWatcher.setFuture(future);
    ProgressManager::addTask(future, tr("Downloading Highlighting Definitions"),
                             "TextEditor.Task.Download");
}

void MultiDefinitionDownloader::downloadDefinitionsFinished()
{
    int errors = 0;
    bool writeError = false;
    foreach (DefinitionDownloader *downloader, m_downloaders) {
        DefinitionDownloader::Status status = downloader->status();
        if (status != DefinitionDownloader::Ok) {
            ++errors;
            if (status == DefinitionDownloader::WriteError && !writeError)
                writeError = true;
        }
        delete downloader;
    }

    if (errors > 0) {
        QString text;
        if (errors == m_downloaders.size())
            text = tr("Error downloading selected definition(s).");
        else
            text = tr("Error downloading one or more definitions.");
        if (writeError)
            text.append(QLatin1Char('\n') + tr("Please check the directory's access rights."));
        QMessageBox::critical(Core::ICore::dialogParent(), tr("Download Error"), text);
    }

    QList<QUrl> urls;
    foreach (const QString &definition, m_referencedDefinitions) {
        if (DefinitionMetaDataPtr metaData =
                Manager::instance()->availableDefinitionByName(definition)) {
            urls << metaData->url;
        }
    }
    m_referencedDefinitions.clear();
    if (urls.isEmpty())
        emit finished();
    else
        downloadDefinitions(urls);
}

void Manager::downloadDefinitionsFinished()
{
    delete m_multiDownloader;
    m_multiDownloader = 0;
}

void MultiDefinitionDownloader::downloadReferencedDefinition(const QString &name)
{
    if (m_installedDefinitions.contains(name))
        return;
    m_referencedDefinitions.insert(name);
    m_installedDefinitions.append(name);
}

bool Manager::isDownloadingDefinitions() const
{
    return m_multiDownloader != 0;
}

void Manager::clear()
{
    m_register.m_idByName.clear();
    m_register.m_idByMimeType.clear();
    m_register.m_definitionsMetaData.clear();
    m_definitions.clear();
}

} // namespace Internal
} // namespace TextEditor

#include "manager.moc"
