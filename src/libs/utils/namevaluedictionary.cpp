/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "algorithm.h"
#include "namevaluedictionary.h"
#include "qtcassert.h"

#include <QDir>

namespace Utils {

NameValueDictionary::NameValueDictionary(const QStringList &env, OsType osType)
    : m_osType(osType)
{
    for (const QString &s : env) {
        int i = s.indexOf('=', 1);
        if (i >= 0) {
            const QString key = s.left(i);
            if (!key.contains('=')) {
                const QString value = s.mid(i + 1);
                set(key, value);
            }
        }
    }
}

NameValueDictionary::NameValueDictionary(const NameValuePairs &nameValues)
{
    for (const auto &nameValue : nameValues)
        set(nameValue.first, nameValue.second);
}

NameValueMap::iterator NameValueDictionary::findKey(const QString &key)
{
    for (auto it = m_values.begin(); it != m_values.end(); ++it) {
        if (key.compare(it.key().name, nameCaseSensitivity()) == 0)
            return it;
    }
    return m_values.end();
}

NameValueMap::const_iterator NameValueDictionary::findKey(const QString &key) const
{
    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it) {
        if (key.compare(it.key().name, nameCaseSensitivity()) == 0)
            return it;
    }
    return m_values.constEnd();
}

QStringList NameValueDictionary::toStringList() const
{
    QStringList result;
    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it) {
        if (it.value().second)
            result.append(it.key().name + '=' + it.value().first);
    }
    return result;
}

void NameValueDictionary::set(const QString &key, const QString &value, bool enabled)
{
    QTC_ASSERT(!key.contains('='), return );
    const auto it = findKey(key);
    const auto valuePair = qMakePair(value, enabled);
    if (it == m_values.end())
        m_values.insert(DictKey(key, nameCaseSensitivity()), valuePair);
    else
        it.value() = valuePair;
}

void NameValueDictionary::unset(const QString &key)
{
    QTC_ASSERT(!key.contains('='), return );
    const auto it = findKey(key);
    if (it != m_values.end())
        m_values.erase(it);
}

void NameValueDictionary::clear()
{
    m_values.clear();
}

QString NameValueDictionary::value(const QString &key) const
{
    const auto it = findKey(key);
    return it != m_values.end() && it.value().second ? it.value().first : QString();
}

NameValueDictionary::const_iterator NameValueDictionary::constFind(const QString &name) const
{
    return findKey(name);
}

int NameValueDictionary::size() const
{
    return m_values.size();
}

void NameValueDictionary::modify(const NameValueItems &items)
{
    NameValueDictionary resultKeyValueDictionary = *this;
    for (const NameValueItem &item : items)
        item.apply(&resultKeyValueDictionary);
    *this = resultKeyValueDictionary;
}

NameValueItems NameValueDictionary::diff(const NameValueDictionary &other, bool checkAppendPrepend) const
{
    NameValueMap::const_iterator thisIt = constBegin();
    NameValueMap::const_iterator otherIt = other.constBegin();

    NameValueItems result;
    while (thisIt != constEnd() || otherIt != other.constEnd()) {
        if (thisIt == constEnd()) {
            result.append({other.key(otherIt), other.value(otherIt),
                otherIt.value().second ? NameValueItem::SetEnabled : NameValueItem::SetDisabled});
            ++otherIt;
        } else if (otherIt == other.constEnd()) {
            result.append(NameValueItem(key(thisIt), QString(), NameValueItem::Unset));
            ++thisIt;
        } else if (thisIt.key() < otherIt.key()) {
            result.append(NameValueItem(key(thisIt), QString(), NameValueItem::Unset));
            ++thisIt;
        } else if (thisIt.key() > otherIt.key()) {
            result.append({other.key(otherIt), otherIt.value().first,
                otherIt.value().second ? NameValueItem::SetEnabled : NameValueItem::SetDisabled});
            ++otherIt;
        } else {
            const QString &oldValue = thisIt.value().first;
            const QString &newValue = otherIt.value().first;
            const bool oldEnabled = thisIt.value().second;
            const bool newEnabled = otherIt.value().second;
            if (oldValue != newValue) {
                if (checkAppendPrepend && newValue.startsWith(oldValue)
                        && oldEnabled == newEnabled) {
                    QString appended = newValue.right(newValue.size() - oldValue.size());
                    if (appended.startsWith(OsSpecificAspects::pathListSeparator(osType())))
                        appended.remove(0, 1);
                    result.append(NameValueItem(other.key(otherIt), appended, NameValueItem::Append));
                } else if (checkAppendPrepend && newValue.endsWith(oldValue)
                           && oldEnabled == newEnabled) {
                    QString prepended = newValue.left(newValue.size() - oldValue.size());
                    if (prepended.endsWith(OsSpecificAspects::pathListSeparator(osType())))
                        prepended.chop(1);
                    result.append(NameValueItem(other.key(otherIt), prepended, NameValueItem::Prepend));
                } else {
                    result.append({other.key(otherIt), newValue, newEnabled
                            ? NameValueItem::SetEnabled : NameValueItem::SetDisabled});
                }
            }
            ++otherIt;
            ++thisIt;
        }
    }
    return result;
}

bool NameValueDictionary::hasKey(const QString &key) const
{
    return findKey(key) != constEnd();
}

OsType NameValueDictionary::osType() const
{
    return m_osType;
}

Qt::CaseSensitivity NameValueDictionary::nameCaseSensitivity() const
{
    return OsSpecificAspects::envVarCaseSensitivity(osType());
}

QString NameValueDictionary::userName() const
{
    return value(QString::fromLatin1(m_osType == OsTypeWindows ? "USERNAME" : "USER"));
}

} // namespace Utils
