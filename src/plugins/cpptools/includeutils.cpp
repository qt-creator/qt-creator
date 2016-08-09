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

#include "includeutils.h"

#include <cplusplus/pp-engine.h>
#include <cplusplus/PreprocessorClient.h>
#include <cplusplus/PreprocessorEnvironment.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QTextBlock>
#include <QTextDocument>

#include <algorithm>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::IncludeUtils;
using namespace Utils;

namespace {

bool includeFileNamelessThen(const Include & left, const Include & right)
{ return left.unresolvedFileName() < right.unresolvedFileName(); }

int lineForAppendedIncludeGroup(const QList<IncludeGroup> &groups,
                                unsigned *newLinesToPrepend)
{
    if (newLinesToPrepend)
        *newLinesToPrepend += 1;
    return groups.last().last().line() + 1;
}

int lineForPrependedIncludeGroup(const QList<IncludeGroup> &groups,
                                 unsigned *newLinesToAppend)
{
    if (newLinesToAppend)
        *newLinesToAppend += 1;
    return groups.first().first().line();
}

QString includeDir(const QString &include)
{
    QString dirPrefix = QFileInfo(include).dir().path();
    if (dirPrefix == QLatin1String("."))
        return QString();
    dirPrefix.append(QLatin1Char('/'));
    return dirPrefix;
}

int lineAfterFirstComment(const QTextDocument *textDocument)
{
    int insertLine = -1;

    QTextBlock block = textDocument->firstBlock();
    while (block.isValid()) {
        const QString trimmedText = block.text().trimmed();

        // Only skip the first comment!
        if (trimmedText.startsWith(QLatin1String("/*"))) {
            do {
                const int pos = block.text().indexOf(QLatin1String("*/"));
                if (pos > -1) {
                    insertLine = block.blockNumber() + 2;
                    break;
                }
                block = block.next();
            } while (block.isValid());
            break;
        } else if (trimmedText.startsWith(QLatin1String("//"))) {
            block = block.next();
            while (block.isValid()) {
                if (!block.text().trimmed().startsWith(QLatin1String("//"))) {
                    insertLine = block.blockNumber() + 1;
                    break;
                }
                block = block.next();
            }
            break;
        }

        if (!trimmedText.isEmpty())
            break;
        block = block.next();
    }

    return insertLine;
}

} // anonymous namespace

LineForNewIncludeDirective::LineForNewIncludeDirective(const QTextDocument *textDocument,
                                                       const Document::Ptr cppDocument,
                                                       MocIncludeMode mocIncludeMode,
                                                       IncludeStyle includeStyle)
    : m_textDocument(textDocument)
    , m_cppDocument(cppDocument)
    , m_includeStyle(includeStyle)
{
    QList<Document::Include> includes
        = cppDocument->resolvedIncludes() + cppDocument->unresolvedIncludes();
    Utils::sort(includes, &Include::line);

    // Ignore *.moc includes if requested
    if (mocIncludeMode == IgnoreMocIncludes) {
        foreach (const Document::Include &include, includes) {
            if (!include.unresolvedFileName().endsWith(QLatin1String(".moc")))
                m_includes << include;
        }
    } else {
        m_includes = includes;
    }

    // Detect include style
    if (m_includeStyle == AutoDetect) {
        unsigned timesIncludeStyleChanged = 0;
        if (m_includes.isEmpty() || m_includes.size() == 1) {
            m_includeStyle = LocalBeforeGlobal; // Fallback
        } else {
            for (int i = 1, size = m_includes.size(); i < size; ++i) {
                if (m_includes.at(i - 1).type() != m_includes.at(i).type()) {
                    if (++timesIncludeStyleChanged > 1)
                        break;
                }
            }
            if (timesIncludeStyleChanged == 1) {
                m_includeStyle = m_includes.first().type() == Client::IncludeLocal
                    ? LocalBeforeGlobal
                    : GlobalBeforeLocal;
            } else {
                m_includeStyle = LocalBeforeGlobal; // Fallback
            }
        }
    }
}

