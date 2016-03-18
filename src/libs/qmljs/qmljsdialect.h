/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmljs_global.h"

#include <utils/fileutils.h>

#include <QString>

namespace QmlJS {

class QMLJS_EXPORT Dialect {
public:
    enum Enum
    {
        NoLanguage = 0,
        JavaScript = 1,
        Json = 2,
        Qml = 3,
        QmlQtQuick1 = 4,
        QmlQtQuick2 = 5,
        QmlQbs = 6,
        QmlProject = 7,
        QmlTypeInfo = 8,
        QmlQtQuick2Ui = 9,
        AnyLanguage = 10,
    };
    Dialect(Enum dialect = NoLanguage)
        : m_dialect(dialect)
    { }
    static Dialect mergeLanguages(const Dialect &l1, const Dialect &l2);

    bool isQmlLikeLanguage() const;
    bool isFullySupportedLanguage() const;
    bool isQmlLikeOrJsLanguage() const;
    QList<Dialect> companionLanguages() const;
    bool restrictLanguage(const Dialect &l2);
    QString toString() const;
    bool operator ==(const Dialect &o) const;
    bool operator !=(const Dialect &o) const {
        return !(*this == o);
    }
    bool operator <(const Dialect &o) const;
    Enum dialect() const {
        return m_dialect;
    }
    void mergeLanguage(const Dialect &l2);
private:
    Enum m_dialect;
};

QMLJS_EXPORT uint qHash(const Dialect &o);

QMLJS_EXPORT QDebug operator << (QDebug &dbg, const Dialect &dialect);

class QMLJS_EXPORT PathAndLanguage {
public:
    PathAndLanguage(const Utils::FileName &path = Utils::FileName(), Dialect language = Dialect::AnyLanguage);
    PathAndLanguage(const PathAndLanguage &o)
        : m_path(o.path()), m_language(o.language())
    { }
    Utils::FileName path() const {
        return m_path;
    }
    Dialect language() const {
        return m_language;
    }
    bool operator ==(const PathAndLanguage &other) const;
    bool operator < (const PathAndLanguage &other) const;
private:
    Utils::FileName m_path;
    Dialect m_language;
};

// tries to find the "most specific" language still compatible with all requested ones
class QMLJS_EXPORT LanguageMerger
{
public:
    LanguageMerger()
        : m_specificLanguage(Dialect::AnyLanguage),
          m_minimalSpecificLanguage(Dialect::NoLanguage),
          m_restrictFailed(false)
    { }

    void merge(Dialect l);

    Dialect mergedLanguage() const {
        return m_specificLanguage;
    }
    bool restrictFailed() const {
        return m_restrictFailed;
    }
private:
    Dialect m_specificLanguage;
    Dialect m_minimalSpecificLanguage;
    bool m_restrictFailed;
};


QMLJS_EXPORT QDebug operator << (QDebug &dbg, const PathAndLanguage &pathAndLanguage);

class QMLJS_EXPORT PathsAndLanguages
{
public:
    explicit PathsAndLanguages()
    { }
    explicit PathsAndLanguages(const QList<PathAndLanguage> &list)
        : m_list(list)
    { }
    PathsAndLanguages(const PathsAndLanguages &o)
        : m_list(o.m_list)
    { }

    bool maybeInsert(const Utils::FileName &path, Dialect language = Dialect::AnyLanguage) {
        return maybeInsert(PathAndLanguage(path, language));
    }

    bool maybeInsert(const PathAndLanguage &pathAndLanguage);

    PathAndLanguage at(int i) const {
        return m_list.at(i);
    }
    int size() const {
        return m_list.size();
    }
    int length() const {
        return m_list.length();
    }
    void clear() {
        m_list.clear();
    }
    // foreach support
    typedef QList<PathAndLanguage>::const_iterator const_iterator;
    const_iterator begin() const {
        return m_list.begin();
    }
    const_iterator end() const {
        return m_list.end();
    }
    void compact();
private:
    QList<PathAndLanguage> m_list;
};

} // namespace QmlJS
