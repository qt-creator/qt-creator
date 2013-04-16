/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "manager.h"
#include "highlightdefinition.h"
#include "highlightdefinitionhandler.h"
#include "highlighterexception.h"
#include "definitiondownloader.h"
#include "highlightersettings.h"
#include "plaintexteditorfactory.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"
#include "texteditorsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/QtConcurrentTools>
#include <utils/networkaccessmanager.h>

#include <QtAlgorithms>
#include <QString>
#include <QLatin1Char>
#include <QLatin1String>
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

using namespace TextEditor;
using namespace Internal;

Manager::Manager() :
    m_isDownloadingDefinitionsSpec(false),
    m_hasQueuedRegistration(false)
{
    connect(&m_registeringWatcher, SIGNAL(finished()), this, SLOT(registerMimeTypesFinished()));
    connect(&m_downloadWatcher, SIGNAL(finished()), this, SLOT(downloadDefinitionsFinished()));
}

Manager::~Manager()
{
    disconnect(&m_registeringWatcher);
    disconnect(&m_downloadWatcher);
    if (m_registeringWatcher.isRunning())
        m_registeringWatcher.cancel();
    if (m_downloadWatcher.isRunning())
        m_downloadWatcher.cancel();
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

QString Manager::definitionIdByMimeType(const QString &mimeType) const
{
    return m_register.m_idByMimeType.value(mimeType);
}

QString Manager::definitionIdByAnyMimeType(const QStringList &mimeTypes) const
{
    QString definitionId;
    foreach (const QString &mimeType, mimeTypes) {
        definitionId = definitionIdByMimeType(mimeType);
        if (!definitionId.isEmpty())
            break;
    }
    return definitionId;
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
        } catch (HighlighterException &) {
            definition.clear();
        }
        m_isBuildingDefinition.remove(id);
        definitionFile.close();

        m_definitions.insert(id, definition);
    }

    return m_definitions.value(id);
}

QSharedPointer<HighlightDefinitionMetaData> Manager::definitionMetaData(const QString &id) const
{
    return m_register.m_definitionsMetaData.value(id);
}

bool Manager::isBuildingDefinition(const QString &id) const
{
    return m_isBuildingDefinition.contains(id);
}

namespace TextEditor {
namespace Internal {

class ManagerProcessor : public QObject
{
    Q_OBJECT
public:
    ManagerProcessor();
    void process(QFutureInterface<QPair<Manager::RegisterData,
                                        QList<Core::MimeType> > > &future);

    QStringList m_definitionsPaths;
    QSet<QString> m_knownMimeTypes;
    QSet<QString> m_knownSuffixes;
    QHash<QString, Core::MimeType> m_userModified;
    static const int kMaxProgress;

