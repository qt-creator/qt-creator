// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljs_global.h>

#include <QHash>
#include <QList>
#include <QSharedPointer>

#include <utility>

QT_FORWARD_DECLARE_CLASS(QString)
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
    static bool isSame(const Ptr &trie1, const Ptr &trie2);

    static Ptr replaceF(const Ptr &trie, const QHash<QString, QString> &replacements);
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

    QStringList complete(const QString &root, const QString &base = QString(),
        LookupFlags flags = LookupFlags(CaseInsensitive|Partial)) const;
    bool contains(const QString &value, LookupFlags flags = LookupFlags(0)) const;
    QStringList stringList() const;

    Trie insertF(const QString &value) const;
    Trie intersectF(const Trie &v) const;
    Trie mergeF(const Trie &v) const;
    Trie replaceF(const QHash<QString, QString> &replacements) const;

    void insert(const QString &value);
    void intersect(const Trie &v);
    void merge(const Trie &v);
    void replace(const QHash<QString, QString> &replacements);

    bool isEmpty() const;
    bool operator==(const Trie &o);
    bool operator!=(const Trie &o);

    friend QMLJS_EXPORT QDebug operator<<(QDebug dbg, const TrieNode::Ptr &trie);
    friend QMLJS_EXPORT QDebug operator<<(QDebug dbg, const Trie &trie);

    TrieNode::Ptr trie;
};

template <typename T> void enumerateTrieNode(const TrieNode::Ptr &trie, T &t,
    QString base = QString())
{
    if (trie.isNull())
        return;
    base.append(trie->prefix);
    for (const TrieNode::Ptr &subT : std::as_const(trie->postfixes)) {
        enumerateTrieNode(subT,t,base);
    }
    if (trie->postfixes.isEmpty())
        t(base);
}

QMLJS_EXPORT int matchStrength(const QString &searchStr, const QString &str);
QMLJS_EXPORT QStringList matchStrengthSort(const QString &searchString, QStringList &res);

QMLJS_EXPORT QDebug operator<<(QDebug dbg, const TrieNode::Ptr &trie);
QMLJS_EXPORT QDebug operator<<(QDebug dbg, const Trie &trie);

} // end namespace PersistentTrie
} // end namespace QmlJS
