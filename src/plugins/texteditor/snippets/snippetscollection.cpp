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
#include <QtAlgorithms>

#include <iterator>
#include <algorithm>

using namespace TextEditor;
using namespace Internal;

namespace {

struct SnippetComp
{
    bool operator()(const Snippet &a, const Snippet &b) const
    {
        const int comp = a.trigger().toLower().localeAwareCompare(b.trigger().toLower());
        if (comp < 0)
            return true;
        else if (comp == 0 &&
                 a.complement().toLower().localeAwareCompare(b.complement().toLower()) < 0)
            return true;
        return false;
    }
};
SnippetComp snippetComp;

struct RemovedSnippetPred
{
    bool operator()(const Snippet &s) const
    {
        return s.isRemoved();
    }
};
RemovedSnippetPred removedSnippetPred;

} // Anonymous

const QLatin1String SnippetsCollection::kSnippet("snippet");
const QLatin1String SnippetsCollection::kSnippets("snippets");
const QLatin1String SnippetsCollection::kTrigger("trigger");
const QLatin1String SnippetsCollection::kId("id");
const QLatin1String SnippetsCollection::kComplement("complement");
const QLatin1String SnippetsCollection::kGroup("group");
const QLatin1String SnippetsCollection::kRemoved("removed");
const QLatin1String SnippetsCollection::kModified("modified");

// Hint
SnippetsCollection::Hint::Hint(int index) : m_index(index)
{}

SnippetsCollection::Hint::Hint(int index, QList<Snippet>::iterator it) : m_index(index), m_it(it)
{}

int SnippetsCollection::Hint::index() const
{
    return m_index;
}

// SnippetsCollection
SnippetsCollection::SnippetsCollection() :
    m_snippets(Snippet::GroupSize),
    m_activeSnippetsEnd(Snippet::GroupSize),
    m_builtInSnippetsPath(QLatin1String(":/texteditor/snippets/")),
    m_userSnippetsPath(Core::ICore::instance()->userResourcePath() + QLatin1String("/snippets/")),
    m_snippetsFileName(QLatin1String("snippets.xml"))
{
    for (Snippet::Group group = Snippet::Cpp; group < Snippet::GroupSize; ++group)
        m_activeSnippetsEnd[group] = m_snippets[group].end();
}

SnippetsCollection::~SnippetsCollection()
{}

void SnippetsCollection::insertSnippet(const Snippet &snippet, Snippet::Group group)
{
    insertSnippet(snippet, group, computeInsertionHint(snippet, group));
}

void SnippetsCollection::insertSnippet(const Snippet &snippet,
                                       Snippet::Group group,
                                       const Hint &hint)
{
    if (snippet.isBuiltIn() && snippet.isRemoved()) {
        m_activeSnippetsEnd[group] = m_snippets[group].insert(m_activeSnippetsEnd[group], snippet);
    } else {
        m_snippets[group].insert(hint.m_it, snippet);
        updateActiveSnippetsEnd(group);
    }
}

SnippetsCollection::Hint SnippetsCollection::computeInsertionHint(const Snippet &snippet,
                                                                  Snippet::Group group)
{
    QList<Snippet> &snippets = m_snippets[group];
    QList<Snippet>::iterator it = qUpperBound(
        snippets.begin(), m_activeSnippetsEnd.at(group), snippet, snippetComp);
    return Hint(static_cast<int>(std::distance(snippets.begin(), it)), it);
}

void SnippetsCollection::replaceSnippet(int index, const Snippet &snippet, Snippet::Group group)
{
    replaceSnippet(index, snippet, group, computeReplacementHint(index, snippet, group));
}

void SnippetsCollection::replaceSnippet(int index,
                                        const Snippet &snippet,
                                        Snippet::Group group,
                                        const Hint &hint)
{
    Snippet replacement(snippet);
    if (replacement.isBuiltIn() && !replacement.isModified())
        replacement.setIsModified(true);

    if (index == hint.index()) {
        m_snippets[group][index] = replacement;
    } else {
        insertSnippet(replacement, group, hint);
        // Consider whether the row moved up towards the beginning or down towards the end.
        if (index < hint.index())
            m_snippets[group].removeAt(index);
        else
            m_snippets[group].removeAt(index + 1);
        updateActiveSnippetsEnd(group);
    }
}

SnippetsCollection::Hint SnippetsCollection::computeReplacementHint(int index,
                                                                    const Snippet &snippet,
                                                                    Snippet::Group group)
{
    QList<Snippet> &snippets = m_snippets[group];
    QList<Snippet>::iterator it = qLowerBound(
        snippets.begin(), m_activeSnippetsEnd.at(group), snippet, snippetComp);
    int hintIndex = static_cast<int>(std::distance(snippets.begin(), it));
    if (index < hintIndex - 1)
        return Hint(hintIndex - 1, it);
    it = qUpperBound(it, m_activeSnippetsEnd.at(group), snippet, snippetComp);
    hintIndex = static_cast<int>(std::distance(snippets.begin(), it));
    if (index > hintIndex)
        return Hint(hintIndex, it);
    // Even if the snipet is at a different index it is still inside a valid range.
    return Hint(index);
}

void SnippetsCollection::removeSnippet(int index, Snippet::Group group)
{
    Snippet snippet(m_snippets.at(group).at(index));
    m_snippets[group].removeAt(index);
    if (snippet.isBuiltIn()) {
        snippet.setIsRemoved(true);
        m_activeSnippetsEnd[group] = m_snippets[group].insert(m_activeSnippetsEnd[group], snippet);
    } else {
        updateActiveSnippetsEnd(group);
    }
}