    struct PriorityComp
    {
        bool operator()(const QSharedPointer<HighlightDefinitionMetaData> &a,
                        const QSharedPointer<HighlightDefinitionMetaData> &b) {
            return a->priority() > b->priority();
        }
    };
};

const int ManagerProcessor::kMaxProgress = 200;

ManagerProcessor::ManagerProcessor()
    : m_knownSuffixes(QSet<QString>::fromList(Core::ICore::mimeDatabase()->suffixes()))
{
    const HighlighterSettings &settings = TextEditorSettings::instance()->highlighterSettings();
    m_definitionsPaths.append(settings.definitionFilesPath());
    if (settings.useFallbackLocation())
        m_definitionsPaths.append(settings.fallbackDefinitionFilesPath());

    Core::MimeDatabase *mimeDatabase = Core::ICore::mimeDatabase();
    foreach (const Core::MimeType &userMimeType, mimeDatabase->readUserModifiedMimeTypes())
        m_userModified.insert(userMimeType.type(), userMimeType);
    foreach (const Core::MimeType &mimeType, mimeDatabase->mimeTypes())
        m_knownMimeTypes.insert(mimeType.type());
}

void ManagerProcessor::process(QFutureInterface<QPair<Manager::RegisterData,
                                                      QList<Core::MimeType> > > &future)
{
    future.setProgressRange(0, kMaxProgress);

    // @TODO: Improve MIME database to handle the following limitation.
    // The generic highlighter only register its types after all other plugins
    // have populated Creator's MIME database (so it does not override anything).
    // When the generic highlighter settings change only its internal data is cleaned-up
    // and rebuilt. Creator's MIME database is not touched. So depending on how the
    // user plays around with the generic highlighter file definitions (changing
    // duplicated patterns, for example), some changes might not be reflected.
    // A definitive implementation would require some kind of re-load or update
    // (considering hierarchies, aliases, etc) of the MIME database whenever there
    // is a change in the generic highlighter settings.

    Manager::RegisterData data;
    QList<Core::MimeType> newMimeTypes;

    foreach (const QString &path, m_definitionsPaths) {
        if (path.isEmpty())
            continue;

        QDir definitionsDir(path);
        QStringList filter(QLatin1String("*.xml"));
        definitionsDir.setNameFilters(filter);
        QList<QSharedPointer<HighlightDefinitionMetaData> > allMetaData;
        foreach (const QFileInfo &fileInfo, definitionsDir.entryInfoList()) {
            const QSharedPointer<HighlightDefinitionMetaData> &metaData =
                    Manager::parseMetadata(fileInfo);
            if (!metaData.isNull())
                allMetaData.append(metaData);
        }

        // Consider definitions with higher priority first.
        qSort(allMetaData.begin(), allMetaData.end(), PriorityComp());

        foreach (const QSharedPointer<HighlightDefinitionMetaData> &metaData, allMetaData) {
            if (future.isCanceled())
                return;
            if (future.progressValue() < kMaxProgress - 1)
                future.setProgressValue(future.progressValue() + 1);

            if (data.m_idByName.contains(metaData->name()))
                // Name already exists... This is a fallback item, do not consider it.
                continue;

            const QString &id = metaData->id();
            data.m_idByName.insert(metaData->name(), id);
            data.m_definitionsMetaData.insert(id, metaData);

            static const QStringList textPlain(QLatin1String("text/plain"));

            // A definition can specify multiple MIME types and file extensions/patterns,
            // but all on a single string. So associate all patterns with all MIME types.
            QList<Core::MimeGlobPattern> globPatterns;
            foreach (const QString &type, metaData->mimeTypes()) {
                if (data.m_idByMimeType.contains(type))
                    continue;

                data.m_idByMimeType.insert(type, id);
                if (!m_knownMimeTypes.contains(type)) {
                    m_knownMimeTypes.insert(type);

                    Core::MimeType mimeType;
                    mimeType.setType(type);
                    mimeType.setSubClassesOf(textPlain);
                    mimeType.setComment(metaData->name());

                    // If there's a user modification for this mime type, we want to use the
                    // modified patterns and rule-based matchers. If not, just consider what
                    // is specified in the definition file.
                    QHash<QString, Core::MimeType>::const_iterator it =
                        m_userModified.find(mimeType.type());
                    if (it == m_userModified.end()) {
                        if (globPatterns.isEmpty()) {
                            foreach (const QString &pattern, metaData->patterns()) {
                                static const QLatin1String mark("*.");
                                if (pattern.startsWith(mark)) {
                                    const QString &suffix = pattern.right(pattern.length() - 2);
                                    if (!m_knownSuffixes.contains(suffix))
                                        m_knownSuffixes.insert(suffix);
                                    else
                                        continue;
                                }
                                globPatterns.append(Core::MimeGlobPattern(pattern, 50));
                            }
                        }
                        mimeType.setGlobPatterns(globPatterns);
                    } else {
                        mimeType.setGlobPatterns(it.value().globPatterns());
                        mimeType.setMagicRuleMatchers(it.value().magicRuleMatchers());
                    }

                    newMimeTypes.append(mimeType);
                }
            }
        }
    }

    future.reportResult(qMakePair(data, newMimeTypes));
}

} // Internal
} // TextEditor


void Manager::registerMimeTypes()
{
    if (!m_registeringWatcher.isRunning()) {
        clear();

        ManagerProcessor *processor = new ManagerProcessor;
        QFuture<QPair<RegisterData, QList<Core::MimeType> > > future =
            QtConcurrent::run(&ManagerProcessor::process, processor);
        connect(&m_registeringWatcher, SIGNAL(finished()), processor, SLOT(deleteLater()));
        m_registeringWatcher.setFuture(future);

        Core::ICore::progressManager()->addTask(future,
                                                            tr("Registering definitions"),
                                                            QLatin1String(Constants::TASK_REGISTER_DEFINITIONS));
    } else {
        m_hasQueuedRegistration = true;
        m_registeringWatcher.cancel();
    }
}

void Manager::registerMimeTypesFinished()
{
    if (m_hasQueuedRegistration) {
        m_hasQueuedRegistration = false;
        registerMimeTypes();
    } else if (!m_registeringWatcher.isCanceled()) {
        const QPair<RegisterData, QList<Core::MimeType> > &result = m_registeringWatcher.result();
        m_register = result.first;

        PlainTextEditorFactory *factory = TextEditorPlugin::instance()->editorFactory();
        const QSet<QString> &inFactory = factory->mimeTypes().toSet();
        foreach (const Core::MimeType &mimeType, result.second) {
            Core::ICore::mimeDatabase()->addMimeType(mimeType);
            if (!inFactory.contains(mimeType.type()))
                factory->addMimeType(mimeType.type());
        }

        emit mimeTypesRegistered();
    }
}

