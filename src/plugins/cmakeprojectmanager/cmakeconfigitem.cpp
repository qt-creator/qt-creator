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

#include "cmakeconfigitem.h"

#include <projectexplorer/kit.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QString>

namespace CMakeProjectManager {

// --------------------------------------------------------------------
// CMakeConfigItem:
// --------------------------------------------------------------------

CMakeConfigItem::CMakeConfigItem() = default;

CMakeConfigItem::CMakeConfigItem(const CMakeConfigItem &other) :
    key(other.key), type(other.type), isAdvanced(other.isAdvanced),
    inCMakeCache(false), isUnset(other.isUnset), value(other.value),
    documentation(other.documentation), values(other.values)
{}

CMakeConfigItem::CMakeConfigItem(const QByteArray &k, Type t,
                                 const QByteArray &d, const QByteArray &v) :
    key(k), type(t), value(v), documentation(d)
{ }

CMakeConfigItem::CMakeConfigItem(const QByteArray &k, const QByteArray &v) :
    key(k), value(v)
{ }

QByteArray CMakeConfigItem::valueOf(const QByteArray &key, const QList<CMakeConfigItem> &input)
{
    for (auto it = input.constBegin(); it != input.constEnd(); ++it) {
        if (it->key == key)
            return it->value;
    }
    return QByteArray();
}

QString CMakeConfigItem::expandedValueOf(const ProjectExplorer::Kit *k, const QByteArray &key,
                                         const QList<CMakeConfigItem> &input)
{
    for (auto it = input.constBegin(); it != input.constEnd(); ++it) {
        if (it->key == key)
            return it->expandedValue(k);
    }
    return QString();
}

static QString between(const QString::ConstIterator it1, const QString::ConstIterator it2)
{
    QString result;
    for (auto it = it1; it != it2; ++it)
        result.append(*it);
    return result;
}

QStringList CMakeConfigItem::cmakeSplitValue(const QString &in, bool keepEmpty)
{
    QStringList newArgs;
    if (in.isEmpty())
        return newArgs;

    int squareNesting = 0;
    QString newArg;
    auto last = in.constBegin();
    for (auto c = in.constBegin(); c != in.constEnd(); ++c) {
        switch (c->unicode()) {
        case '\\': {
            auto next = c + 1;
            if (next != in.constEnd() && *next == ';') {
                newArg.append(between(last, c));
                last = next;
                c = next;
            }
        } break;
        case '[': {
            ++squareNesting;
        } break;
        case ']': {
            --squareNesting;
        } break;
        case ';': {
            // Break the string here if we are not nested inside square
            // brackets.
            if (squareNesting == 0) {
              newArg.append(between(last, c));
              // Skip over the semicolon
              last = c + 1;
              if (!newArg.isEmpty() || keepEmpty) {
                // Add the last argument if the string is not empty.
                newArgs.append(newArg);
                newArg.clear();
              }
            }
        } break;
        default: {
            // Just append this character.
        } break;
        }
    }
    newArg.append(between(last, in.constEnd()));
    if (!newArg.isEmpty() || keepEmpty) {
        // Add the last argument if the string is not empty.
        newArgs.append(newArg);
    }

    return newArgs;
}

CMakeConfigItem::Type CMakeConfigItem::typeStringToType(const QByteArray &type)
{
    if (type == "BOOL")
        return CMakeConfigItem::BOOL;
    if (type == "STRING")
        return CMakeConfigItem::STRING;
    if (type == "FILEPATH")
        return CMakeConfigItem::FILEPATH;
    if (type == "PATH")
        return CMakeConfigItem::PATH;
    if (type == "STATIC")
        return CMakeConfigItem::STATIC;

    QTC_CHECK(type == "INTERNAL" || type == "UNINITIALIZED");
    return CMakeConfigItem::INTERNAL;
}

QString CMakeConfigItem::expandedValue(const ProjectExplorer::Kit *k) const
{
    return expandedValue(k->macroExpander());
}

QString CMakeConfigItem::expandedValue(const Utils::MacroExpander *expander) const
{
    return expander->expand(QString::fromUtf8(value));
}

std::function<bool (const CMakeConfigItem &a, const CMakeConfigItem &b)> CMakeConfigItem::sortOperator()
{
    return [](const CMakeConfigItem &a, const CMakeConfigItem &b) { return a.key < b.key; };
}

CMakeConfigItem CMakeConfigItem::fromString(const QString &s)
{
    // Strip comments (only at start of line!):
    int commentStart = s.count();
    for (int i = 0; i < s.count(); ++i) {
        const QChar c = s.at(i);
        if (c == ' ' || c == '\t')
            continue;
        else if ((c == '#')
                 || (c == '/' && i < s.count() - 1 && s.at(i + 1) == '/')) {
            commentStart = i;
            break;
        } else {
            break;
        }
    }
    const QString line = s.mid(0, commentStart);

    // Split up line:
    int firstPos = -1;
    int colonPos = -1;
    int equalPos = -1;
    for (int i = 0; i < line.count(); ++i) {
        const QChar c = s.at(i);
        if (firstPos < 0 && !c.isSpace()) {
            firstPos = i;
        }
        if (c == QLatin1Char(':')) {
            if (colonPos > 0)
                break;
            colonPos = i;
            continue;
        }
        if (c == QLatin1Char('=')) {
            equalPos = i;
            break;
        }
    }

    QString key;
    QString type;
    QString value;
    if (equalPos >= 0) {
        key = line.mid(firstPos, ((colonPos >= 0) ? colonPos : equalPos) - firstPos);
        type = (colonPos < 0) ? QString() : line.mid(colonPos + 1, equalPos - colonPos - 1);
        value = line.mid(equalPos + 1);
    }

    // Fill in item:
    CMakeConfigItem item;
    if (!key.isEmpty()) {
        item.key = key.toUtf8();
        item.type = CMakeConfigItem::typeStringToType(type.toUtf8());
        item.value = value.toUtf8();
    }
    return item;
}

static QByteArray trimCMakeCacheLine(const QByteArray &in) {
    int start = 0;
    while (start < in.count() && (in.at(start) == ' ' || in.at(start) == '\t'))
        ++start;

    return in.mid(start, in.count() - start - 1);
}

static QByteArrayList splitCMakeCacheLine(const QByteArray &line) {
    const int colonPos = line.indexOf(':');
    if (colonPos < 0)
        return QByteArrayList();

    const int equalPos = line.indexOf('=', colonPos + 1);
    if (equalPos < colonPos)
        return QByteArrayList();

    return QByteArrayList() << line.mid(0, colonPos)
                            << line.mid(colonPos + 1, equalPos - colonPos - 1)
                            << line.mid(equalPos + 1);
}

QList<CMakeConfigItem> CMakeConfigItem::itemsFromFile(const Utils::FileName &cacheFile, QString *errorMessage)
{
    CMakeConfig result;
    QFile cache(cacheFile.toString());
    if (!cache.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("CMakeProjectManager::CMakeConfigItem", "Failed to open %1 for reading.")
                .arg(cacheFile.toUserOutput());
        return CMakeConfig();
    }

    QSet<QByteArray> advancedSet;
    QMap<QByteArray, QByteArray> valuesMap;
    QByteArray documentation;
    while (!cache.atEnd()) {
        const QByteArray line = trimCMakeCacheLine(cache.readLine());

        if (line.isEmpty() || line.startsWith('#'))
            continue;

        if (line.startsWith("//")) {
            documentation = line.mid(2);
            continue;
        }

        const QByteArrayList pieces = splitCMakeCacheLine(line);
        if (pieces.isEmpty())
            continue;

        QTC_ASSERT(pieces.count() == 3, continue);
        const QByteArray key = pieces.at(0);
        const QByteArray type = pieces.at(1);
        const QByteArray value = pieces.at(2);

        if (key.endsWith("-ADVANCED") && value == "1") {
            advancedSet.insert(key.left(key.count() - 9 /* "-ADVANCED" */));
        } else if (key.endsWith("-STRINGS") && CMakeConfigItem::typeStringToType(type) == CMakeConfigItem::INTERNAL) {
            valuesMap[key.left(key.count() - 8) /* "-STRINGS" */] = value;
        } else {
            CMakeConfigItem::Type t = CMakeConfigItem::typeStringToType(type);
            result << CMakeConfigItem(key, t, documentation, value);
        }
    }

    // Set advanced flags:
    for (int i = 0; i < result.count(); ++i) {
        CMakeConfigItem &item = result[i];
        item.isAdvanced = advancedSet.contains(item.key);

        if (valuesMap.contains(item.key)) {
            item.values = CMakeConfigItem::cmakeSplitValue(QString::fromUtf8(valuesMap[item.key]));
        } else if (item.key  == "CMAKE_BUILD_TYPE") {
            // WA for known options
            item.values << "" << "Debug" << "Release" << "MinSizeRel" << "RelWithDebInfo";
        }
    }

    Utils::sort(result, CMakeConfigItem::sortOperator());

    return result;

}

QString CMakeConfigItem::toString(const Utils::MacroExpander *expander) const
{
    if (key.isEmpty() || type == CMakeProjectManager::CMakeConfigItem::STATIC)
        return QString();

    if (isUnset)
        return "unset " + QString::fromUtf8(key);

    QString typeStr;
    switch (type)
    {
    case CMakeProjectManager::CMakeConfigItem::FILEPATH:
        typeStr = QLatin1String("FILEPATH");
        break;
    case CMakeProjectManager::CMakeConfigItem::PATH:
        typeStr = QLatin1String("PATH");
        break;
    case CMakeProjectManager::CMakeConfigItem::BOOL:
        typeStr = QLatin1String("BOOL");
        break;
    case CMakeProjectManager::CMakeConfigItem::INTERNAL:
        typeStr = QLatin1String("INTERNAL");
        break;
    case CMakeProjectManager::CMakeConfigItem::STRING:
    default:
        typeStr = QLatin1String("STRING");
        break;
    }

    const QString expandedValue
            = expander ? expander->expand(QString::fromUtf8(value)) : QString::fromUtf8(value);
    return QString::fromUtf8(key) + QLatin1Char(':') + typeStr + QLatin1Char('=') + expandedValue;
}

QString CMakeConfigItem::toArgument(const Utils::MacroExpander *expander) const
{
    if (isUnset)
        return "-U" + QString::fromUtf8(key);
    return "-D" + toString(expander);
}

bool CMakeConfigItem::operator==(const CMakeConfigItem &o) const
{
    // type, isAdvanced and documentation do not matter for a match!
    return o.key == key && o.value == value && o.isUnset == isUnset;
}

#if WITH_TESTS

} // namespace CMakeProjectManager