int LineForNewIncludeDirective::findInsertLineForVeryFirstInclude(unsigned *newLinesToPrepend,
                                                                  unsigned *newLinesToAppend)
{
    int insertLine = 1;

    // If there is an include guard, insert right after that one
    const QByteArray includeGuardMacroName = m_cppDocument->includeGuardMacroName();
    if (!includeGuardMacroName.isEmpty()) {
        const QList<Macro> definedMacros = m_cppDocument->definedMacros();
        foreach (const Macro &definedMacro, definedMacros) {
            if (definedMacro.name() == includeGuardMacroName) {
                if (newLinesToPrepend)
                    *newLinesToPrepend = 1;
                if (newLinesToAppend)
                    *newLinesToAppend += 1;
                insertLine = definedMacro.line() + 1;
            }
        }
        QTC_CHECK(insertLine != 1);
    } else {
        // Otherwise, if there is a comment, insert right after it
        insertLine = lineAfterFirstComment(m_textDocument);
        if (insertLine != -1) {
            if (newLinesToPrepend)
                *newLinesToPrepend = 1;

        // Otherwise, insert at top of file
        } else {
            if (newLinesToAppend)
                *newLinesToAppend += 1;
            insertLine = 1;
        }
    }

    return insertLine;
}

int LineForNewIncludeDirective::operator()(const QString &newIncludeFileName,
                                           unsigned *newLinesToPrepend,
                                           unsigned *newLinesToAppend)
{
    if (newLinesToPrepend)
        *newLinesToPrepend = false;
    if (newLinesToAppend)
        *newLinesToAppend = false;

    const QString pureIncludeFileName = newIncludeFileName.mid(1, newIncludeFileName.length() - 2);
    const Client::IncludeType newIncludeType =
        newIncludeFileName.startsWith(QLatin1Char('"')) ? Client::IncludeLocal
                                                        : Client::IncludeGlobal;

    // Handle no includes
    if (m_includes.empty())
        return findInsertLineForVeryFirstInclude(newLinesToPrepend, newLinesToAppend);

    typedef QList<IncludeGroup> IncludeGroups;

    const IncludeGroups groupsNewline = IncludeGroup::detectIncludeGroupsByNewLines(m_includes);
    const bool includeAtTop
        = (newIncludeType == Client::IncludeLocal && m_includeStyle == LocalBeforeGlobal)
            || (newIncludeType == Client::IncludeGlobal && m_includeStyle == GlobalBeforeLocal);
    IncludeGroup bestGroup = includeAtTop ? groupsNewline.first() : groupsNewline.last();

    IncludeGroups groupsMatchingIncludeType = getGroupsByIncludeType(groupsNewline, newIncludeType);
    if (groupsMatchingIncludeType.isEmpty()) {
        const IncludeGroups groupsMixedIncludeType
            = IncludeGroup::filterMixedIncludeGroups(groupsNewline);
        // case: The new include goes into an own include group
        if (groupsMixedIncludeType.isEmpty()) {
            return includeAtTop
                ? lineForPrependedIncludeGroup(groupsNewline, newLinesToAppend)
                : lineForAppendedIncludeGroup(groupsNewline, newLinesToPrepend);
        // case: add to mixed group
        } else {
            const IncludeGroup bestMixedGroup = groupsMixedIncludeType.last(); // TODO: flaterize
            const IncludeGroups groupsIncludeType
                = IncludeGroup::detectIncludeGroupsByIncludeType(bestMixedGroup.includes());
            groupsMatchingIncludeType = getGroupsByIncludeType(groupsIncludeType, newIncludeType);
            // Avoid extra new lines for include groups which are not separated by new lines
            newLinesToPrepend = 0;
            newLinesToAppend = 0;
        }
    }

    IncludeGroups groupsSameIncludeDir;
    IncludeGroups groupsMixedIncludeDirs;
    foreach (const IncludeGroup &group, groupsMatchingIncludeType) {
        if (group.hasCommonIncludeDir())
            groupsSameIncludeDir << group;
        else
            groupsMixedIncludeDirs << group;
    }

    IncludeGroups groupsMatchingIncludeDir;
    foreach (const IncludeGroup &group, groupsSameIncludeDir) {
        if (group.commonIncludeDir() == includeDir(pureIncludeFileName))
            groupsMatchingIncludeDir << group;
    }

    // case: There are groups with a matching include dir, insert the new include
    //       at the best position of the best group
    if (!groupsMatchingIncludeDir.isEmpty()) {
        // The group with the longest common matching prefix is the best group
        int longestPrefixSoFar = 0;
        foreach (const IncludeGroup &group, groupsMatchingIncludeDir) {
            const int groupPrefixLength = group.commonPrefix().length();
            if (groupPrefixLength >= longestPrefixSoFar) {
                bestGroup = group;
                longestPrefixSoFar = groupPrefixLength;
            }
        }
    } else {
        // case: The new include goes into an own include group
        if (groupsMixedIncludeDirs.isEmpty()) {
            if (includeAtTop) {
                return groupsSameIncludeDir.isEmpty()
                    ? lineForPrependedIncludeGroup(groupsNewline, newLinesToAppend)
                    : lineForAppendedIncludeGroup(groupsSameIncludeDir, newLinesToPrepend);
            } else {
                return lineForAppendedIncludeGroup(groupsNewline, newLinesToPrepend);
            }
        // case: The new include is inserted at the best position of the best
        //       group with mixed include dirs
        } else {
            IncludeGroups groupsIncludeDir;
            foreach (const IncludeGroup &group, groupsMixedIncludeDirs) {
                groupsIncludeDir.append(
                            IncludeGroup::detectIncludeGroupsByIncludeDir(group.includes()));
            }
            IncludeGroup localBestIncludeGroup = IncludeGroup(QList<Include>());
            foreach (const IncludeGroup &group, groupsIncludeDir) {
                if (group.commonIncludeDir() == includeDir(pureIncludeFileName))
                    localBestIncludeGroup = group;
            }
            if (!localBestIncludeGroup.isEmpty())
                bestGroup = localBestIncludeGroup;
            else
                bestGroup = groupsMixedIncludeDirs.last();
        }
    }

    return bestGroup.lineForNewInclude(pureIncludeFileName, newIncludeType);
}

