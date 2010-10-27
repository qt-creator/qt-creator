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

#include "snippetsmanager.h"
#include "isnippeteditordecorator.h"
#include "snippetscollection.h"
#include "reuse.h"

#include <coreplugin/icore.h>

#include <QtCore/QLatin1String>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QDebug>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

using namespace TextEditor;
using namespace Internal;

const QLatin1String SnippetsManager::kSnippet("snippet");
const QLatin1String SnippetsManager::kSnippets("snippets");
const QLatin1String SnippetsManager::kTrigger("trigger");
const QLatin1String SnippetsManager::kId("id");
const QLatin1String SnippetsManager::kComplement("complement");
const QLatin1String SnippetsManager::kGroup("group");
const QLatin1String SnippetsManager::kRemoved("removed");
const QLatin1String SnippetsManager::kModified("modified");

SnippetsManager::SnippetsManager() :
    m_collectionLoaded(false),
    m_collection(new SnippetsCollection),
    m_builtInSnippetsPath(QLatin1String(":/texteditor/snippets/")),
    m_userSnippetsPath(Core::ICore::instance()->userResourcePath() + QLatin1String("/snippets/")),
    m_snippetsFileName(QLatin1String("snippets.xml"))
{}

SnippetsManager::~SnippetsManager()
{}

SnippetsManager *SnippetsManager::instance()
{
    static SnippetsManager manager;
    return &manager;
}

void SnippetsManager::loadSnippetsCollection()
{
    QHash<QString, Snippet> activeBuiltInSnippets;
    const QList<Snippet> &builtInSnippets = readXML(m_builtInSnippetsPath + m_snippetsFileName);
    foreach (const Snippet &snippet, builtInSnippets)
        activeBuiltInSnippets.insert(snippet.id(), snippet);

    const QList<Snippet> &userSnippets = readXML(m_userSnippetsPath + m_snippetsFileName);
    foreach (const Snippet &snippet, userSnippets) {
        if (snippet.isBuiltIn())
            // This user snippet overrides the corresponding built-in snippet.
            activeBuiltInSnippets.remove(snippet.id());
        m_collection->insertSnippet(snippet, snippet.group());
    }

    foreach (const Snippet &snippet, activeBuiltInSnippets)
        m_collection->insertSnippet(snippet, snippet.group());
}

void SnippetsManager::reloadSnippetsCollection()
{
    m_collection->clear();
    loadSnippetsCollection();
}

void SnippetsManager::persistSnippetsCollection()
{
    if (QFile::exists(m_userSnippetsPath) || QDir().mkpath(m_userSnippetsPath)) {
        QFile file(m_userSnippetsPath + m_snippetsFileName);
        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            QXmlStreamWriter writer(&file);
            writer.setAutoFormatting(true);
            writer.writeStartDocument();
            writer.writeStartElement(kSnippets);
            for (Snippet::Group group = Snippet::Cpp; group < Snippet::GroupSize; ++group) {
                const int size = m_collection->totalSnippets(group);
                for (int i = 0; i < size; ++i) {
                    const Snippet &snippet = m_collection->snippet(i, group);
                    if (!snippet.isBuiltIn() ||
                       (snippet.isBuiltIn() && (snippet.isRemoved() || snippet.isModified()))) {
                        writeSnippetXML(snippet, &writer);
                    }
                }
            }
            writer.writeEndElement();
            writer.writeEndDocument();
            file.close();
        }
    }
}

void SnippetsManager::writeSnippetXML(const Snippet &snippet, QXmlStreamWriter *writer)
{
    writer->writeStartElement(kSnippet);
    writer->writeAttribute(kGroup, fromSnippetGroup(snippet.group()));
    writer->writeAttribute(kTrigger, snippet.trigger());
    writer->writeAttribute(kId, snippet.id());
    writer->writeAttribute(kComplement, snippet.complement());
    writer->writeAttribute(kRemoved, fromBool(snippet.isRemoved()));
    writer->writeAttribute(kModified, fromBool(snippet.isModified()));
    writer->writeCharacters(snippet.content());
    writer->writeEndElement();
}

QList<Snippet> SnippetsManager::readXML(const QString &fileName)
{
    QList<Snippet> snippets;
    QFile file(fileName);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QXmlStreamReader xml(&file);
        if (xml.readNextStartElement()) {
            if (xml.name() == kSnippets) {
                while (xml.readNextStartElement()) {
                    if (xml.name() == kSnippet) {
                        const QXmlStreamAttributes &atts = xml.attributes();

                        Snippet snippet(atts.value(kId).toString());
                        snippet.setTrigger(atts.value(kTrigger).toString());
                        snippet.setComplement(atts.value(kComplement).toString());
                        snippet.setGroup(toSnippetGroup(atts.value(kGroup).toString()));
                        snippet.setIsRemoved(toBool(atts.value(kRemoved).toString()));
                        snippet.setIsModified(toBool(atts.value(kModified).toString()));

                        QString content;
                        while (!xml.atEnd()) {
                            xml.readNext();
                            if (xml.isCharacters()) {
                                content += xml.text();
                            } else if (xml.isEndElement()) {
                                snippet.setContent(content);
                                snippets.append(snippet);
                                break;
                            }
                        }
                    } else {
                        xml.skipCurrentElement();
                    }
                }
            } else {
                xml.skipCurrentElement();
            }
        }
        if (xml.hasError())
            qWarning() << fileName << xml.errorString() << xml.lineNumber() << xml.columnNumber();
        file.close();
    }

    return snippets;
}

QSharedPointer<SnippetsCollection> SnippetsManager::snippetsCollection()
{
    if (!m_collectionLoaded) {
        loadSnippetsCollection();
        m_collectionLoaded = true;
    }
    return m_collection;
}
