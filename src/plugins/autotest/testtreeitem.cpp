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

#include "autotestconstants.h"
#include "itestparser.h"
#include "testconfiguration.h"
#include "testtreeitem.h"

#include <cplusplus/Icons.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <texteditor/texteditor.h>
#include <utils/utilsicons.h>

#include <QIcon>

namespace Autotest {

TestTreeItem::TestTreeItem(ITestFramework *framework, const QString &name,
                           const QString &filePath, Type type)
    : m_framework(framework),
      m_name(name),
      m_filePath(filePath),
      m_type(type)
{
    switch (m_type) {
    case Root:
    case GroupNode:
    case TestSuite:
    case TestCase:
    case TestFunction:
        m_checked = Qt::Checked;
        break;
    default:
        m_checked = Qt::Unchecked;
        break;
    }
}

static QIcon testTreeIcon(TestTreeItem::Type type)
{
    static QIcon icons[] = {
        QIcon(),
        Utils::Icons::OPENFILE.icon(),
        QIcon(":/autotest/images/suite.png"),
        Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::Class),
        Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::SlotPrivate),
        QIcon(":/autotest/images/data.png")
    };

    if (int(type) >= int(sizeof icons / sizeof *icons))
        return icons[3];
    return icons[type];
}

QVariant TestTreeItem::data(int /*column*/, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (m_type == Root && childCount() == 0)
            return QCoreApplication::translate("TestTreeItem", "%1 (none)").arg(m_name);
        else
            return m_name;
    case Qt::ToolTipRole:
        return m_filePath;
    case Qt::DecorationRole:
        return testTreeIcon(m_type);
    case Qt::CheckStateRole:
        return QVariant();
    case LinkRole: {
        if (m_type == GroupNode)
            return QVariant();
        QVariant itemLink;
        itemLink.setValue(Utils::Link(m_filePath, int(m_line), int(m_column)));
        return itemLink;
    }
    case ItalicRole:
        return false;
    case TypeRole:
        return m_type;
    case EnabledRole:
        return true;
    }
    return QVariant();
}

bool TestTreeItem::setData(int /*column*/, const QVariant &data, int role)
{
    if (role == Qt::CheckStateRole) {
        Qt::CheckState old = m_checked;
        m_checked = Qt::CheckState(data.toInt());
        return m_checked != old;
    }
    return false;
}

Qt::ItemFlags TestTreeItem::flags(int /*column*/) const
{
    static const Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    switch (m_type) {
    case Root:
    case GroupNode:
        return Qt::ItemIsEnabled | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable;
    case TestSuite:
    case TestCase:
        return defaultFlags | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable;
    case TestFunction:
        return defaultFlags | Qt::ItemIsUserCheckable;
    default:
        return defaultFlags;
    }
}

bool TestTreeItem::modifyTestCaseOrSuiteContent(const TestParseResult *result)
{
    bool hasBeenModified = modifyName(result->name);
    hasBeenModified |= modifyLineAndColumn(result);
    return hasBeenModified;
}

bool TestTreeItem::modifyTestFunctionContent(const TestParseResult *result)
{
    bool hasBeenModified = modifyFilePath(result->fileName);
    hasBeenModified |= modifyLineAndColumn(result);
    return hasBeenModified;
}

bool TestTreeItem::modifyDataTagContent(const TestParseResult *result)
{

    bool hasBeenModified = modifyTestFunctionContent(result);
    hasBeenModified |= modifyName(result->name);
    return hasBeenModified;
}