QList<IncludeGroup> LineForNewIncludeDirective::getGroupsByIncludeType(
        const QList<IncludeGroup> &groups, IncludeType includeType)
{
    return includeType == Client::IncludeLocal
        ? IncludeGroup::filterIncludeGroups(groups, Client::IncludeLocal)
        : IncludeGroup::filterIncludeGroups(groups, Client::IncludeGlobal);
}

/// includes will be modified!
QList<IncludeGroup> IncludeGroup::detectIncludeGroupsByNewLines(QList<Document::Include> &includes)
{
    // Create groups
    QList<IncludeGroup> result;
    unsigned lastLine = 0;
    QList<Include> currentIncludes;
    bool isFirst = true;
    foreach (const Include &include, includes) {
        // First include...
        if (isFirst) {
            isFirst = false;
            currentIncludes << include;
        // Include belongs to current group
        } else if (lastLine + 1 == include.line()) {
            currentIncludes << include;
        // Include is member of new group
        } else {
            result << IncludeGroup(currentIncludes);
            currentIncludes.clear();
            currentIncludes << include;
        }

        lastLine = include.line();
    }

    if (!currentIncludes.isEmpty())
        result << IncludeGroup(currentIncludes);

    return result;
}

QList<IncludeGroup> IncludeGroup::detectIncludeGroupsByIncludeDir(const QList<Include> &includes)
{
    // Create sub groups
    QList<IncludeGroup> result;
    QString lastDir;
    QList<Include> currentIncludes;
    bool isFirst = true;
    foreach (const Include &include, includes) {
        const QString currentDirPrefix = includeDir(include.unresolvedFileName());

        // First include...
        if (isFirst) {
            isFirst = false;
            currentIncludes << include;
        // Include belongs to current group
        } else if (lastDir == currentDirPrefix) {
            currentIncludes << include;
        // Include is member of new group
        } else {
            result << IncludeGroup(currentIncludes);
            currentIncludes.clear();
            currentIncludes << include;
        }

        lastDir = currentDirPrefix;
    }

    if (!currentIncludes.isEmpty())
        result << IncludeGroup(currentIncludes);

    return result;
}

QList<IncludeGroup> IncludeGroup::detectIncludeGroupsByIncludeType(const QList<Include> &includes)
{
    // Create sub groups
    QList<IncludeGroup> result;
    Client::IncludeType lastIncludeType = Client::IncludeLocal;
    QList<Include> currentIncludes;
    bool isFirst = true;
    foreach (const Include &include, includes) {
        const Client::IncludeType currentIncludeType = include.type();

        // First include...
        if (isFirst) {
            isFirst = false;
            currentIncludes << include;
        // Include belongs to current group
        } else if (lastIncludeType == currentIncludeType) {
            currentIncludes << include;
        // Include is member of new group
        } else {
            result << IncludeGroup(currentIncludes);
            currentIncludes.clear();
            currentIncludes << include;
        }

        lastIncludeType = currentIncludeType;
    }

    if (!currentIncludes.isEmpty())
        result << IncludeGroup(currentIncludes);

    return result;
}

