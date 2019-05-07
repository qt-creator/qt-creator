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

namespace {
NameValueMap::iterator findKey(NameValueMap &input, Utils::OsType osType, const QString &key)
{
    const Qt::CaseSensitivity casing = (osType == Utils::OsTypeWindows) ? Qt::CaseInsensitive
                                                                        : Qt::CaseSensitive;
    for (auto it = input.begin(); it != input.end(); ++it) {
        if (key.compare(it.key(), casing) == 0)
            return it;
    }
    return input.end();
}

NameValueMap::const_iterator findKey(const NameValueMap &input, Utils::OsType osType, const QString &key)
{
    const Qt::CaseSensitivity casing = (osType == Utils::OsTypeWindows) ? Qt::CaseInsensitive
                                                                        : Qt::CaseSensitive;
    for (auto it = input.constBegin(); it != input.constEnd(); ++it) {
        if (key.compare(it.key(), casing) == 0)
            return it;
    }
    return input.constEnd();
}
} // namespace

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

QStringList NameValueDictionary::toStringList() const
{
    QStringList result;
    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it)
        result.append(it.key() + '=' + it.value());
    return result;
}

void NameValueDictionary::set(const QString &key, const QString &value)
{
    QTC_ASSERT(!key.contains('='), return );
    auto it = findKey(m_values, m_osType, key);
    if (it == m_values.end())
        m_values.insert(key, value);
    else
        it.value() = value;
}

void NameValueDictionary::unset(const QString &key)
{
    QTC_ASSERT(!key.contains('='), return );
    auto it = findKey(m_values, m_osType, key);
    if (it != m_values.end())
        m_values.erase(it);
}

void NameValueDictionary::clear()
{
    m_values.clear();
}

QString NameValueDictionary::value(const QString &key) const
{
    const auto it = findKey(m_values, m_osType, key);
    return it != m_values.end() ? it.value() : QString();
}

NameValueDictionary::const_iterator NameValueDictionary::constFind(const QString &name) const
{
    return findKey(m_values, m_osType, name);
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

enum : char {
#ifdef Q_OS_WIN
    pathSepC = ';'
#else
    pathSepC = ':'
#endif
};

NameValueItems NameValueDictionary::diff(const NameValueDictionary &other, bool checkAppendPrepend) const
{
    NameValueMap::const_iterator thisIt = constBegin();
    NameValueMap::const_iterator otherIt = other.constBegin();

    NameValueItems result;
    while (thisIt != constEnd() || otherIt != other.constEnd()) {
        if (thisIt == constEnd()) {
            result.append(NameValueItem(otherIt.key(), otherIt.value()));
            ++otherIt;
        } else if (otherIt == other.constEnd()) {
            result.append(NameValueItem(thisIt.key(), QString(), NameValueItem::Unset));
            ++thisIt;
        } else if (thisIt.key() < otherIt.key()) {
            result.append(NameValueItem(thisIt.key(), QString(), NameValueItem::Unset));
            ++thisIt;
        } else if (thisIt.key() > otherIt.key()) {
            result.append(NameValueItem(otherIt.key(), otherIt.value()));
            ++otherIt;
        } else {
            const QString &oldValue = thisIt.value();
            const QString &newValue = otherIt.value();
            if (oldValue != newValue) {
                if (checkAppendPrepend && newValue.startsWith(oldValue)) {
                    QString appended = newValue.right(newValue.size() - oldValue.size());
                    if (appended.startsWith(QLatin1Char(pathSepC)))
                        appended.remove(0, 1);
                    result.append(NameValueItem(otherIt.key(), appended, NameValueItem::Append));
                } else if (checkAppendPrepend && newValue.endsWith(oldValue)) {
                    QString prepended = newValue.left(newValue.size() - oldValue.size());
                    if (prepended.endsWith(QLatin1Char(pathSepC)))
                        prepended.chop(1);
                    result.append(NameValueItem(otherIt.key(), prepended, NameValueItem::Prepend));
                } else {
                    result.append(NameValueItem(otherIt.key(), newValue));
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
    return m_values.contains(key);
}

OsType NameValueDictionary::osType() const
{
    return m_osType;
}

QString NameValueDictionary::userName() const
{
    return value(QString::fromLatin1(m_osType == OsTypeWindows ? "USERNAME" : "USER"));
}

/** Expand environment variables in a string.
 *
 * KeyValueDictionary variables are accepted in the following forms:
 * $SOMEVAR, ${SOMEVAR} on Unix and %SOMEVAR% on Windows.
 * No escapes and quoting are supported.
 * If a variable is not found, it is not substituted.
 */
QString NameValueDictionary::expandVariables(const QString &input) const
{
    QString result = input;

    if (m_osType == OsTypeWindows) {
        for (int vStart = -1, i = 0; i < result.length();) {
            if (result.at(i++) == '%') {
                if (vStart > 0) {
                    const_iterator it = findKey(m_values, m_osType, result.mid(vStart, i - vStart - 1));
                    if (it != m_values.constEnd()) {
                        result.replace(vStart - 1, i - vStart + 1, *it);
                        i = vStart - 1 + it->length();
                        vStart = -1;
                    } else {
                        vStart = i;
                    }
                } else {
                    vStart = i;
                }
            }
        }
    } else {
        enum { BASE, OPTIONALVARIABLEBRACE, VARIABLE, BRACEDVARIABLE } state = BASE;
        int vStart = -1;

        for (int i = 0; i < result.length();) {
            QChar c = result.at(i++);
            if (state == BASE) {
                if (c == '$')
                    state = OPTIONALVARIABLEBRACE;
            } else if (state == OPTIONALVARIABLEBRACE) {
                if (c == '{') {
                    state = BRACEDVARIABLE;
                    vStart = i;
                } else if (c.isLetterOrNumber() || c == '_') {
                    state = VARIABLE;
                    vStart = i - 1;
                } else {
                    state = BASE;
                }
            } else if (state == BRACEDVARIABLE) {
                if (c == '}') {
                    const_iterator it = m_values.constFind(result.mid(vStart, i - 1 - vStart));
                    if (it != constEnd()) {
                        result.replace(vStart - 2, i - vStart + 2, *it);
                        i = vStart - 2 + it->length();
                    }
                    state = BASE;
                }
            } else if (state == VARIABLE) {
                if (!c.isLetterOrNumber() && c != '_') {
                    const_iterator it = m_values.constFind(result.mid(vStart, i - vStart - 1));
                    if (it != constEnd()) {
                        result.replace(vStart - 1, i - vStart, *it);
                        i = vStart - 1 + it->length();
                    }
                    state = BASE;
                }
            }
        }
        if (state == VARIABLE) {
            const_iterator it = m_values.constFind(result.mid(vStart));
            if (it != constEnd())
                result.replace(vStart - 1, result.length() - vStart + 1, *it);
        }
    }
    return result;
}

QStringList NameValueDictionary::expandVariables(const QStringList &variables) const
{
    return Utils::transform(variables, [this](const QString &i) { return expandVariables(i); });
}

} // namespace Utils