bool TestTreeItem::modifyLineAndColumn(const TestParseResult *result)
{
    bool hasBeenModified = false;
    if (m_line != result->line) {
        m_line = result->line;
        hasBeenModified = true;
    }
    if (m_column != result->column) {
        m_column = result->column;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

Qt::CheckState TestTreeItem::checked() const
{
    switch (m_type) {
    case Root:
    case GroupNode:
    case TestSuite:
    case TestCase:
    case TestFunction:
    case TestDataTag:
        return m_checked;
    default:
        return Qt::Unchecked;
    }
}

void TestTreeItem::markForRemoval(bool mark)
{
    m_status = mark ? MarkedForRemoval : Cleared;
}

void TestTreeItem::markForRemovalRecursively(bool mark)
{
    markForRemoval(mark);
    for (int row = 0, count = childCount(); row < count; ++row)
        childAt(row)->markForRemovalRecursively(mark);
}

void TestTreeItem::markForRemovalRecursively(const QString &filePath)
{
    bool mark = m_filePath == filePath;
    forFirstLevelChildren([&mark, &filePath](TestTreeItem *child) {
        child->markForRemovalRecursively(filePath);
        mark &= child->markedForRemoval();
    });
    markForRemoval(mark);
}

TestTreeItem *TestTreeItem::parentItem() const
{
    return static_cast<TestTreeItem *>(parent());
}

TestTreeItem *TestTreeItem::findChildByName(const QString &name)
{
    return findFirstLevelChild([name](const TestTreeItem *other) { return other->name() == name; });
}

TestTreeItem *TestTreeItem::findChildByFile(const QString &filePath)
{
    return findFirstLevelChild([filePath](const TestTreeItem *other) {
        return other->filePath() == filePath;
    });
}

TestTreeItem *TestTreeItem::findChildByFileAndType(const QString &filePath, Type tType)
{
    return findFirstLevelChild([filePath, tType](const TestTreeItem *other) {
        return other->type() == tType && other->filePath() == filePath;
    });
}

TestTreeItem *TestTreeItem::findChildByNameAndFile(const QString &name, const QString &filePath)
{
    return findFirstLevelChild([name, filePath](const TestTreeItem *other) {
        return other->filePath() == filePath && other->name() == name;
    });
}

TestConfiguration *TestTreeItem::asConfiguration(TestRunMode mode) const
{
    switch (mode) {
    case TestRunMode::Run:
    case TestRunMode::RunWithoutDeploy:
        return testConfiguration();
    case TestRunMode::Debug:
    case TestRunMode::DebugWithoutDeploy:
        return debugConfiguration();
    default:
        break;
    }
    return nullptr;
}

QList<TestConfiguration *> TestTreeItem::getAllTestConfigurations() const
{
    return QList<TestConfiguration *>();
}

QList<TestConfiguration *> TestTreeItem::getSelectedTestConfigurations() const
{
    return QList<TestConfiguration *>();
}

QList<TestConfiguration *> TestTreeItem::getTestConfigurationsForFile(const Utils::FilePath &) const
{
    return QList<TestConfiguration *>();
}

bool TestTreeItem::lessThan(const TestTreeItem *other, SortMode mode) const
{
    const QString &lhs = data(0, Qt::DisplayRole).toString();
    const QString &rhs = other->data(0, Qt::DisplayRole).toString();

    switch (mode) {
    case Alphabetically:
        if (lhs == rhs)
            return index().row() > other->index().row();
        return lhs > rhs;
    case Naturally: {
        if (m_type == GroupNode && other->type() == GroupNode)
            return m_filePath > other->filePath();

        const Utils::Link &leftLink = data(0, LinkRole).value<Utils::Link>();
        const Utils::Link &rightLink = other->data(0, LinkRole).value<Utils::Link>();
        if (leftLink.targetFileName == rightLink.targetFileName) {
            return leftLink.targetLine == rightLink.targetLine
                    ? leftLink.targetColumn > rightLink.targetColumn
                    : leftLink.targetLine > rightLink.targetLine;
        }
        return leftLink.targetFileName > rightLink.targetFileName;
    }
    }
    return true;
}

bool TestTreeItem::isGroupNodeFor(const TestTreeItem *other) const
{
    QTC_ASSERT(other, return false);
    if (type() != TestTreeItem::GroupNode)
        return false;

    // for now there's only the possibility to have 'Folder' nodes
    return QFileInfo(other->filePath()).absolutePath() == filePath();
}

bool TestTreeItem::isGroupable() const
{
    return true;
}

QSet<QString> TestTreeItem::internalTargets() const
{
    auto cppMM = CppTools::CppModelManager::instance();
    const QList<CppTools::ProjectPart::Ptr> projectParts = cppMM->projectPart(m_filePath);
    // if we have no project parts it's most likely a header with declarations only and CMake based
    if (projectParts.isEmpty())
        return TestTreeItem::dependingInternalTargets(cppMM, m_filePath);
    QSet<QString> targets;
    for (const CppTools::ProjectPart::Ptr &part : projectParts) {
        targets.insert(part->buildSystemTarget);
        if (part->buildTargetType != ProjectExplorer::BuildTargetType::Executable)
            targets.unite(TestTreeItem::dependingInternalTargets(cppMM, m_filePath));
    }
    return targets;
}

void TestTreeItem::copyBasicDataFrom(const TestTreeItem *other)
{
    if (!other)
        return;
    m_name = other->m_name;
    m_filePath = other->m_filePath;
    m_type = other->m_type;
    m_checked = other->m_checked;
    m_line = other->m_line;
    m_column = other->m_column;
    m_proFile = other->m_proFile;
    m_status = other->m_status;
}

inline bool TestTreeItem::modifyFilePath(const QString &filePath)
{
    if (m_filePath != filePath) {
        m_filePath = filePath;
        return true;
    }
    return false;
}

inline bool TestTreeItem::modifyName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        return true;
    }
    return false;
}

ITestFramework *TestTreeItem::framework() const
{
    return m_framework;
}

/*
 * try to find build system target that depends on the given file - if the file is no header
 * try to find the corresponding header and use this instead to find the respective target
 */
QSet<QString> TestTreeItem::dependingInternalTargets(CppTools::CppModelManager *cppMM,
                                                     const QString &file)
{
    QSet<QString> result;
    QTC_ASSERT(cppMM, return result);
    const CPlusPlus::Snapshot snapshot = cppMM->snapshot();
    QTC_ASSERT(snapshot.contains(file), return result);
    bool wasHeader;
    const QString correspondingFile
            = CppTools::correspondingHeaderOrSource(file, &wasHeader, CppTools::CacheUsage::ReadOnly);
    const Utils::FilePaths dependingFiles = snapshot.filesDependingOn(
                wasHeader ? file : correspondingFile);
    for (const Utils::FilePath &fn : dependingFiles) {
        for (const CppTools::ProjectPart::Ptr &part : cppMM->projectPart(fn))
            result.insert(part->buildSystemTarget);
    }
    return result;
}

} // namespace Autotest
