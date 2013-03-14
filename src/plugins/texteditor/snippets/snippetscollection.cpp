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

#include "snippetscollection.h"
#include "isnippetprovider.h"
#include "reuse.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/fileutils.h>

#include <QLatin1String>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QXmlStreamReader>
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

SnippetsCollection *SnippetsCollection::instance()
{
    static SnippetsCollection collection;
    return &collection;
}

// SnippetsCollection
SnippetsCollection::SnippetsCollection() :
    m_userSnippetsPath(Core::ICore::userResourcePath() + QLatin1String("/snippets/")),
    m_userSnippetsFile(QLatin1String("snippets.xml"))
{
    QDir dir(Core::ICore::resourcePath() + QLatin1String("/snippets/"));
    dir.setNameFilters(QStringList(QLatin1String("*.xml")));
    foreach (const QFileInfo &fi, dir.entryInfoList())
        m_builtInSnippetsFiles.append(fi.absoluteFilePath());

    connect(Core::ICore::instance(), SIGNAL(coreOpened()), this, SLOT(identifyGroups()));
}

SnippetsCollection::~SnippetsCollection()
{}

void SnippetsCollection::insertSnippet(const Snippet &snippet)
{
    insertSnippet(snippet, computeInsertionHint(snippet));
}

void SnippetsCollection::insertSnippet(const Snippet &snippet, const Hint &hint)
{
    const int group = groupIndex(snippet.groupId());
    if (snippet.isBuiltIn() && snippet.isRemoved()) {
        m_activeSnippetsEnd[group] = m_snippets[group].insert(m_activeSnippetsEnd[group], snippet);
    } else {
        m_snippets[group].insert(hint.m_it, snippet);
        updateActiveSnippetsEnd(group);
    }
}

SnippetsCollection::Hint SnippetsCollection::computeInsertionHint(const Snippet &snippet)
{
    const int group = groupIndex(snippet.groupId());
    QList<Snippet> &snippets = m_snippets[group];
    QList<Snippet>::iterator it = qUpperBound(
        snippets.begin(), m_activeSnippetsEnd.at(group), snippet, snippetComp);
    return Hint(static_cast<int>(std::distance(snippets.begin(), it)), it);
}

void SnippetsCollection::replaceSnippet(int index, const Snippet &snippet)
{
    replaceSnippet(index, snippet, computeReplacementHint(index, snippet));
}

void SnippetsCollection::replaceSnippet(int index, const Snippet &snippet, const Hint &hint)
{
    const int group = groupIndex(snippet.groupId());
    Snippet replacement(snippet);
    if (replacement.isBuiltIn() && !replacement.isModified())
        replacement.setIsModified(true);

    if (index == hint.index()) {
        m_snippets[group][index] = replacement;
    } else {
        insertSnippet(replacement, hint);
        // Consider whether the row moved up towards the beginning or down towards the end.
        if (index < hint.index())
            m_snippets[group].removeAt(index);
        else
            m_snippets[group].removeAt(index + 1);
        updateActiveSnippetsEnd(group);
    }
}