QSharedPointer<HighlightDefinitionMetaData> Manager::parseMetadata(const QFileInfo &fileInfo)
{
    static const QLatin1Char kSemiColon(';');
    static const QLatin1Char kSpace(' ');
    static const QLatin1Char kDash('-');
    static const QLatin1String kLanguage("language");
    static const QLatin1String kArtificial("text/x-artificial-");

    QFile definitionFile(fileInfo.absoluteFilePath());
    if (!definitionFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return QSharedPointer<HighlightDefinitionMetaData>();

    QSharedPointer<HighlightDefinitionMetaData> metaData(new HighlightDefinitionMetaData);

    QXmlStreamReader reader(&definitionFile);
    while (!reader.atEnd() && !reader.hasError()) {
        if (reader.readNext() == QXmlStreamReader::StartElement && reader.name() == kLanguage) {
            const QXmlStreamAttributes &atts = reader.attributes();

            metaData->setFileName(fileInfo.fileName());
            metaData->setId(fileInfo.absoluteFilePath());
            metaData->setName(atts.value(HighlightDefinitionMetaData::kName).toString());
            metaData->setVersion(atts.value(HighlightDefinitionMetaData::kVersion).toString());
            metaData->setPriority(atts.value(HighlightDefinitionMetaData::kPriority).toString()
                                  .toInt());
            metaData->setPatterns(atts.value(HighlightDefinitionMetaData::kExtensions)
                                  .toString().split(kSemiColon, QString::SkipEmptyParts));

            QStringList mimeTypes = atts.value(HighlightDefinitionMetaData::kMimeType).
                                    toString().split(kSemiColon, QString::SkipEmptyParts);
            if (mimeTypes.isEmpty()) {
                // There are definitions which do not specify a MIME type, but specify file
                // patterns. Creating an artificial MIME type is a workaround.
                QString artificialType(kArtificial);
                artificialType.append(metaData->name().trimmed().replace(kSpace, kDash));
                mimeTypes.append(artificialType);
            }
            metaData->setMimeTypes(mimeTypes);

            break;
        }
    }
    reader.clear();
    definitionFile.close();

    return metaData;
}

QList<HighlightDefinitionMetaData> Manager::parseAvailableDefinitionsList(QIODevice *device) const
{
    static const QLatin1Char kSlash('/');
    static const QLatin1String kDefinition("Definition");

    QList<HighlightDefinitionMetaData> metaDataList;
    QXmlStreamReader reader(device);
    while (!reader.atEnd() && !reader.hasError()) {
        if (reader.readNext() == QXmlStreamReader::StartElement &&
            reader.name() == kDefinition) {
            const QXmlStreamAttributes &atts = reader.attributes();

            HighlightDefinitionMetaData metaData;
            metaData.setName(atts.value(HighlightDefinitionMetaData::kName).toString());
            metaData.setVersion(atts.value(HighlightDefinitionMetaData::kVersion).toString());
            QString url(atts.value(HighlightDefinitionMetaData::kUrl).toString());
            metaData.setUrl(QUrl(url));
            const int slash = url.lastIndexOf(kSlash);
            if (slash != -1)
                metaData.setFileName(url.right(url.length() - slash - 1));

            metaDataList.append(metaData);
        }
    }
    reader.clear();

    return metaDataList;
}

void Manager::downloadAvailableDefinitionsMetaData()
{
    QUrl url(QLatin1String("http://www.kate-editor.org/syntax/update-3.9.xml"));
    QNetworkRequest request(url);
    // Currently this takes a couple of seconds on Windows 7: QTBUG-10106.
    QNetworkReply *reply = Utils::NetworkAccessManager::instance()->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadAvailableDefinitionsListFinished()));
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
    m_downloaders.clear();
    foreach (const QUrl &url, urls)
        m_downloaders.append(new DefinitionDownloader(url, savePath));

    m_isDownloadingDefinitionsSpec = true;
    QFuture<void> future = QtConcurrent::map(m_downloaders, DownloaderStarter());
    m_downloadWatcher.setFuture(future);
    Core::ICore::progressManager()->addTask(future,
                                                        tr("Downloading definitions"),
                                                        QLatin1String(Constants::TASK_DOWNLOAD_DEFINITIONS));
}

void Manager::downloadDefinitionsFinished()
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
            text.append(tr("\nPlease check the directory's access rights."));
        QMessageBox::critical(0, tr("Download Error"), text);
    }

    m_isDownloadingDefinitionsSpec = false;
}

bool Manager::isDownloadingDefinitions() const
{
    return m_isDownloadingDefinitionsSpec;
}

void Manager::clear()
{
    m_register.m_idByName.clear();
    m_register.m_idByMimeType.clear();
    m_register.m_definitionsMetaData.clear();
    m_definitions.clear();
}

#include "manager.moc"
