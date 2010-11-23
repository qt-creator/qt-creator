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

#ifndef SNIPPETSCOLLECTION_H
#define SNIPPETSCOLLECTION_H

#include "snippet.h"

#include <QtCore/QVector>
#include <QtCore/QList>

QT_FORWARD_DECLARE_CLASS(QXmlStreamWriter)

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

class SnippetsCollection
{
public:
    SnippetsCollection();
    ~SnippetsCollection();

    class Hint
    {
        friend class SnippetsCollection;
    public:
        int index() const;
    private:
        Hint(int index);
        Hint(int index, QList<Snippet>::iterator it);
        int m_index;
        QList<Snippet>::iterator m_it;
    };

    void insertSnippet(const Snippet &snippet, Snippet::Group group);
    void insertSnippet(const Snippet &snippet, Snippet::Group group, const Hint &hint);
    Hint computeInsertionHint(const Snippet &snippet, Snippet::Group group);

    void replaceSnippet(int index, const Snippet &snippet, Snippet::Group group);
    void replaceSnippet(int index, const Snippet &snippet, Snippet::Group group, const Hint &hint);
    Hint computeReplacementHint(int index, const Snippet &snippet, Snippet::Group group);

    void removeSnippet(int index, Snippet::Group group);

    void setSnippetContent(int index, Snippet::Group group, const QString &content);

    const Snippet &snippet(int index, Snippet::Group group) const;

    int totalActiveSnippets(Snippet::Group group) const;
    int totalSnippets(Snippet::Group group) const;

    void reload();
    void synchronize();

private:
    void clear();
    void updateActiveSnippetsEnd(Snippet::Group group);

    static QList<Snippet> readXML(const QString &fileName);
    static void writeSnippetXML(const Snippet &snippet, QXmlStreamWriter *writer);

    static const QLatin1String kSnippet;
    static const QLatin1String kSnippets;
    static const QLatin1String kTrigger;
    static const QLatin1String kId;
    static const QLatin1String kComplement;
    static const QLatin1String kGroup;
    static const QLatin1String kRemoved;
    static const QLatin1String kModified;

    QVector<QList<Snippet> > m_snippets;
    QVector<QList<Snippet>::iterator> m_activeSnippetsEnd;

    QString m_builtInSnippetsPath;
    QString m_userSnippetsPath;
    QString m_snippetsFileName;
};

} // Internal
} // TextEditor

#endif // SNIPPETSCOLLECTION_H