SnippetsCollection::Hint SnippetsCollection::computeReplacementHint(int index,
                                                                    const Snippet &snippet)
{
    const int group = groupIndex(snippet.groupId());
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

void SnippetsCollection::removeSnippet(int index, const QString &groupId)
{
    const int group = groupIndex(groupId);
    Snippet snippet(m_snippets.at(group).at(index));
    m_snippets[group].removeAt(index);
    if (snippet.isBuiltIn()) {
        snippet.setIsRemoved(true);
        m_activeSnippetsEnd[group] = m_snippets[group].insert(m_activeSnippetsEnd[group], snippet);
    } else {
        updateActiveSnippetsEnd(group);
    }
}

const Snippet &SnippetsCollection::snippet(int index, const QString &groupId) const
{
    return m_snippets.at(groupIndex(groupId)).at(index);
}

void SnippetsCollection::setSnippetContent(int index,
                                           const QString &groupId,
                                           const QString &content)
{
    Snippet &snippet = m_snippets[groupIndex(groupId)][index];
    snippet.setContent(content);
    if (snippet.isBuiltIn() && !snippet.isModified())
        snippet.setIsModified(true);
}

int SnippetsCollection::totalActiveSnippets(const QString &groupId) const
{
    const int group = groupIndex(groupId);
    return std::distance<QList<Snippet>::const_iterator>(m_snippets.at(group).begin(),
                                                         m_activeSnippetsEnd.at(group));
}

int SnippetsCollection::totalSnippets(const QString &groupId) const
{
    return m_snippets.at(groupIndex(groupId)).size();
}

QList<QString> SnippetsCollection::groupIds() const
{
    return m_groupIndexById.keys();
}

void SnippetsCollection::clearSnippets()
{
    for (int group = 0; group < m_groupIndexById.size(); ++group)
        clearSnippets(group);
}

void SnippetsCollection::clearSnippets(int groupIndex)
{
    m_snippets[groupIndex].clear();
    m_activeSnippetsEnd[groupIndex] = m_snippets[groupIndex].end();
}

void SnippetsCollection::updateActiveSnippetsEnd(int groupIndex)
{
    m_activeSnippetsEnd[groupIndex] = std::find_if(m_snippets[groupIndex].begin(),
                                                   m_snippets[groupIndex].end(),
                                                   removedSnippetPred);
}

void SnippetsCollection::restoreRemovedSnippets(const QString &groupId)
{
    // The version restored contains the last modifications (if any) by the user.
    // Reverting the snippet can still bring it to the original version
    const int group = groupIndex(groupId);
    QVector<Snippet> toRestore(std::distance(m_activeSnippetsEnd[group], m_snippets[group].end()));
    qCopy(m_activeSnippetsEnd[group], m_snippets[group].end(), toRestore.begin());
    m_snippets[group].erase(m_activeSnippetsEnd[group], m_snippets[group].end());
    foreach (Snippet snippet, toRestore) {
        snippet.setIsRemoved(false);
        insertSnippet(snippet);
    }
}

Snippet SnippetsCollection::revertedSnippet(int index, const QString &groupId) const
{
    const Snippet &candidate = snippet(index, groupId);
    Q_ASSERT(candidate.isBuiltIn());

    foreach (const QString &fileName, m_builtInSnippetsFiles) {
        const QList<Snippet> &builtIn = readXML(fileName, candidate.id());
        if (builtIn.size() == 1)
            return builtIn.at(0);
    }
    return Snippet(groupId);
}

void SnippetsCollection::reset(const QString &groupId)
{
    clearSnippets(groupIndex(groupId));

    const QList<Snippet> &builtInSnippets = allBuiltInSnippets();
    foreach (const Snippet &snippet, builtInSnippets)
        if (groupId == snippet.groupId())
            insertSnippet(snippet);
}

void SnippetsCollection::reload()
{
    clearSnippets();

    const QList<Snippet> &builtInSnippets = allBuiltInSnippets();
    QHash<QString, Snippet> activeBuiltInSnippets;
    foreach (const Snippet &snippet, builtInSnippets)
        activeBuiltInSnippets.insert(snippet.id(), snippet);

    const QList<Snippet> &userSnippets = readXML(m_userSnippetsPath + m_userSnippetsFile);
    foreach (const Snippet &snippet, userSnippets) {
        if (snippet.isBuiltIn())
            // This user snippet overrides the corresponding built-in snippet.
            activeBuiltInSnippets.remove(snippet.id());
        insertSnippet(snippet);
    }

    foreach (const Snippet &snippet, activeBuiltInSnippets)
        insertSnippet(snippet);
}

bool SnippetsCollection::synchronize(QString *errorString)
{
    if (!QFile::exists(m_userSnippetsPath) && !QDir().mkpath(m_userSnippetsPath)) {
        *errorString = tr("Cannot create user snippet directory %1").arg(
                QDir::toNativeSeparators(m_userSnippetsPath));
        return false;
    }
    Utils::FileSaver saver(m_userSnippetsPath + m_userSnippetsFile);
    if (!saver.hasError()) {
        typedef QHash<QString, int>::ConstIterator GroupIndexByIdConstIt;

        QXmlStreamWriter writer(saver.file());
        writer.setAutoFormatting(true);
        writer.writeStartDocument();
        writer.writeStartElement(kSnippets);
        const GroupIndexByIdConstIt cend = m_groupIndexById.constEnd();
        for (GroupIndexByIdConstIt it = m_groupIndexById.constBegin(); it != cend; ++it ) {
            const QString &groupId = it.key();
            const int size = m_snippets.at(groupIndex(groupId)).size();
            for (int i = 0; i < size; ++i) {
                const Snippet &current = snippet(i, groupId);
                if (!current.isBuiltIn() || current.isRemoved() || current.isModified())
                    writeSnippetXML(current, &writer);
            }
        }
        writer.writeEndElement();
        writer.writeEndDocument();

        saver.setResult(&writer);
    }
    if (!saver.finalize(errorString))
        return false;

    reload();
    return true;
}

void SnippetsCollection::writeSnippetXML(const Snippet &snippet, QXmlStreamWriter *writer) const
{
    writer->writeStartElement(kSnippet);
    writer->writeAttribute(kGroup, snippet.groupId());
    writer->writeAttribute(kTrigger, snippet.trigger());
    writer->writeAttribute(kId, snippet.id());
    writer->writeAttribute(kComplement, snippet.complement());
    writer->writeAttribute(kRemoved, fromBool(snippet.isRemoved()));
    writer->writeAttribute(kModified, fromBool(snippet.isModified()));
    writer->writeCharacters(snippet.content());
    writer->writeEndElement();
}

QList<Snippet> SnippetsCollection::readXML(const QString &fileName, const QString &snippetId) const
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
                        const QString &groupId = atts.value(kGroup).toString();
                        if (isGroupKnown(groupId) && (snippetId.isEmpty() || snippetId == id)) {
                            Snippet snippet(groupId, id);
                            snippet.setTrigger(atts.value(kTrigger).toString());
                            snippet.setComplement(atts.value(kComplement).toString());
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

QList<Snippet> SnippetsCollection::allBuiltInSnippets() const
{
    QList<Snippet> builtInSnippets;
    foreach (const QString &fileName, m_builtInSnippetsFiles)
        builtInSnippets.append(readXML(fileName));
    return builtInSnippets;
}

int SnippetsCollection::groupIndex(const QString &groupId) const
{
    return m_groupIndexById.value(groupId);
}

void SnippetsCollection::identifyGroups()
{
    const QList<ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::getObjects<ISnippetProvider>();
    foreach (ISnippetProvider *provider, providers) {
        const int groupIndex = m_groupIndexById.size();
        m_groupIndexById.insert(provider->groupId(), groupIndex);
        m_snippets.resize(groupIndex + 1);
        m_activeSnippetsEnd.resize(groupIndex + 1);
        m_activeSnippetsEnd[groupIndex] = m_snippets[groupIndex].end();
    }

    reload();
}

bool SnippetsCollection::isGroupKnown(const QString &groupId) const
{
    return m_groupIndexById.value(groupId, -1) != -1;
}
