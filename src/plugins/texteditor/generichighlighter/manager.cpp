/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "manager.h"
#include "highlightdefinition.h"
#include "highlightdefinitionhandler.h"
#include "highlighterexception.h"
#include "texteditorplugin.h"
#include "texteditorsettings.h"
#include "plaintexteditorfactory.h"
#include "highlightersettings.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <qtconcurrent/QtConcurrentTools>

#include <QtCore/QtAlgorithms>
#include <QtCore/QtPlugin>
#include <QtCore/QString>
#include <QtCore/QLatin1Char>
#include <QtCore/QLatin1String>
#include <QtCore/QStringList>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QRegExp>
#include <QtCore/QFuture>
#include <QtCore/QtConcurrentRun>
#include <QtXml/QXmlSimpleReader>
#include <QtXml/QXmlInputSource>
#include <QtXml/QXmlStreamReader>
#include <QtXml/QXmlStreamAttributes>

using namespace TextEditor;
using namespace Internal;

Manager::Manager()
{}

Manager::~Manager()
{
}

Manager *Manager::instance()
{
    static Manager manager;
    return &manager;
}

QString Manager::definitionIdByName(const QString &name) const
{ return m_idByName.value(name); }

QString Manager::definitionIdByMimeType(const QString &mimeType) const
{
    if (m_idByMimeType.count(mimeType) <= 1) {
        return m_idByMimeType.value(mimeType);
    } else {
        QStringList candidateIds;
        QMultiHash<QString, QString>::const_iterator it = m_idByMimeType.find(mimeType);
        QMultiHash<QString, QString>::const_iterator endIt = m_idByMimeType.end();
        for (; it != endIt && it.key() == mimeType; ++it)
            candidateIds.append(it.value());

        qSort(candidateIds.begin(), candidateIds.end(), m_priorityComp);
        return candidateIds.last();
    }
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

const QSharedPointer<HighlightDefinition> &Manager::definition(const QString &id)
{
    if (!m_definitions.contains(id)) {
        m_isBuilding.insert(id);

        QFile definitionFile(id);
        if (!definitionFile.open(QIODevice::ReadOnly | QIODevice::Text))
            throw HighlighterException();
        QXmlInputSource source(&definitionFile);

        QSharedPointer<HighlightDefinition> definition(new HighlightDefinition);
        HighlightDefinitionHandler handler(definition);

        QXmlSimpleReader reader;
        reader.setContentHandler(&handler);
        reader.parse(source);

        m_definitions.insert(id, definition);
        definitionFile.close();
        m_isBuilding.remove(id);
    }

    return *m_definitions.constFind(id);
}

bool Manager::isBuildingDefinition(const QString &id) const
{ return m_isBuilding.contains(id); }

void Manager::registerMimeTypes()
{
    clear();
    QFuture<Core::MimeType> future =
            QtConcurrent::run(&Manager::gatherDefinitionsMimeTypes, this);
    m_watcher.setFuture(future);

    connect(&m_watcher, SIGNAL(resultReadyAt(int)), this, SLOT(registerMimeType(int)));
}

void Manager::gatherDefinitionsMimeTypes(QFutureInterface<Core::MimeType> &future)
{
    const HighlighterSettings &settings = TextEditorSettings::instance()->highlighterSettings();
    QDir definitionsDir(settings.m_definitionFilesPath);

    QStringList filter(QLatin1String("*.xml"));
    definitionsDir.setNameFilters(filter);

    const QFileInfoList &filesInfo = definitionsDir.entryInfoList();
    foreach (const QFileInfo &fileInfo, filesInfo) {
        QString comment;
        QStringList mimeTypes;
        QStringList patterns;
        parseDefinitionMetadata(fileInfo, &comment, &mimeTypes, &patterns);

        // A definition can specify multiple MIME types and file extensions/patterns. However, each
        // thing is done with a single string. Then, there is no direct way to tell which patterns
        // belong to which MIME types nor whether a MIME type is just an alias for the other.
        // Currently, I associate all expressions/patterns with all MIME types from a definition.

        static const QStringList textPlain(QLatin1String("text/plain"));

        QList<QRegExp> expressions;
        foreach (const QString &type, mimeTypes) {
            Core::MimeType mimeType = Core::ICore::instance()->mimeDatabase()->findByType(type);
            if (mimeType.isNull()) {
                if (expressions.isEmpty()) {
                    foreach (const QString &pattern, patterns)
                        expressions.append(QRegExp(pattern, Qt::CaseSensitive, QRegExp::Wildcard));
                }

                mimeType.setType(type);
                mimeType.setSubClassesOf(textPlain);
                mimeType.setComment(comment);
                mimeType.setGlobPatterns(expressions);

                future.reportResult(mimeType);
            }
        }
    }
}

void Manager::registerMimeType(int index) const
{
    const Core::MimeType &mimeType = m_watcher.resultAt(index);
    Core::ICore::instance()->mimeDatabase()->addMimeType(mimeType);
    TextEditorPlugin::instance()->editorFactory()->addMimeType(mimeType.type());
}

void Manager::parseDefinitionMetadata(const QFileInfo &fileInfo,
                                                  QString *comment,
                                                  QStringList *mimeTypes,
                                                  QStringList *patterns)
{
    static const QLatin1Char kSemiColon(';');
    static const QLatin1Char kSlash('/');
    static const QLatin1String kLanguage("language");
    static const QLatin1String kName("name");
    static const QLatin1String kExtensions("extensions");
    static const QLatin1String kMimeType("mimetype");
    static const QLatin1String kPriority("priority");
    static const QLatin1String kArtificial("artificial");

    const QString &id = fileInfo.absoluteFilePath();

    QFile definitionFile(id);
    if (!definitionFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QXmlStreamReader reader(&definitionFile);
    while (!reader.atEnd() && !reader.hasError()) {
        if (reader.readNext() == QXmlStreamReader::StartElement &&
            reader.name() == kLanguage) {
            const QXmlStreamAttributes &attr = reader.attributes();

            *comment = attr.value(kName).toString();
            m_idByName.insert(*comment, id);

            *patterns = attr.value(kExtensions).toString().split(kSemiColon,
                                                                 QString::SkipEmptyParts);

            *mimeTypes = attr.value(kMimeType).toString().split(kSemiColon,
                                                                QString::SkipEmptyParts);
            if (mimeTypes->isEmpty()) {
                // There are definitions which do not specify a MIME type, but specify file
                // patterns. Creating an artificial MIME type is a workaround.
                QString artificialType(kArtificial);
                artificialType.append(kSlash).append(*comment);
                m_idByMimeType.insert(artificialType, id);
                mimeTypes->append(artificialType);
            } else {
                foreach (const QString &type, *mimeTypes)
                    m_idByMimeType.insert(type, id);
            }

            // The priority below should not be confused with the priority used when matching files
            // to MIME types. Kate uses this when there are definitions which share equal
            // extensions/patterns. Here it is for choosing a highlight definition if there are
            // multiple ones associated with the same MIME type (should not happen in general).
            m_priorityComp.m_priorityById.insert(id, attr.value(kPriority).toString().toInt());

            break;
        }
    }
    reader.clear();
    definitionFile.close();
}

void Manager::clear()
{
    m_priorityComp.m_priorityById.clear();
    m_idByName.clear();
    m_idByMimeType.clear();
    m_definitions.clear();
}
