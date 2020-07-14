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

#include "namevalueitem.h"
#include "algorithm.h"
#include "namevaluedictionary.h"
#include "qtcassert.h"

#include <QDebug>

namespace Utils {

void NameValueItem::sort(NameValueItems *list)
{
    Utils::sort(*list, &NameValueItem::name);
}

NameValueItems NameValueItem::fromStringList(const QStringList &list)
{
    NameValueItems result;
    for (const QString &string : list) {
        int pos = string.indexOf("+=");
        if (pos != -1) {
            result.append({string.left(pos), string.mid(pos + 2), NameValueItem::Append});
            continue;
        }
        pos = string.indexOf("=+");
        if (pos != -1) {
            result.append({string.left(pos), string.mid(pos + 2), NameValueItem::Prepend});
            continue;
        }
        pos = string.indexOf('=', 1);
        if (pos == -1) {
            result.append(NameValueItem(string, QString(), NameValueItem::Unset));
            continue;
        }
        const int hashPos = string.indexOf('#');
        if (hashPos != -1 && hashPos < pos) {
            result.append({string.mid(hashPos + 1, pos - hashPos - 1), string.mid(pos + 1),
                           NameValueItem::SetDisabled});
        } else {
            result.append({string.left(pos), string.mid(pos + 1)});
        }
    }
    return result;
}

QStringList NameValueItem::toStringList(const NameValueItems &list)
{
    return Utils::transform<QStringList>(list, [](const NameValueItem &item) {
        switch (item.operation) {
        case NameValueItem::Unset:
            return item.name;
        case NameValueItem::Append:
            return QString(item.name + "+=" + item.value);
        case NameValueItem::Prepend:
            return QString(item.name + "=+" + item.value);
        case NameValueItem::SetDisabled:
            return QString('#' + item.name + '=' + item.value);
        case NameValueItem::SetEnabled:
            return QString(item.name + '=' + item.value);
        }
        return QString();
    });
}

NameValueItems NameValueItem::itemsFromVariantList(const QVariantList &list)
{
    return Utils::transform<NameValueItems>(list, [](const QVariant &item) {
        return itemFromVariantList(item.toList());
    });
}

QVariantList NameValueItem::toVariantList(const NameValueItems &list)
{
    return Utils::transform<QVariantList>(list, [](const NameValueItem &item) {
        return QVariant(toVariantList(item));
    });
}

NameValueItem NameValueItem::itemFromVariantList(const QVariantList &list)
{
    QTC_ASSERT(list.size() == 3, return NameValueItem("", ""));
    QString key = list.value(0).toString();
    Operation operation = Operation(list.value(1).toInt());
    QString value = list.value(2).toString();
    return NameValueItem(key, value, operation);
}

QVariantList NameValueItem::toVariantList(const NameValueItem &item)
{
    return QVariantList() << item.name << item.operation << item.value;
}

static QString expand(const NameValueDictionary *dictionary, QString value)
{
    int replaceCount = 0;
    for (int i = 0; i < value.size(); ++i) {
        if (value.at(i) == '$') {
            if ((i + 1) < value.size()) {
                const QChar &c = value.at(i + 1);
                int end = -1;
                if (c == '(')
                    end = value.indexOf(')', i);
                else if (c == '{')
                    end = value.indexOf('}', i);
                if (end != -1) {
                    const QString &key = value.mid(i + 2, end - i - 2);
                    NameValueDictionary::const_iterator it = dictionary->constFind(key);
                    if (it != dictionary->constEnd())
                        value.replace(i, end - i + 1, it.value().first);
                    ++replaceCount;
                    QTC_ASSERT(replaceCount < 100, break);
                }
            }
        }
    }
    return value;
}

enum : char {
#ifdef Q_OS_WIN
    pathSepC = ';'
#else
    pathSepC = ':'
#endif
};

void NameValueItem::apply(NameValueDictionary *dictionary, Operation op) const
{
    switch (op) {
    case SetEnabled:
        dictionary->set(name, expand(dictionary, value));
        break;
    case SetDisabled:
        dictionary->set(name, expand(dictionary, value), false);
        break;
    case Unset:
        dictionary->unset(name);
        break;
    case Prepend: {
        const NameValueDictionary::const_iterator it = dictionary->constFind(name);
        if (it != dictionary->constEnd()) {
            QString v = dictionary->value(it);
            const QChar pathSep{QLatin1Char(pathSepC)};
            int sepCount = 0;
            if (v.startsWith(pathSep))
                ++sepCount;
            if (value.endsWith(pathSep))
                ++sepCount;
            if (sepCount == 2)
                v.remove(0, 1);
            else if (sepCount == 0)
                v.prepend(pathSep);
            v.prepend(expand(dictionary, value));
            dictionary->set(name, v);
        } else {
            apply(dictionary, SetEnabled);
        }
    } break;
    case Append: {
        const NameValueDictionary::const_iterator it = dictionary->constFind(name);
        if (it != dictionary->constEnd()) {
            QString v = dictionary->value(it);
            const QChar pathSep{QLatin1Char(pathSepC)};
            int sepCount = 0;
            if (v.endsWith(pathSep))
                ++sepCount;
            if (value.startsWith(pathSep))
                ++sepCount;
            if (sepCount == 2)
                v.chop(1);
            else if (sepCount == 0)
                v.append(pathSep);
            v.append(expand(dictionary, value));
            dictionary->set(name, v);
        } else {
            apply(dictionary, SetEnabled);
        }
    } break;
    }
}

QDebug operator<<(QDebug debug, const NameValueItem &i)
{
    QDebugStateSaver saver(debug);
    debug.noquote();
    debug.nospace();
    debug << "KeyValueItem(";
    switch (i.operation) {
    case NameValueItem::SetEnabled:
        debug << "set \"" << i.name << "\" to \"" << i.value << '"';
        break;
    case NameValueItem::SetDisabled:
        debug << "set \"" << i.name << "\" to \"" << i.value << '"' << "[disabled]";
        break;
    case NameValueItem::Unset:
        debug << "unset \"" << i.name << '"';
        break;
    case NameValueItem::Prepend:
        debug << "prepend to \"" << i.name << "\":\"" << i.value << '"';
        break;
    case NameValueItem::Append:
        debug << "append to \"" << i.name << "\":\"" << i.value << '"';
        break;
    }
    debug << ')';
    return debug;
}
} // namespace Utils