const Snippet &SnippetsCollection::snippet(int index, Snippet::Group group) const
{
    return m_snippets.at(group).at(index);
}

void SnippetsCollection::setSnippetContent(int index, Snippet::Group group, const QString &content)
{
    Snippet &snippet = m_snippets[group][index];
    snippet.setContent(content);
    if (snippet.isBuiltIn() && !snippet.isModified())
        snippet.setIsModified(true);
}

int SnippetsCollection::totalActiveSnippets(Snippet::Group group) const
{
    return std::distance<QList<Snippet>::const_iterator>(m_snippets.at(group).begin(),
                                                         m_activeSnippetsEnd.at(group));
}

int SnippetsCollection::totalSnippets(Snippet::Group group) const
{
    return m_snippets.at(group).size();
}

void SnippetsCollection::clear()
{
    for (Snippet::Group group = Snippet::Cpp; group < Snippet::GroupSize; ++group)
        clear(group);
}

void SnippetsCollection::clear(Snippet::Group group)
{
    m_snippets[group].clear();
    m_activeSnippetsEnd[group] = m_snippets[group].end();
}

void SnippetsCollection::updateActiveSnippetsEnd(Snippet::Group group)
{
    m_activeSnippetsEnd[group] = std::find_if(m_snippets[group].begin(),
                                              m_snippets[group].end(),
                                              removedSnippetPred);
}

void SnippetsCollection::restoreRemovedSnippets(Snippet::Group group)
{
    // The version restored contains the last modifications (if any) by the user.
    // Reverting the snippet can still bring it to the original version.
    QVector<Snippet> toRestore(std::distance(m_activeSnippetsEnd[group], m_snippets[group].end()));
    qCopy(m_activeSnippetsEnd[group], m_snippets[group].end(), toRestore.begin());
    m_snippets[group].erase(m_activeSnippetsEnd[group], m_snippets[group].end());
    foreach (Snippet snippet, toRestore) {
        snippet.setIsRemoved(false);
        insertSnippet(snippet, group);
    }
}

Snippet SnippetsCollection::revertedSnippet(int index, Snippet::Group group) const
{
    const Snippet &candidate = snippet(index, group);
    Q_ASSERT(candidate.isBuiltIn());

    const QList<Snippet> &builtIn =
        readXML(m_builtInSnippetsPath + m_snippetsFileName, candidate.id());
    if (builtIn.size() == 1)
        return builtIn.at(0);
    return Snippet();
}

void SnippetsCollection::reset(Snippet::Group group)
{
    clear(group);

    const QList<Snippet> &builtInSnippets = readXML(m_builtInSnippetsPath + m_snippetsFileName);
    foreach (const Snippet &snippet, builtInSnippets)
        if (group == snippet.group())
            insertSnippet(snippet, snippet.group());
}

void SnippetsCollection::reload()
{
    clear();

    QHash<QString, Snippet> activeBuiltInSnippets;
    const QList<Snippet> &builtInSnippets = readXML(m_builtInSnippetsPath + m_snippetsFileName);
    foreach (const Snippet &snippet, builtInSnippets)
        activeBuiltInSnippets.insert(snippet.id(), snippet);

    const QList<Snippet> &userSnippets = readXML(m_userSnippetsPath + m_snippetsFileName);
    foreach (const Snippet &snippet, userSnippets) {
        if (snippet.isBuiltIn())
            // This user snippet overrides the corresponding built-in snippet.
            activeBuiltInSnippets.remove(snippet.id());
        insertSnippet(snippet, snippet.group());
    }

    foreach (const Snippet &snippet, activeBuiltInSnippets)
        insertSnippet(snippet, snippet.group());
}

void SnippetsCollection::synchronize()
{
    if (QFile::exists(m_userSnippetsPath) || QDir().mkpath(m_userSnippetsPath)) {
        QFile file(m_userSnippetsPath + m_snippetsFileName);
        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            QXmlStreamWriter writer(&file);
            writer.setAutoFormatting(true);
            writer.writeStartDocument();
            writer.writeStartElement(kSnippets);
            for (Snippet::Group group = Snippet::Cpp; group < Snippet::GroupSize; ++group) {
                const int size = totalSnippets(group);
                for (int i = 0; i < size; ++i) {
                    const Snippet &current = snippet(i, group);
                    if (!current.isBuiltIn() || current.isRemoved() || current.isModified())
                        writeSnippetXML(current, &writer);
                }
            }
            writer.writeEndElement();
            writer.writeEndDocument();
            file.close();
        }
    }

    reload();
}

void SnippetsCollection::writeSnippetXML(const Snippet &snippet, QXmlStreamWriter *writer)
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

QList<Snippet> SnippetsCollection::readXML(const QString &fileName, const QString &snippetId)
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
                        const QString &id = atts.value(kId).toString();
                        if (snippetId.isEmpty() || snippetId == id) {
                            Snippet snippet(id);
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

                            if (!snippetId.isEmpty())
                                break;
                        } else {
                            xml.skipCurrentElement();
                        }
                    } else {
                        xml.skipCurrentElement();
                    }
                }
            }
        }
        if (xml.hasError())
            qWarning() << fileName << xml.errorString() << xml.lineNumber() << xml.columnNumber();
        file.close();
    }

    return snippets;
}