/// returns groups that solely contains includes of the given include type
QList<IncludeGroup> IncludeGroup::filterIncludeGroups(const QList<IncludeGroup> &groups,
                                                      Client::IncludeType includeType)
{
    QList<IncludeGroup> result;
    foreach (const IncludeGroup &group, groups) {
        if (group.hasOnlyIncludesOfType(includeType))
            result << group;
    }
    return result;
}

/// returns groups that contains includes with local and globale include type
QList<IncludeGroup> IncludeGroup::filterMixedIncludeGroups(const QList<IncludeGroup> &groups)
{
    QList<IncludeGroup> result;
    foreach (const IncludeGroup &group, groups) {
        if (!group.hasOnlyIncludesOfType(Client::IncludeLocal)
            && !group.hasOnlyIncludesOfType(Client::IncludeGlobal)) {
            result << group;
        }
    }
    return result;
}

bool IncludeGroup::hasOnlyIncludesOfType(Client::IncludeType includeType) const
{
    foreach (const Include &include, m_includes) {
        if (include.type() != includeType)
            return false;
    }
    return true;
}

bool IncludeGroup::isSorted() const
{
    const QStringList names = filesNames();
    if (names.isEmpty() || names.size() == 1)
        return true;
    for (int i = 1, total = names.size(); i < total; ++i) {
        if (names.at(i) < names.at(i - 1))
            return false;
    }
    return true;
}

int IncludeGroup::lineForNewInclude(const QString &newIncludeFileName,
                                    Client::IncludeType newIncludeType) const
{
    if (m_includes.empty())
        return -1;

    if (isSorted()) {
        const Include newInclude(newIncludeFileName, QString(), 0, newIncludeType);
        const QList<Include>::const_iterator it = std::lower_bound(m_includes.begin(),
            m_includes.end(), newInclude, includeFileNamelessThen);
        if (it == m_includes.end())
            return m_includes.last().line() + 1;
        else
            return (*it).line();
    } else {
        return m_includes.last().line() + 1;
    }

    return -1;
}

QStringList IncludeGroup::filesNames() const
{
    QStringList names;
    foreach (const Include &include, m_includes)
        names << include.unresolvedFileName();
    return names;
}

QString IncludeGroup::commonPrefix() const
{
    const QStringList files = filesNames();
    if (files.size() <= 1)
        return QString(); // no prefix for single item groups
    return Utils::commonPrefix(files);
}

QString IncludeGroup::commonIncludeDir() const
{
    if (m_includes.isEmpty())
        return QString();
    return includeDir(m_includes.first().unresolvedFileName());
}

bool IncludeGroup::hasCommonIncludeDir() const
{
    if (m_includes.isEmpty())
        return false;

    const QString candidate = includeDir(m_includes.first().unresolvedFileName());
    for (int i = 1, size = m_includes.size(); i < size; ++i) {
        if (includeDir(m_includes.at(i).unresolvedFileName()) != candidate)
            return false;
    }
    return true;
}

#ifdef WITH_TESTS

#include "cppmodelmanager.h"
#include "cppsourceprocessertesthelper.h"
#include "cppsourceprocessor.h"
#include "cpptoolsplugin.h"
#include "cpptoolstestcase.h"

#include <QtTest>

using namespace Tests;
using CppTools::Internal::CppToolsPlugin;

static QList<Include> includesForSource(const QString &filePath)
{
    using namespace CppTools::Internal;
    CppModelManager *cmm = CppModelManager::instance();
    cmm->GC();
    QScopedPointer<CppSourceProcessor> sourceProcessor(CppModelManager::createSourceProcessor());
    sourceProcessor->setHeaderPaths(ProjectPartHeaderPaths()
                                    << ProjectPartHeaderPath(
                                        TestIncludePaths::globalIncludePath(),
                                        ProjectPartHeaderPath::IncludePath));
    sourceProcessor->run(filePath);

    Document::Ptr document = cmm->document(filePath);
    return document->resolvedIncludes();
}