#include "cmakeprojectplugin.h"

#include <QTest>

namespace CMakeProjectManager {
namespace Internal {

void CMakeProjectPlugin::testCMakeSplitValue_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("keepEmpty");
    QTest::addColumn<QStringList>("expectedOutput");

    // negative tests
    QTest::newRow("Empty input")
            << "" << false << QStringList();
    QTest::newRow("Empty input, keep empty")
            << "" << true << QStringList();

    QTest::newRow("single path")
            << "C:/something" << false << QStringList({"C:/something"});
    QTest::newRow("single path, keep empty")
            << "C:/something" << true << QStringList({"C:/something"});

    QTest::newRow(";single path")
            << ";C:/something" << false << QStringList({"C:/something"});
    QTest::newRow(";single path, keep empty")
            << ";C:/something" << true << QStringList({"", "C:/something"});

    QTest::newRow("single path;")
            << "C:/something;" << false << QStringList({"C:/something"});
    QTest::newRow("single path;, keep empty")
            << "C:/something;" << true << QStringList({"C:/something", ""});

    QTest::newRow("single path\\;")
            << "C:/something\\;" << false << QStringList({"C:/something;"});
    QTest::newRow("single path\\;, keep empty")
            << "C:/something\\;" << true << QStringList({"C:/something;"});

    QTest::newRow("single path\\;;second path")
            << "C:/something\\;;/second/path" << false << QStringList({"C:/something;", "/second/path"});
    QTest::newRow("single path\\;;second path, keep empty")
            << "C:/something\\;;/second/path" << true << QStringList({"C:/something;", "/second/path"});

    QTest::newRow("single path;;second path")
            << "C:/something;;/second/path" << false << QStringList({"C:/something", "/second/path"});
    QTest::newRow("single path;;second path, keep empty")
            << "C:/something;;/second/path" << true << QStringList({"C:/something", "", "/second/path"});
}

void CMakeProjectPlugin::testCMakeSplitValue()
{
    QFETCH(QString, input);
    QFETCH(bool, keepEmpty);
    QFETCH(QStringList, expectedOutput);

    const QStringList realOutput = CMakeConfigItem::cmakeSplitValue(input, keepEmpty);

    QCOMPARE(expectedOutput, realOutput);
}

} // namespace Internal
#endif

} // namespace CMakeProjectManager
