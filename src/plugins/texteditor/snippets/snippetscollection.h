// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "snippet.h"

#include <utils/filepath.h>

#include <QVector>
#include <QStringList>
#include <QHash>
#include <QXmlStreamWriter>

namespace TextEditor {
namespace Internal {

// Characteristics of this collection:
// - Store snippets by group.
// - Keep groups of snippets sorted.
// - Allow snippet insertion/replacement based on a hint.
// - Allow modification of snippet members that are not sorting keys.
// - Track removed/modified built-in snippets.
// - Provide fast index access.
// - Not thread-safe.

class SnippetsCollection : public QObject
{
    Q_OBJECT
public:
    ~SnippetsCollection() override;

    static SnippetsCollection *instance();

    class Hint
    {
        friend class SnippetsCollection;
    public:
        int index() const;
    private:
        explicit Hint(int index);
        Hint(int index, QVector<Snippet>::iterator it);
        int m_index;
        QVector<Snippet>::iterator m_it;
    };

    void insertSnippet(const Snippet &snippet);
    void insertSnippet(const Snippet &snippet, const Hint &hint);
    Hint computeInsertionHint(const Snippet &snippet);

    // Replace snippets only within the same group.
    void replaceSnippet(int index, const Snippet &snippet);
    void replaceSnippet(int index, const Snippet &snippet, const Hint &hint);
    Hint computeReplacementHint(int index, const Snippet &snippet);

    void removeSnippet(int index, const QString &groupId);
    void restoreRemovedSnippets(const QString &groupId);

    void setSnippetContent(int index, const QString &groupId, const QString &content);

    const Snippet &snippet(int index, const QString &groupId) const;
    Snippet revertedSnippet(int index, const QString &groupId) const;

    void reset(const QString &groupId);

    int totalActiveSnippets(const QString &groupId) const;
    int totalSnippets(const QString &groupId) const;

    QList<QString> groupIds() const;

    void reload();
    bool synchronize(QString *errorString);

private:
    void identifyGroups();

    SnippetsCollection();

    int groupIndex(const QString &groupId) const;
    bool isGroupKnown(const QString &groupId) const;

    void clearSnippets();
    void clearSnippets(int groupIndex);

    void updateActiveSnippetsEnd(int groupIndex);

    QList<Snippet> readXML(const Utils::FilePath &fileName, const QString &snippetId = {}) const;
    void writeSnippetXML(const Snippet &snippet, QXmlStreamWriter *writer) const;

    QList<Snippet> allBuiltInSnippets() const;

    // Built-in snippets are specified in XMLs distributed in a system's folder. Snippets
    // created or modified/removed (if they are built-ins) by the user are stored in user's
    // folder.
    const Utils::FilePath m_userSnippetsFile;
    const Utils::FilePaths m_builtInSnippetsFiles;

    // Snippets for each group are kept in a list. However, not all of them are necessarily
    // active. Specifically, removed built-in snippets are kept as the last ones (for each
    // group there is a iterator that marks the logical end).
    QVector<QVector<Snippet> > m_snippets;
    QVector<int> m_activeSnippetsCount;

    QHash<QString, int> m_groupIndexById;
};

} // Internal
} // TextEditor