void CppToolsPlugin::test_includeGroups_detectIncludeGroupsByNewLines()
{
    const QString testFilePath = TestIncludePaths::testFilePath(
                QLatin1String("test_main_detectIncludeGroupsByNewLines.cpp"));

    QList<Include> includes = includesForSource(testFilePath);
    QCOMPARE(includes.size(), 17);
    QList<IncludeGroup> includeGroups
        = IncludeGroup::detectIncludeGroupsByNewLines(includes);
    QCOMPARE(includeGroups.size(), 8);

    QCOMPARE(includeGroups.at(0).size(), 1);
    QVERIFY(includeGroups.at(0).commonPrefix().isEmpty());
    QVERIFY(includeGroups.at(0).hasOnlyIncludesOfType(Client::IncludeLocal));
    QVERIFY(includeGroups.at(0).isSorted());

    QCOMPARE(includeGroups.at(1).size(), 2);
    QVERIFY(!includeGroups.at(1).commonPrefix().isEmpty());
    QVERIFY(includeGroups.at(1).hasOnlyIncludesOfType(Client::IncludeLocal));
    QVERIFY(includeGroups.at(1).isSorted());

    QCOMPARE(includeGroups.at(2).size(), 2);
    QVERIFY(!includeGroups.at(2).commonPrefix().isEmpty());
    QVERIFY(includeGroups.at(2).hasOnlyIncludesOfType(Client::IncludeGlobal));
    QVERIFY(!includeGroups.at(2).isSorted());

    QCOMPARE(includeGroups.at(6).size(), 3);
    QVERIFY(includeGroups.at(6).commonPrefix().isEmpty());
    QVERIFY(includeGroups.at(6).hasOnlyIncludesOfType(Client::IncludeGlobal));
    QVERIFY(!includeGroups.at(6).isSorted());

    QCOMPARE(includeGroups.at(7).size(), 3);
    QVERIFY(includeGroups.at(7).commonPrefix().isEmpty());
    QVERIFY(!includeGroups.at(7).hasOnlyIncludesOfType(Client::IncludeLocal));
    QVERIFY(!includeGroups.at(7).hasOnlyIncludesOfType(Client::IncludeGlobal));
    QVERIFY(!includeGroups.at(7).isSorted());

    QCOMPARE(IncludeGroup::filterIncludeGroups(includeGroups, Client::IncludeLocal).size(), 4);
    QCOMPARE(IncludeGroup::filterIncludeGroups(includeGroups, Client::IncludeGlobal).size(), 3);
    QCOMPARE(IncludeGroup::filterMixedIncludeGroups(includeGroups).size(), 1);
}

void CppToolsPlugin::test_includeGroups_detectIncludeGroupsByIncludeDir()
{
    const QString testFilePath = TestIncludePaths::testFilePath(
                QLatin1String("test_main_detectIncludeGroupsByIncludeDir.cpp"));

    QList<Include> includes = includesForSource(testFilePath);
    QCOMPARE(includes.size(), 9);
    QList<IncludeGroup> includeGroups
        = IncludeGroup::detectIncludeGroupsByIncludeDir(includes);
    QCOMPARE(includeGroups.size(), 4);

    QCOMPARE(includeGroups.at(0).size(), 2);
    QVERIFY(includeGroups.at(0).commonIncludeDir().isEmpty());

    QCOMPARE(includeGroups.at(1).size(), 2);
    QCOMPARE(includeGroups.at(1).commonIncludeDir(), QLatin1String("lib/"));

    QCOMPARE(includeGroups.at(2).size(), 2);
    QCOMPARE(includeGroups.at(2).commonIncludeDir(), QLatin1String("otherlib/"));

    QCOMPARE(includeGroups.at(3).size(), 3);
    QCOMPARE(includeGroups.at(3).commonIncludeDir(), QLatin1String(""));
}

void CppToolsPlugin::test_includeGroups_detectIncludeGroupsByIncludeType()
{
    const QString testFilePath = TestIncludePaths::testFilePath(
                QLatin1String("test_main_detectIncludeGroupsByIncludeType.cpp"));

    QList<Include> includes = includesForSource(testFilePath);
    QCOMPARE(includes.size(), 9);
    QList<IncludeGroup> includeGroups
        = IncludeGroup::detectIncludeGroupsByIncludeDir(includes);
    QCOMPARE(includeGroups.size(), 4);

    QCOMPARE(includeGroups.at(0).size(), 2);
    QVERIFY(includeGroups.at(0).hasOnlyIncludesOfType(Client::IncludeLocal));

    QCOMPARE(includeGroups.at(1).size(), 2);
    QVERIFY(includeGroups.at(1).hasOnlyIncludesOfType(Client::IncludeGlobal));

    QCOMPARE(includeGroups.at(2).size(), 2);
    QVERIFY(includeGroups.at(2).hasOnlyIncludesOfType(Client::IncludeLocal));

    QCOMPARE(includeGroups.at(3).size(), 3);
    QVERIFY(includeGroups.at(3).hasOnlyIncludesOfType(Client::IncludeGlobal));
}

#endif // WITH_TESTS
