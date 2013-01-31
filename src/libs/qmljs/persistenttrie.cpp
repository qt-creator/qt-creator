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

/*!
  \class QmlJS::PersistentTrie::Trie

  \brief Implements a trie that is persistent (not on disk but in memory).

  This means that several versions can coexist, as adding an element
  is non destructive, and as much as possible is shared.

  The trie is only *partially* ordered, it preserves the order
  of what was inserted as much as possible.
  This makes some operations a bit slower, but is considered
  a feature.
  This means the order in which you insert the elements matters.

  An important use case for this is completions, and several
  strategies are available.
  Results order can be improved using the matching strength

  All const operations are threadsafe, and copy is cheap (only a
  QSharedPointer copy).

  Assigning a shared pointer is *not* threadsafe, so updating the
  head is *not* thread safe, and should be done only on a local
  instance (shared pointer used only from a single thread), or
  protected with locks.

  This is a two level implementation, based on a fully functional
  implementation (PersistentTrie::Trie), which could be private
  but was left public because deemed useful.

  Would gain from some memory optimization, or direct string implementation.
 */

#include "persistenttrie.h"

#include <QDebug>
#include <QMap>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <utility>

namespace QmlJS {
namespace PersistentTrie {

TrieNode::TrieNode(const QString &pre, QList<TrieNode::Ptr> post) :
    prefix(pre), postfixes(post) {}

TrieNode::TrieNode(const TrieNode &o) : prefix(o.prefix),
    postfixes(o.postfixes) {}

TrieNode::Ptr TrieNode::create(const QString &pre, QList<TrieNode::Ptr> post)
{
    return TrieNode::Ptr(new TrieNode(pre,post));
}

void TrieNode::complete(QStringList &res, const TrieNode::Ptr &trie,
    const QString &value, const QString &base, LookupFlags flags)
{
    // one could also modify this and make it return a TrieNode, instead of a QStringList
    if (trie.isNull())
        return;
    QString::const_iterator i = trie->prefix.constBegin(), iEnd = trie->prefix.constEnd();
    QString::const_iterator j = value.constBegin(), jEnd = value.constEnd();
    while (i != iEnd && j != jEnd) {
        if (i->isSpace()) {
            if (! j->isSpace() && (flags & SkipSpaces) == 0)
                return;
            while (j != jEnd && j->isSpace()) {
                ++j;
            } ;
            do {
                ++i;
            } while (i != iEnd && i->isSpace());
        } else {
            if (*i != *j && ((flags & CaseInsensitive) == 0 || i->toLower() != j->toLower())) {
                if ((flags & SkipChars) != 0)
                    --j;
                else
                    return;
            }
            ++i;
            ++j;
        }
    }
    QString base2 = base + trie->prefix;
    if (j == jEnd) {
        if (trie->postfixes.isEmpty())
            res.append(base2);
        if (trie->postfixes.size() == 1) {
            complete(res, trie->postfixes[0],QString(), base2, flags);
            return;
        }
        foreach (TrieNode::Ptr t, trie->postfixes) {
            if ((flags & Partial) != 0)
                res.append(base2 + t->prefix);
            else
                complete(res, t, QString(), base2, flags);
        }
        return;
    }
    foreach (const TrieNode::Ptr v, trie->postfixes) {
        QString::const_iterator vi = v->prefix.constBegin(), vEnd = v->prefix.constEnd();
        if (vi != vEnd && (*vi == *j || ((flags & CaseInsensitive) != 0
            && vi->toLower() == j->toLower()) || ((flags & SkipChars) != 0)))
            complete(res, v, value.right(jEnd-j), base2, flags);
    }
}

TrieNode::Ptr TrieNode::insertF(const TrieNode::Ptr &trie,
    const QString &value)
{
    if (trie.isNull()) {
        if (value.isEmpty())
            return trie;
        return TrieNode::create(value);
    }
    typedef TrieNode T;
    QString::const_iterator i = trie->prefix.constBegin(), iEnd = trie->prefix.constEnd();
    QString::const_iterator j = value.constBegin(), jEnd = value.constEnd();
    while (i != iEnd && j != jEnd) {
        if (i->isSpace()) {
            if (! j->isSpace())
                break;
            do {
                ++j;
            } while (j != jEnd && j->isSpace());
            do {
                ++i;
            } while (i != iEnd && i->isSpace());
        } else {
            if (*i != *j)
                break;
            ++i;
            ++j;
        }
    }
    if (i == iEnd) {
        if (j == jEnd)
            return trie;
        int tSize = trie->postfixes.size();
        for (int i=0; i < tSize; ++i) {
            const T::Ptr &v=trie->postfixes[i];
            QString::const_iterator vi = v->prefix.constBegin(), vEnd = v->prefix.constEnd();
            if (vi != vEnd && *vi == *j) {
                T::Ptr res = insertF(v, value.right(jEnd-j));
                if (res != v) {
                    QList<T::Ptr> post = trie->postfixes;
                    post.replace(i, res);
                    return T::create(trie->prefix,post);
                } else {
                    return trie;
                }
            }
        }
        QList<T::Ptr> post = trie->postfixes;
        if (post.isEmpty())
            post.append(T::create());
        post.append(T::create(value.right(jEnd - j)));
        return T::create(trie->prefix, post);
    } else {
        T::Ptr newTrie1 = T::create(trie->prefix.right(iEnd - i), trie->postfixes);
        T::Ptr newTrie2 = T::create(value.right(jEnd - j));
        return T::create(trie->prefix.left(i - trie->prefix.constBegin()),
                            QList<T::Ptr>() << newTrie1 << newTrie2);
    }
}

bool TrieNode::contains(const TrieNode::Ptr &trie,
    const QString &value, LookupFlags flags)
{
    if (trie.isNull())
        return false;
    QString::const_iterator i = trie->prefix.constBegin(), iEnd = trie->prefix.constEnd();
    QString::const_iterator j = value.constBegin(), jEnd = value.constEnd();
    while (i != iEnd && j != jEnd) {
        if (i->isSpace()) {
            if (! j->isSpace())
                return false;
            do {
                ++j;
            } while (j != jEnd && j->isSpace());
            do {
                ++i;
            } while (i != iEnd && i->isSpace());
        } else {
            if (*i != *j && ((flags & CaseInsensitive) == 0 || i->toLower() != j->toLower()))
                return false;
            ++i;
            ++j;
        }
    }
    if (j == jEnd) {
        if ((flags & Partial) != 0)
            return true;
        if (i == iEnd) {
            foreach (const TrieNode::Ptr t, trie->postfixes)
                if (t->prefix.isEmpty())
                    return true;
            return trie->postfixes.isEmpty();
        }
        return false;
    }
    if (i != iEnd)
        return false;
    bool res = false;
    foreach (const TrieNode::Ptr v, trie->postfixes) {
        QString::const_iterator vi = v->prefix.constBegin(), vEnd = v->prefix.constEnd();
        if (vi != vEnd && (*vi == *j || ((flags & CaseInsensitive) != 0
            && vi->toLower() == j->toLower())))
            res = res || contains(v, value.right(jEnd-j), flags);
    }
    return res;
}

namespace {
class Appender {
public:
    Appender() {}
    QStringList res;
    void operator()(const QString &s) {
        res.append(s);
    }
};
}

QStringList TrieNode::stringList(const TrieNode::Ptr &trie)
{
    Appender a;
    enumerateTrieNode<Appender>(trie, a, QString());
    return a.res;
}

std::pair<TrieNode::Ptr,int> TrieNode::intersectF(
    const TrieNode::Ptr &v1, const TrieNode::Ptr &v2, int index1)
{
    typedef TrieNode::Ptr P;
    typedef QMap<QString,int>::const_iterator MapIterator;
    if (v1.isNull() || v2.isNull())
        return std::make_pair(P(0), ((v1.isNull()) ? 1 : 0) | ((v2.isNull()) ? 2 : 0));
    QString::const_iterator i = v1->prefix.constBegin()+index1, iEnd = v1->prefix.constEnd();
    QString::const_iterator j = v2->prefix.constBegin(), jEnd = v2->prefix.constEnd();
    while (i != iEnd && j != jEnd) {
        if (i->isSpace()) {
            if (! j->isSpace())
                break;
            do {
                ++j;
            } while (j != jEnd && j->isSpace());
            do {
                ++i;
            } while (i != iEnd && i->isSpace());
        } else {
            if (*i != *j)
                break;
            ++i;
            ++j;
        }
    }
    if (i == iEnd) {
        if (j == jEnd) {
            if (v1->postfixes.isEmpty() || v2->postfixes.isEmpty()) {
                if (v1->postfixes.isEmpty() && v2->postfixes.isEmpty())
                    return std::make_pair(v1, 3);
                foreach (P t1, v1->postfixes)
                    if (t1->prefix.isEmpty()) {
                        if (index1 == 0)
                            return std::make_pair(v2, 2);
                        else
                            return std::make_pair(TrieNode::create(
                                v1->prefix.left(index1).append(v2->prefix), v2->postfixes),0);
                    }
                foreach (P t2, v2->postfixes)
                    if (t2->prefix.isEmpty())
                        return std::make_pair(v1,1);
                return std::make_pair(P(0), 0);
            }
            QMap<QString,int> p1, p2;
            QList<P> p3;
            int ii = 0;
            foreach (P t1, v1->postfixes)
                p1[t1->prefix] = ii++;
            ii = 0;
            foreach (P t2, v2->postfixes)
                p2[t2->prefix] = ii++;
            MapIterator p1Ptr = p1.constBegin(), p2Ptr = p2.constBegin(),
                p1End = p1.constEnd(), p2End = p2.constEnd();
            int sameV1V2 = 3;
            while (p1Ptr != p1End && p2Ptr != p2End) {
                if (p1Ptr.key().isEmpty()) {
                    if (p2Ptr.key().isEmpty()) {
                        if (sameV1V2 == 0)
                            p3.append(v1->postfixes.at(p1Ptr.value()));
                        ++p1Ptr;
                        ++p2Ptr;
                    } else {
                        if (sameV1V2 == 1)
                            for (MapIterator p1I = p1.constBegin();p1I != p1Ptr; ++p1I)
                                p3.append(v1->postfixes.at(p1I.value()));
                        ++p1Ptr;
                        sameV1V2 &= 2;
                    }
                } else if (p2Ptr.key().isEmpty()) {
                    if (sameV1V2 == 2)
                        for (MapIterator p2I = p2.constBegin(); p2I != p2Ptr; ++p2I)
                                p3.append(v2->postfixes.at(p2I.value()));
                    ++p2Ptr;
                    sameV1V2 &= 1;
                } else {
                    QChar c1 = p1Ptr.key().at(0);
                    QChar c2 = p2Ptr.key().at(0);
                    if (c1 < c2) {
                        if (sameV1V2 == 1)
                            for (MapIterator p1I = p1.constBegin(); p1I != p1Ptr; ++p1I)
                                p3.append(v1->postfixes.at(p1I.value()));
                        ++p1Ptr;
                        sameV1V2 &= 2;
                    } else if (c1 > c2) {
                        if (sameV1V2 == 2)
                            for (MapIterator  p2I = p2.constBegin(); p2I != p2Ptr; ++p2I)
                                    p3.append(v2->postfixes.at(p2I. value()));
                        ++p2Ptr;
                        sameV1V2 &= 1;
                    } else {
                        std::pair<P,int> res = intersectF(v1->postfixes.at(p1Ptr.value()),
                            v2->postfixes.at(p2Ptr.value()));
                        if (sameV1V2 !=0 && (sameV1V2 & res.second) == 0) {
                            if ((sameV1V2 & 1) == 1)
                                for (MapIterator p1I = p1.constBegin(); p1I != p1Ptr; ++p1I)
                                    p3.append(v1->postfixes.at(p1I.value()));
                            if (sameV1V2 == 2)
                                for (MapIterator p2I = p2.constBegin(); p2I != p2Ptr; ++p2I)
                                    p3.append(v2->postfixes.at(p2I.value()));
                        }
                        sameV1V2 &= res.second;
                        if (sameV1V2 == 0 && !res.first.isNull())
                            p3.append(res.first);
                        ++p1Ptr;
                        ++p2Ptr;
                    }
                }
            }
            if (p1Ptr != p1End) {
                if (sameV1V2 == 1)
                    for (MapIterator p1I = p1.constBegin(); p1I != p1Ptr; ++p1I)
                        p3.append(v1->postfixes.at(p1I.value()));
                sameV1V2 &= 2;
            } else if (p2Ptr != p2End) {
                if (sameV1V2 == 2) {
                    for (MapIterator  p2I = p2.constBegin(); p2I != p2Ptr; ++p2I)
                        p3.append(v2->postfixes.at(p2I. value()));
                }
                sameV1V2 &= 1;
            }
            switch (sameV1V2) {
            case 0:
                if (p3.isEmpty())
                   return std::make_pair(P(0),0);
                else
                    return std::make_pair(TrieNode::create(v1->prefix,p3),0);
            case 2:
                if (index1 == 0)
                    return std::make_pair(v2,2);
                else
                    return std::make_pair(TrieNode::create(
                            v1->prefix.left(index1).append(v2->prefix), v2->postfixes), 0);
            default:
                return std::make_pair(v1,sameV1V2);
            }
        }
       // i == iEnd && j != jEnd
        foreach (const P &t1, v1->postfixes)
            if ((!t1->prefix.isEmpty()) && t1->prefix.at(0) == *j) {
                std::pair<P,int> res = intersectF(v2,t1,j-v2->prefix.constBegin());
                if (index1 == 0)
                    return std::make_pair(res.first, (((res.second & 1)==1) ? 2 : 0));
                else
                    return std::make_pair(TrieNode::create(
                        v1->prefix.left(index1).append(res.first->prefix),
                        res.first->postfixes), 0);
            }
        return std::make_pair(P(0), 0);
    } else {
        // i != iEnd && j == jEnd
        foreach (P t2, v2->postfixes)
            if (!t2->prefix.isEmpty() && t2->prefix.at(0) == *i) {
                std::pair<P,int> res = intersectF(v1,t2,i-v1->prefix.constBegin());
                return std::make_pair(res.first, (res.second & 1));
            }
        return std::make_pair(P(0), 0);
    }
}

namespace {
class InplaceTrie{
public:
    TrieNode::Ptr trie;

