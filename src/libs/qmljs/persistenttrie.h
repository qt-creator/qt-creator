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

#ifndef PERSISTENTTRIE_H
#define PERSISTENTTRIE_H

#include <qmljs/qmljs_global.h>

#include <QList>
#include <QSharedPointer>

#include <utility>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QStringList)
QT_FORWARD_DECLARE_CLASS(QDebug)

namespace QmlJS {
namespace PersistentTrie {

enum LookupFlags {
    CaseInsensitive = 0x1,
    Partial         = 0x2,
    SkipChars       = 0x4,
    SkipSpaces      = 0x8
};

class QMLJS_EXPORT TrieNode
{
public:
    typedef const TrieNode CTrie;
    typedef QSharedPointer<CTrie> Ptr;
    QString prefix;
    QList<Ptr> postfixes;

    TrieNode(const QString &pre = QString(), QList<Ptr> post = QList<Ptr>());
    TrieNode(const TrieNode &o);
    static Ptr create(const QString &pre = QString(), QList<Ptr> post = QList<Ptr>());

    static void complete(QStringList &results, const Ptr &trie, const QString &root,
                         const QString &base = QString(), LookupFlags flags = LookupFlags(CaseInsensitive|Partial));
    static bool contains(const Ptr &trie, const QString &value, LookupFlags flags = LookupFlags(0));
    static QStringList stringList(const Ptr &trie);

    static Ptr insertF(const Ptr &trie, const QString &value);
    static std::pair<Ptr,int> intersectF(const Ptr &v1, const Ptr &v2, int index1=0);
    static std::pair<Ptr,int> mergeF(const Ptr &v1, const Ptr &v2);

    static QDebug &printStrings(QDebug &dbg, const Ptr &trie);
    static QDebug &describe(QDebug &dbg, const Ptr &trie, int indent);
};

class QMLJS_EXPORT Trie
{
public:
    Trie();
    Trie(const TrieNode::Ptr &t);
    Trie(const Trie &o);

    QStringList complete(const QString &root, const QString &base = QString(),
        LookupFlags flags = LookupFlags(CaseInsensitive|Partial)) const;
    bool contains(const QString &value, LookupFlags flags = LookupFlags(0)) const;
    QStringList stringList() const;

    Trie insertF(const QString &value) const;
    Trie intersectF(const Trie &v) const;
    Trie mergeF(const Trie &v) const;

    void insert(const QString &value);
    void intersect(const Trie &v);
    void merge(const Trie &v);

    friend QMLJS_EXPORT QDebug &operator<<(QDebug &dbg, const TrieNode::Ptr &trie);
    friend QMLJS_EXPORT QDebug &operator<<(QDebug &dbg, const Trie &trie);

    TrieNode::Ptr trie;
};

template <typename T> void enumerateTrieNode(const TrieNode::Ptr &trie, T &t,
    QString base = QString())
{
    if (trie.isNull())
        return;
    base.append(trie->prefix);
    foreach (const TrieNode::Ptr subT, trie->postfixes) {
        enumerateTrieNode(subT,t,base);
    }
    if (trie->postfixes.isEmpty())
        t(base);
}

QMLJS_EXPORT int matchStrength(const QString &searchStr, const QString &str);
QMLJS_EXPORT QStringList matchStrengthSort(const QString &searchString, QStringList &res);

QMLJS_EXPORT QDebug &operator<<(QDebug &dbg, const TrieNode::Ptr &trie);
QMLJS_EXPORT QDebug &operator<<(QDebug &dbg, const Trie &trie);

} // end namespace PersistentTrie
} // end namespace QmlJS

#endif // PERSISTENTTRIE_H
