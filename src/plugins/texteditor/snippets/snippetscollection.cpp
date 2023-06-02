// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "snippetscollection.h"

#include "snippetprovider.h"
#include "reuse.h"
#include "../texteditortr.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QXmlStreamReader>

#include <iterator>
#include <algorithm>

using namespace Utils;

namespace TextEditor {
namespace Internal {

/*  TRANSLATOR TextEditor::Internal::Snippets

    Snippets are text fragments that can be inserted into an editor via the usual completion
    mechanics using a trigger text. The translated text (trigger variant) is used to
    disambiguate between snippets with the same trigger.
*/

static bool snippetComp(const Snippet &a, const Snippet &b)
{
    const int comp = a.trigger().toLower().localeAwareCompare(b.trigger().toLower());
    if (comp < 0)
        return true;
    else if (comp == 0 &&
             a.complement().toLower().localeAwareCompare(b.complement().toLower()) < 0)
        return true;
    return false;
}

const QLatin1String kSnippet("snippet");
const QLatin1String kSnippets("snippets");
const QLatin1String kTrigger("trigger");
const QLatin1String kId("id");
const QLatin1String kComplement("complement");
const QLatin1String kGroup("group");
const QLatin1String kRemoved("removed");
const QLatin1String kModified("modified");

// Hint
SnippetsCollection::Hint::Hint(int index) : m_index(index)
{}

SnippetsCollection::Hint::Hint(int index, QVector<Snippet>::iterator it) : m_index(index), m_it(it)
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
SnippetsCollection::SnippetsCollection()
    : m_userSnippetsFile(Core::ICore::userResourcePath("snippets/snippets.xml")),
      m_builtInSnippetsFiles(Core::ICore::resourcePath("snippets")
        .dirEntries(FileFilter({"*.xml"})))
{
    identifyGroups();
}

SnippetsCollection::~SnippetsCollection() = default;

void SnippetsCollection::insertSnippet(const Snippet &snippet)
{
    insertSnippet(snippet, computeInsertionHint(snippet));
}

void SnippetsCollection::insertSnippet(const Snippet &snippet, const Hint &hint)
{
    const int group = groupIndex(snippet.groupId());
    if (snippet.isBuiltIn() && snippet.isRemoved()) {
        m_snippets[group].append(snippet);
    } else {
        m_snippets[group].insert(hint.m_it, snippet);
        updateActiveSnippetsEnd(group);
    }
}

SnippetsCollection::Hint SnippetsCollection::computeInsertionHint(const Snippet &snippet)
{
    const int group = groupIndex(snippet.groupId());
    QVector<Snippet> &snippets = m_snippets[group];
    QVector<Snippet>::iterator it = std::upper_bound(snippets.begin(),
                                                   snippets.begin()
                                                       + m_activeSnippetsCount.at(group),
                                                   snippet,
                                                   snippetComp);
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
    QVector<Snippet> &snippets = m_snippets[group];
    auto activeSnippetsEnd = snippets.begin() + m_activeSnippetsCount.at(group);
    QVector<Snippet>::iterator it = std::lower_bound(snippets.begin(),
                                                     activeSnippetsEnd,
                                                     snippet,
                                                     snippetComp);
    int hintIndex = static_cast<int>(std::distance(snippets.begin(), it));
    if (index < hintIndex - 1)
        return Hint(hintIndex - 1, it);
    it = std::upper_bound(it, activeSnippetsEnd, snippet, snippetComp);
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
    updateActiveSnippetsEnd(group);
    if (snippet.isBuiltIn()) {
        snippet.setIsRemoved(true);
        m_snippets[group].append(snippet);
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
    return m_activeSnippetsCount.at(group);
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
    m_activeSnippetsCount[groupIndex] = m_snippets[groupIndex].size();
}

void SnippetsCollection::updateActiveSnippetsEnd(int groupIndex)
{
    const int index = Utils::indexOf(m_snippets[groupIndex],
                                     [](const Snippet &s) { return s.isRemoved(); });
    m_activeSnippetsCount[groupIndex] = index < 0 ? m_snippets[groupIndex].size() : index;
}

void SnippetsCollection::restoreRemovedSnippets(const QString &groupId)
{
    // The version restored contains the last modifications (if any) by the user.
    // Reverting the snippet can still bring it to the original version
    const int group = groupIndex(groupId);
    if (m_activeSnippetsCount[group] == m_snippets[group].size()) // no removed snippets
        return;
    const QVector<Snippet> toRestore = m_snippets[group].mid(m_activeSnippetsCount[group]);
    m_snippets[group].resize(m_activeSnippetsCount[group]);

    for (Snippet snippet : std::as_const(toRestore)) {
        snippet.setIsRemoved(false);
        insertSnippet(snippet);
    }
}

Snippet SnippetsCollection::revertedSnippet(int index, const QString &groupId) const
{
    const Snippet &candidate = snippet(index, groupId);
    Q_ASSERT(candidate.isBuiltIn());

    for (const FilePath &fileName : m_builtInSnippetsFiles) {
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
    for (const Snippet &snippet : builtInSnippets)
        if (groupId == snippet.groupId())
            insertSnippet(snippet);
}

void SnippetsCollection::reload()
{
    clearSnippets();

    const QList<Snippet> &builtInSnippets = allBuiltInSnippets();
    QHash<QString, Snippet> activeBuiltInSnippets;
    for (const Snippet &snippet : builtInSnippets)
        activeBuiltInSnippets.insert(snippet.id(), snippet);

    const QList<Snippet> &userSnippets = readXML(m_userSnippetsFile);
    for (const Snippet &snippet : userSnippets) {
        if (snippet.isBuiltIn())
            // This user snippet overrides the corresponding built-in snippet.
            activeBuiltInSnippets.remove(snippet.id());
        insertSnippet(snippet);
    }

    for (const Snippet &snippet : std::as_const(activeBuiltInSnippets))
        insertSnippet(snippet);
}

bool SnippetsCollection::synchronize(QString *errorString)
{
    if (!m_userSnippetsFile.parentDir().ensureWritableDir()) {
        *errorString = Tr::tr("Cannot create user snippet directory %1")
                .arg(m_userSnippetsFile.parentDir().toUserOutput());
        return false;
    }
    FileSaver saver(m_userSnippetsFile);
    if (!saver.hasError()) {
        using GroupIndexByIdConstIt = QHash<QString, int>::ConstIterator;

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

QList<Snippet> SnippetsCollection::readXML(const FilePath &fileName, const QString &snippetId) const
{
    QList<Snippet> snippets;
    QFile file(fileName.toString());
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QXmlStreamReader xml(&file);
        if (xml.readNextStartElement()) {
            if (xml.name() == kSnippets) {
                while (xml.readNextStartElement()) {
                    if (xml.name() == kSnippet) {
                        const QXmlStreamAttributes &atts = xml.attributes();
                        const QString &id = atts.value(kId).toString();
                        const QString &groupId = atts.value(kGroup).toString();
                        const QString &trigger = atts.value(kTrigger).toString();
                        if (!Snippet::isValidTrigger(trigger)) {
                            qWarning()
                                << fileName << "ignore snippet for invalid trigger" << trigger
                                << "A valid trigger can only contain letters, "
                                   "numbers, or underscores, where the first character is "
                                   "limited to letter or underscore.";

                            xml.skipCurrentElement();
                        } else if (isGroupKnown(groupId) && (snippetId.isEmpty() || snippetId == id)) {
                            Snippet snippet(groupId, id);
                            snippet.setTrigger(trigger);
                            snippet.setComplement(Tr::tr(
                                                      atts.value(kComplement).toString().toLatin1(),
                                                      atts.value(kId).toString().toLatin1()));
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
    for (const FilePath &fileName : m_builtInSnippetsFiles)
        builtInSnippets.append(readXML(fileName));
    return builtInSnippets;
}

int SnippetsCollection::groupIndex(const QString &groupId) const
{
    return m_groupIndexById.value(groupId);
}

void SnippetsCollection::identifyGroups()
{
    for (const SnippetProvider &provider : SnippetProvider::snippetProviders()) {
        const int groupIndex = m_groupIndexById.size();
        m_groupIndexById.insert(provider.groupId(), groupIndex);
        m_snippets.resize(groupIndex + 1);
        m_activeSnippetsCount.resize(groupIndex + 1);
        m_activeSnippetsCount[groupIndex] = m_snippets[groupIndex].size();
    }

    reload();
}

bool SnippetsCollection::isGroupKnown(const QString &groupId) const
{
    return m_groupIndexById.value(groupId, -1) != -1;
}

} // Internal
} // TextEditor
