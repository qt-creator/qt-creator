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

#include "qmljsdialect.h"
#include "qmljsconstants.h"

#include <QDebug>

namespace QmlJS {


bool Dialect::isQmlLikeLanguage() const
{
    switch (m_dialect) {
    case Dialect::Qml:
    case Dialect::QmlQtQuick1:
    case Dialect::QmlQtQuick2:
    case Dialect::QmlQtQuick2Ui:
    case Dialect::QmlQbs:
    case Dialect::QmlProject:
    case Dialect::QmlTypeInfo:
    case Dialect::AnyLanguage:
        return true;
    default:
        return false;
    }
}

bool Dialect::isFullySupportedLanguage() const
{
    switch (m_dialect) {
    case Dialect::JavaScript:
    case Dialect::Json:
    case Dialect::Qml:
    case Dialect::QmlQtQuick1:
    case Dialect::QmlQtQuick2:
    case Dialect::QmlQtQuick2Ui:
        return true;
    case Dialect::NoLanguage:
    case Dialect::AnyLanguage:
    case Dialect::QmlQbs:
    case Dialect::QmlProject:
    case Dialect::QmlTypeInfo:
        break;
    }
    return false;
}

bool Dialect::isQmlLikeOrJsLanguage() const
{
    switch (m_dialect) {
    case Dialect::Qml:
    case Dialect::QmlQtQuick1:
    case Dialect::QmlQtQuick2:
    case Dialect::QmlQtQuick2Ui:
    case Dialect::QmlQbs:
    case Dialect::QmlProject:
    case Dialect::QmlTypeInfo:
    case Dialect::JavaScript:
    case Dialect::AnyLanguage:
        return true;
    default:
        return false;
    }
}

QString Dialect::toString() const
{
    switch (m_dialect) {
    case Dialect::JavaScript:
        return QLatin1String("JavaScript");
    case Dialect::Json:
        return QLatin1String("Json");
    case Dialect::Qml:
        return QLatin1String("Qml");
    case Dialect::QmlQtQuick1:
        return QLatin1String("QmlQtQuick1");
    case Dialect::QmlQtQuick2:
        return QLatin1String("QmlQtQuick2");
    case Dialect::QmlQtQuick2Ui:
        return QLatin1String("QmlQtQuick2Ui");
    case Dialect::NoLanguage:
        return QLatin1String("NoLanguage");
    case Dialect::AnyLanguage:
        return QLatin1String("AnyLanguage");
    case Dialect::QmlQbs:
        return QLatin1String("QmlQbs");
    case Dialect::QmlProject:
        return QLatin1String("QmlProject");
    case Dialect::QmlTypeInfo:
        break;
    }
    return QLatin1String("QmlTypeInfo");
}

bool Dialect::operator ==(const Dialect &o) const {
    return m_dialect == o.m_dialect;
}

bool Dialect::operator <(const Dialect &o) const {
    return m_dialect < o.m_dialect;
}

Dialect Dialect::mergeLanguages(const Dialect &l1, const Dialect &l2)
{
    if (l1 == Dialect::NoLanguage)
        return l2;
    if (l2 == Dialect::NoLanguage)
        return l1;
    QList<Dialect> ll1 = l1.companionLanguages();
    QList<Dialect> ll2 = l2.companionLanguages();
    bool i1 = ll1.contains(l2);
    bool i2 = ll2.contains(l1);
    if (i1 && i2) {
        if (ll1.size() > ll2.size())
            return l1;
        if (ll2.size() > ll1.size())
            return l2;
        if (l1 < l2)
            return l1;
        return l2;
    }
    if (i1 && !i2)
        return l1;
    if (i2 && !i1)
        return l2;
    QList<Dialect> qmlLangs = Dialect(Qml).companionLanguages();
    if (qmlLangs.contains(l1) && qmlLangs.contains(l2))
        return Dialect::Qml;
    return Dialect::AnyLanguage;
}

void Dialect::mergeLanguage(const Dialect &l2) {
    *this = mergeLanguages(*this, l2);
}

bool Dialect::restrictLanguage(const Dialect &l2)
{
    if (*this == l2)
        return true;
    QList<Dialect> ll1 = companionLanguages();
    QList<Dialect> ll2 = l2.companionLanguages();
    bool i1 = ll1.contains(l2);
    bool i2 = ll2.contains(*this);
    if (i1 && i2) {
        if (ll1.size() < ll2.size())
            return true;
        if (ll2.size() < ll1.size()) {
            *this = l2;
            return true;
        }
        if (m_dialect < l2.m_dialect)
            return true;
        *this = l2;
        return true;
    }
    if (i1 && !i2) {
        *this = l2;
        return true;
    }
    if (i2 && !i1)
        return true;
    qDebug() << toString() << "restrictTo" << l2.toString() << "failed";
    qDebug() << ll1 << ll2;
    qDebug() << i1 << i2;
    QList<Dialect> qmlLangs = Dialect(Qml).companionLanguages();
    if (qmlLangs.contains(*this) && qmlLangs.contains(l2))
        *this = Dialect::Qml;
    *this = Dialect::AnyLanguage;
    return false;
}

QList<Dialect> Dialect::companionLanguages() const
{
    QList<Dialect> langs;
    langs << *this;
    switch (m_dialect) {
    case Dialect::JavaScript:
    case Dialect::Json:
    case Dialect::QmlProject:
    case Dialect::QmlTypeInfo:
        break;
    case Dialect::QmlQbs:
        langs << Dialect::JavaScript;
        break;
    case Dialect::Qml:
        langs << Dialect::QmlQtQuick1 << Dialect::QmlQtQuick2 << Dialect::QmlQtQuick2Ui
              << Dialect::JavaScript;
        break;
    case Dialect::QmlQtQuick1:
        langs << Dialect::Qml << Dialect::JavaScript;
        break;
    case Dialect::QmlQtQuick2:
    case Dialect::QmlQtQuick2Ui:
        langs.clear();
        langs << Dialect::QmlQtQuick2 << Dialect::QmlQtQuick2Ui << Dialect::Qml
              << Dialect::JavaScript;
        break;
    case Dialect::AnyLanguage:
        langs << Dialect::JavaScript << Dialect::Json << Dialect::QmlProject << Dialect:: QmlQbs
              << Dialect::QmlTypeInfo << Dialect::QmlQtQuick1 << Dialect::QmlQtQuick2
              << Dialect::QmlQtQuick2Ui << Dialect::Qml;
        break;
    case Dialect::NoLanguage:
        return QList<Dialect>(); // return at least itself?
    }
    if (*this != Dialect::AnyLanguage)
        langs << Dialect::AnyLanguage;
    return langs;
}

uint qHash(const Dialect &o)
{
    return uint(o.dialect());
}

QDebug operator << (QDebug &dbg, const Dialect &dialect)
{
    dbg << dialect.toString();
    return dbg;
}

PathAndLanguage::PathAndLanguage(const Utils::FileName &path, Dialect language)
    : m_path(path), m_language(language)
{ }

bool PathAndLanguage::operator ==(const PathAndLanguage &other) const
{
    return path() == other.path() && language() == other.language();
}

bool PathAndLanguage::operator <(const PathAndLanguage &other) const
{
    if (path() < other.path())
        return true;
    if (path() > other.path())
        return false;
    if (language() == other.language())
        return false;
    bool i1 = other.language().companionLanguages().contains(language());
    bool i2 = language().companionLanguages().contains(other.language());
    if (i1 && !i2)
        return true;
    if (i2 && !i1)
        return false;
    return language() < other.language();
}

QDebug operator << (QDebug &dbg, const PathAndLanguage &pathAndLanguage)
{
    dbg << "{ path:" << pathAndLanguage.path() << " language:" << pathAndLanguage.language().toString() << "}";
    return dbg;
}

bool PathsAndLanguages::maybeInsert(const PathAndLanguage &pathAndLanguage) {
    for (int i = 0; i < m_list.size(); ++i) {
        PathAndLanguage currentElement = m_list.at(i);
        if (currentElement.path() == pathAndLanguage.path()) {
            int j = i;
            do {
                if (pathAndLanguage.language() < currentElement.language()) {
                    if (currentElement.language() == pathAndLanguage.language())
                        return false;
                    break;
                }
                ++j;
                if (j == m_list.length())
                    break;
                currentElement = m_list.at(j);
            } while (currentElement.path() == pathAndLanguage.path());
            m_list.insert(j, pathAndLanguage);
            return true;
        }
    }
    m_list.append(pathAndLanguage);
    return true;
}

void PathsAndLanguages::compact()
{
    if (m_list.isEmpty())
        return;

    int oldCompactionPlace = 0;
    Utils::FileName oldPath = m_list.first().path();
    QList<PathAndLanguage> compactedList;
    bool restrictFailed = false;
    for (int i = 1; i < m_list.length(); ++i) {
        Utils::FileName newPath = m_list.at(i).path();
        if (newPath == oldPath) {
            int newCompactionPlace = i - 1;
            compactedList << m_list.mid(oldCompactionPlace, newCompactionPlace - oldCompactionPlace);
            LanguageMerger merger;
            merger.merge(m_list.at(i - 1).language());
            do {
                merger.merge(m_list.at(i).language());
                if (++i == m_list.length())
                    break;
                newPath = m_list.at(i).path();
            } while (newPath == oldPath);
            oldCompactionPlace = i;
            compactedList << PathAndLanguage(oldPath, merger.mergedLanguage());
            if (merger.restrictFailed())
                restrictFailed = true;
        }
        oldPath = newPath;
    }
    if (oldCompactionPlace == 0)
        return;
    compactedList << m_list.mid(oldCompactionPlace);
    if (restrictFailed)
        qCWarning(qmljsLog) << "failed to restrict PathAndLanguages " << m_list;
    m_list = compactedList;
}

void LanguageMerger::merge(Dialect l)
{
    bool restrictSuccedeed = m_specificLanguage.restrictLanguage(l);
    m_specificLanguage.mergeLanguage(m_minimalSpecificLanguage);
    if (!restrictSuccedeed) {
        m_minimalSpecificLanguage = m_specificLanguage;
        m_restrictFailed = true;
    }
}

} // namespace QmlJS