    void operator()(QString s){
        trie = TrieNode::insertF(trie,s);
    }
};
}

std::pair<TrieNode::Ptr,int> TrieNode::mergeF(
    const TrieNode::Ptr &v1, const TrieNode::Ptr &v2)
{
    //could be much more efficient if implemented directly on the trie like intersectF
    InplaceTrie t;
    t.trie = v1;
    enumerateTrieNode<InplaceTrie>(v2, t, QString());
    return std::make_pair(t.trie, ((t.trie == v1) ? 1 : 0));
}

QDebug &TrieNode::printStrings(QDebug &dbg, const TrieNode::Ptr &trie)
{
    if (trie.isNull())
        return dbg << "Trie{*NULL*}";
    dbg<<"Trie{ contents:[";
    bool first = true;
    foreach (const QString &s, stringList(trie)) {
        if (!first)
            dbg << ",";
        else
            first = false;
        dbg << s;
    }
    dbg << "]}";
    return dbg;
}

QDebug &TrieNode::describe(QDebug &dbg, const TrieNode::Ptr &trie,
    int indent = 0)
{
    dbg.space();
    dbg.nospace();
    if (trie.isNull()) {
        dbg << "NULL";
        return dbg;
    }
    dbg << trie->prefix;
    int newIndent = indent + trie->prefix.size() + 3;
    bool newLine = false;
    foreach (TrieNode::Ptr sub, trie->postfixes) {
        if (newLine) {
            dbg << "\n";
            for (int i=0; i < newIndent; ++i)
                dbg << " ";
        } else {
            newLine = true;
        }
        describe(dbg, sub, newIndent);
    }
    return dbg;
}

QDebug &operator<<(QDebug &dbg, const TrieNode::Ptr &trie)
{
    dbg.nospace()<<"Trie{\n";
    TrieNode::describe(dbg,trie,0);
    dbg << "}";
    return dbg.space();
}
QDebug &operator<<(QDebug &dbg, const Trie &trie)
{
    dbg.nospace()<<"Trie{\n";
    TrieNode::describe(dbg,trie.trie,0);
    dbg << "}";
    return dbg.space();
}
Trie::Trie() {}
Trie::Trie(const TrieNode::Ptr &trie) : trie(trie) {}
Trie::Trie(const Trie &o) : trie(o.trie){}

QStringList Trie::complete(const QString &root, const QString &base,
    LookupFlags flags) const
{
    QStringList res;
    TrieNode::complete(res, trie, root, base, flags);
    return res;
}

bool Trie::contains(const QString &value, LookupFlags flags) const
{
    return TrieNode::contains(trie, value, flags);
}

QStringList Trie::stringList() const
{
    return TrieNode::stringList(trie);
}

/*!
    \brief inserts into the current trie.

    Non thread safe, only use this on an instance that is used only
    in a single theread, or that is protected by locks.
 */
void Trie::insert(const QString &value)
{
    trie = TrieNode::insertF(trie, value);
}

/*!
    \brief intesects into the current trie.

    Non thread safe, only use this on an instance that is used only
    in a single theread, or that is protected by locks.
 */
void Trie::intersect(const Trie &v)
{
    trie = TrieNode::intersectF(trie, v.trie).first;
}

/*!
    \brief merges the given trie into the current one.

    Non thread safe, only use this on an instance that is used only
    in a single theread, or that is protected by locks.
 */
void Trie::merge(const Trie &v)
{
    trie = TrieNode::mergeF(trie, v.trie).first;
}

Trie Trie::insertF(const QString &value) const
{
    return Trie(TrieNode::insertF(trie, value));
}

Trie Trie::intersectF(const Trie &v) const
{
    return Trie(TrieNode::intersectF(trie, v.trie).first);
}

Trie Trie::mergeF(const Trie &v) const
{
    return Trie(TrieNode::mergeF(trie, v.trie).first);
}

/*!
  \fn int matchStrength(const QString &searchStr, const QString &str)

  Returns a number defining how well the serachStr matches str.

  Quite simplistic, looks only at the first match, and prefers contiguos
  matches, or matches to ca capitalized or separated word.
*/
int matchStrength(const QString &searchStr, const QString &str)
{
    QString::const_iterator i = searchStr.constBegin(), iEnd = searchStr.constEnd(),
        j = str.constBegin(), jEnd = str.constEnd();
    bool lastWasNotUpper=true, lastWasSpacer=true, lastWasMatch = false;
    int res = 0;
    while (i != iEnd && j != jEnd) {
        bool thisIsUpper = (*j).isUpper();
        bool thisIsLetterOrNumber = (*j).isLetterOrNumber();
        if ((*i).toLower() == (*j).toLower()) {
            if (lastWasMatch || (lastWasNotUpper && thisIsUpper)
                || (thisIsUpper && (*i).isUpper())
                || (lastWasSpacer && thisIsLetterOrNumber))
                ++res;
            lastWasMatch = true;
            ++i;
        } else {
            lastWasMatch = false;
        }
        ++j;
        lastWasNotUpper = !thisIsUpper;
        lastWasSpacer = !thisIsLetterOrNumber;
    }
    if (i != iEnd)
        return iEnd - i;
    return res;
}

namespace {
class CompareMatchStrength{
    QString searchStr;
public:
    CompareMatchStrength(const QString& searchStr) : searchStr(searchStr) { }
    bool operator()(const QString &v1, const QString &v2) {
        return matchStrength(searchStr,v1) > matchStrength(searchStr,v2);
    }
};
}

/*!
  \fn QStringList matchingStrengthSort(const QString &searchStr, QStringList &res)

  returns a number defining the matching strength of res to the given searchStr
*/
QStringList matchStrengthSort(const QString &searchStr, QStringList &res)
{
    CompareMatchStrength compare(searchStr);
    std::stable_sort(res.begin(), res.end(), compare);
    return res;
}

} // end namespace PersistentTrie
} // end namespace QmlJS
