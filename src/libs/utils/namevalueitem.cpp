// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "namevalueitem.h"
#include "algorithm.h"
#include "namevaluedictionary.h"
#include "qtcassert.h"

#include <QDebug>

namespace Utils {

void EnvironmentItem::sort(EnvironmentItems *list)
{
    Utils::sort(*list, &EnvironmentItem::name);
}

EnvironmentItems EnvironmentItem::fromStringList(const QStringList &list)
{
    EnvironmentItems result;
    for (const QString &string : list) {
        if (string.startsWith("##")) {
            result.append({string.mid(2), {}, EnvironmentItem::Comment});
            continue;
        }
        int pos = string.indexOf("+=");
        if (pos != -1) {
            result.append({string.left(pos), string.mid(pos + 2), EnvironmentItem::Append});
            continue;
        }
        pos = string.indexOf("=+");
        if (pos != -1) {
            result.append({string.left(pos), string.mid(pos + 2), EnvironmentItem::Prepend});
            continue;
        }
        pos = string.indexOf('=', 1);
        if (pos == -1) {
            result.append(EnvironmentItem(string, QString(), EnvironmentItem::Unset));
            continue;
        }
        const int hashPos = string.indexOf('#');
        if (hashPos != -1 && hashPos < pos) {
            result.append({string.mid(hashPos + 1, pos - hashPos - 1), string.mid(pos + 1),
                           EnvironmentItem::SetDisabled});
        } else {
            result.append({string.left(pos), string.mid(pos + 1)});
        }
    }
    return result;
}

QStringList EnvironmentItem::toStringList(const EnvironmentItems &list)
{
    return Utils::transform<QStringList>(list, [](const EnvironmentItem &item) {
        switch (item.operation) {
        case EnvironmentItem::Unset:
            return item.name;
        case EnvironmentItem::Append:
            return QString(item.name + "+=" + item.value);
        case EnvironmentItem::Prepend:
            return QString(item.name + "=+" + item.value);
        case EnvironmentItem::SetDisabled:
            return QString('#' + item.name + '=' + item.value);
        case EnvironmentItem::SetEnabled:
            return QString(item.name + '=' + item.value);
        case EnvironmentItem::Comment:
            return QString("##" + item.name);
        }
        return QString();
    });
}

EnvironmentItems EnvironmentItem::itemsFromVariantList(const QVariantList &list)
{
    return Utils::transform<EnvironmentItems>(list, [](const QVariant &item) {
        return itemFromVariantList(item.toList());
    });
}

QVariantList EnvironmentItem::toVariantList(const EnvironmentItems &list)
{
    return Utils::transform<QVariantList>(list, [](const EnvironmentItem &item) {
        return QVariant(toVariantList(item));
    });
}

EnvironmentItem EnvironmentItem::itemFromVariantList(const QVariantList &list)
{
    QTC_ASSERT(list.size() == 3, return EnvironmentItem("", ""));
    QString key = list.value(0).toString();
    Operation operation = Operation(list.value(1).toInt());
    QString value = list.value(2).toString();
    return EnvironmentItem(key, value, operation);
}

QVariantList EnvironmentItem::toVariantList(const EnvironmentItem &item)
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
                    const NameValueDictionary::const_iterator it = dictionary->find(key);
                    if (it != dictionary->end())
                        value.replace(i, end - i + 1, it.value());
                    ++replaceCount;
                    QTC_ASSERT(replaceCount < 100, break);
                }
            }
        }
    }
    return value;
}

void EnvironmentItem::apply(NameValueDictionary *dictionary, Operation op) const
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
        const NameValueDictionary::const_iterator it = dictionary->find(name);
        if (it != dictionary->end()) {
            QString v = it.value();
            const QChar pathSep = HostOsInfo::pathListSeparator();
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
        const NameValueDictionary::const_iterator it = dictionary->find(name);
        if (it != dictionary->end()) {
            QString v = it.value();
            const QChar pathSep = HostOsInfo::pathListSeparator();
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
    case Comment: // ignore comments when applying to environment
        break;
    }
}

QDebug operator<<(QDebug debug, const EnvironmentItem &i)
{
    QDebugStateSaver saver(debug);
    debug.noquote();
    debug.nospace();
    debug << "KeyValueItem(";
    switch (i.operation) {
    case EnvironmentItem::SetEnabled:
        debug << "set \"" << i.name << "\" to \"" << i.value << '"';
        break;
    case EnvironmentItem::SetDisabled:
        debug << "set \"" << i.name << "\" to \"" << i.value << '"' << "[disabled]";
        break;
    case EnvironmentItem::Unset:
        debug << "unset \"" << i.name << '"';
        break;
    case EnvironmentItem::Prepend:
        debug << "prepend to \"" << i.name << "\":\"" << i.value << '"';
        break;
    case EnvironmentItem::Append:
        debug << "append to \"" << i.name << "\":\"" << i.value << '"';
        break;
    case EnvironmentItem::Comment:
        debug << "comment:" << i.name;
        break;
    }
    debug << ')';
    return debug;
}
} // namespace Utils
