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


#include "includeutils.h"

#include <cplusplus/pp-engine.h>
#include <cplusplus/PreprocessorClient.h>
#include <cplusplus/PreprocessorEnvironment.h>

#include <utils/stringutils.h>

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QTextBlock>
#include <QTextDocument>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::IncludeUtils;
using namespace Utils;

static bool includeLineLessThan(const Include &left, const Include &right)
{ return left.line() < right.line(); }

static bool includeFileNamelessThen(const Include & left, const Include & right)
{ return left.unresolvedFileName() < right.unresolvedFileName(); }

LineForNewIncludeDirective::LineForNewIncludeDirective(const QTextDocument *textDocument,
                                                       QList<Document::Include> includes,
                                                       MocIncludeMode mocIncludeMode,
                                                       IncludeStyle includeStyle)
    : m_textDocument(textDocument)
    , m_includeStyle(includeStyle)
{
    // Ignore *.moc includes if requested
    if (mocIncludeMode == IgnoreMocIncludes) {
        foreach (const Document::Include &include, includes) {
            if (!include.unresolvedFileName().endsWith(QLatin1String(".moc")))
                m_includes << include;
        }
    } else {
        m_includes = includes;
    }

    // TODO: Remove this filter loop once FastPreprocessor::sourceNeeded does not add
    // extra includes anymore.
    for (int i = m_includes.count() - 1; i >= 0; --i) {
        if (!QFileInfo(m_includes.at(i).resolvedFileName()).isAbsolute())
            m_includes.removeAt(i);
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

int LineForNewIncludeDirective::operator()(const QString &newIncludeFileName,
                                           unsigned *newLinesToPrepend,
                                           unsigned *newLinesToAppend)
{
    if (newLinesToPrepend)
        *newLinesToPrepend = false;
    if (newLinesToAppend)
        *newLinesToAppend = false;

    const QString pureIncludeFileName = newIncludeFileName.mid(1, newIncludeFileName.length() - 2);
    const CPlusPlus::Client::IncludeType newIncludeType =
        newIncludeFileName.startsWith(QLatin1Char('"')) ? Client::IncludeLocal
                                                        : Client::IncludeGlobal;

    // Handle no includes
    if (m_includes.empty()) {
        unsigned insertLine = 0;

        QTextBlock block = m_textDocument->firstBlock();
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

        if (insertLine == 0) {
            if (newLinesToAppend)
                *newLinesToAppend += 1;
            insertLine = 1;
        } else {
            if (newLinesToPrepend)
                *newLinesToPrepend = 1;
        }
        return insertLine;
    }

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
                ? IncludeGroup::lineForPrependedIncludeGroup(groupsNewline, newLinesToAppend)
                : IncludeGroup::lineForAppendedIncludeGroup(groupsNewline, newLinesToPrepend);
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
        if (group.commonIncludeDir() == IncludeGroup::includeDir(pureIncludeFileName))
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
                    ? IncludeGroup::lineForPrependedIncludeGroup(groupsNewline, newLinesToAppend)
                    : IncludeGroup::lineForAppendedIncludeGroup(groupsSameIncludeDir, newLinesToPrepend);
            } else {
                return IncludeGroup::lineForAppendedIncludeGroup(groupsNewline, newLinesToPrepend);
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
                if (group.commonIncludeDir() == IncludeGroup::includeDir(pureIncludeFileName))
                    localBestIncludeGroup = group;
            }
            if (!localBestIncludeGroup.isEmpty()) {
                bestGroup = localBestIncludeGroup;
            } else {
                bestGroup = groupsMixedIncludeDirs.last();
            }
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
    // Sort by line
    qSort(includes.begin(), includes.end(), includeLineLessThan);

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
        }
        // Include belongs to current group
        else if (lastLine + 1 == include.line()) {
            currentIncludes << include;
        }
        // Include is member of new group
        else {
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
        }
        // Include belongs to current group
        else if (lastDir == currentDirPrefix) {
            currentIncludes << include;
        }
        // Include is member of new group
        else {
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
    CPlusPlus::Client::IncludeType lastIncludeType;
    QList<Include> currentIncludes;
    bool isFirst = true;
    foreach (const Include &include, includes) {
        const CPlusPlus::Client::IncludeType currentIncludeType = include.type();

        // First include...
        if (isFirst) {
            isFirst = false;
            currentIncludes << include;
        }
        // Include belongs to current group
        else if (lastIncludeType == currentIncludeType) {
            currentIncludes << include;
        }
        // Include is member of new group
        else {
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

QString IncludeGroup::includeDir(const QString &include)
{
    QString dirPrefix = QFileInfo(include).dir().path();
    if (dirPrefix == QLatin1String("."))
        return QString();
    dirPrefix.append(QLatin1Char('/'));
    return dirPrefix;
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
        const Include newInclude(newIncludeFileName, QString(), -1, newIncludeType);
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

int IncludeGroup::lineForAppendedIncludeGroup(const QList<IncludeGroup> &groups,
                                              unsigned *newLinesToPrepend)
{
    if (newLinesToPrepend)
        *newLinesToPrepend += 1;
    return groups.last().last().line() + 1;
}

int IncludeGroup::lineForPrependedIncludeGroup(const QList<IncludeGroup> &groups,
                                               unsigned *newLinesToAppend)
{
    if (newLinesToAppend)
        *newLinesToAppend += 1;
    return groups.first().first().line();
}
