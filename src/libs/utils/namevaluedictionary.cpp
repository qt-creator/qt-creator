// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "algorithm.h"
#include "namevaluedictionary.h"
#include "namevalueitem.h"
#include "qtcassert.h"

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
    return m_values.find(DictKey(key, nameCaseSensitivity()));
}

NameValueMap::const_iterator NameValueDictionary::findKey(const QString &key) const
{
    return m_values.find(DictKey(key, nameCaseSensitivity()));
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

int NameValueDictionary::size() const
{
    return m_values.size();
}

void NameValueDictionary::modify(const EnvironmentItems &items)
{
    NameValueDictionary resultKeyValueDictionary = *this;
    for (const EnvironmentItem &item : items)
        item.apply(&resultKeyValueDictionary);
    *this = resultKeyValueDictionary;
}

EnvironmentItems NameValueDictionary::diff(const NameValueDictionary &other, bool checkAppendPrepend) const
{
    NameValueMap::const_iterator thisIt = m_values.begin();
    NameValueMap::const_iterator otherIt = other.m_values.begin();

    EnvironmentItems result;
    while (thisIt != m_values.end() || otherIt != other.m_values.end()) {
        if (thisIt == m_values.end()) {
            const auto enabled = otherIt.value().second ? EnvironmentItem::SetEnabled
                                                        : EnvironmentItem::SetDisabled;
            result.append({otherIt.key().name, otherIt.value().first, enabled});
            ++otherIt;
        } else if (otherIt == other.m_values.end()) {
            result.append(EnvironmentItem(thisIt.key().name, QString(), EnvironmentItem::Unset));
            ++thisIt;
        } else if (thisIt.key() < otherIt.key()) {
            result.append(EnvironmentItem(thisIt.key().name, QString(), EnvironmentItem::Unset));
            ++thisIt;
        } else if (thisIt.key() > otherIt.key()) {
            const auto enabled = otherIt.value().second ? EnvironmentItem::SetEnabled
                                                        : EnvironmentItem::SetDisabled;
            result.append({otherIt.key().name, otherIt.value().first, enabled});
            ++otherIt;
        } else {
            const QString &oldValue = thisIt.value().first;
            const QString &newValue = otherIt.value().first;
            const bool oldEnabled = thisIt.value().second;
            const bool newEnabled = otherIt.value().second;
            if (oldValue != newValue || oldEnabled != newEnabled) {
                if (checkAppendPrepend && newValue.startsWith(oldValue)
                        && oldEnabled == newEnabled) {
                    QString appended = newValue.right(newValue.size() - oldValue.size());
                    if (appended.startsWith(OsSpecificAspects::pathListSeparator(osType())))
                        appended.remove(0, 1);
                    result.append(
                        EnvironmentItem(otherIt.key().name, appended, EnvironmentItem::Append));
                } else if (checkAppendPrepend && newValue.endsWith(oldValue)
                           && oldEnabled == newEnabled) {
                    QString prepended = newValue.left(newValue.size() - oldValue.size());
                    if (prepended.endsWith(OsSpecificAspects::pathListSeparator(osType())))
                        prepended.chop(1);
                    result.append(
                        EnvironmentItem(otherIt.key().name, prepended, EnvironmentItem::Prepend));
                } else {
                    const auto enabled = newEnabled ? EnvironmentItem::SetEnabled
                                                    : EnvironmentItem::SetDisabled;
                    result.append({otherIt.key().name, newValue, enabled});
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
    return m_values.find(DictKey(key, nameCaseSensitivity())) != m_values.end();
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
