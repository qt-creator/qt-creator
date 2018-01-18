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

#include "quicktesttreeitem.h"
#include "quicktestconfiguration.h"
#include "quicktestparser.h"
#include "../testframeworkmanager.h"

#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

QVariant QuickTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (type() == TestCase && name().isEmpty())
            return QCoreApplication::translate("QuickTestTreeItem", "<unnamed>");
        break;
    case Qt::ToolTipRole:
        if (type() == TestCase && name().isEmpty())
            return QCoreApplication::translate("QuickTestTreeItem",
                                               "<p>Give all test cases a name to ensure correct "
                                               "behavior when running test cases and to be able to "
                                               "select them.</p>");
        break;
    case Qt::CheckStateRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
        case TestDataTag:
            return QVariant();
        case TestCase:
            return name().isEmpty() ? QVariant() : checked();
        case TestFunctionOrSet:
            return (parentItem() && !parentItem()->name().isEmpty()) ? checked() : QVariant();
        default:
            return checked();
        }

    case ItalicRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            return true;
        case TestCase:
            return name().isEmpty();
        case TestFunctionOrSet:
            return parentItem() ? parentItem()->name().isEmpty() : false;
        default:
            return false;
        }
    default:
        break;
    }
    return TestTreeItem::data(column, role);
}

Qt::ItemFlags QuickTestTreeItem::flags(int column) const
{
    // handle unnamed quick tests and their functions
    switch (type()) {
    case TestCase:
        if (name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        break;
    case TestFunctionOrSet:
        if (parentItem()->name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        break;
    default: {} // avoid warning regarding unhandled enum values
    }
    return TestTreeItem::flags(column);
}

bool QuickTestTreeItem::canProvideTestConfiguration() const
{
    switch (type()) {
    case TestCase:
        return !name().isEmpty();
    case TestFunctionOrSet:
        return !parentItem()->name().isEmpty();
    default:
        return false;
    }
}

bool QuickTestTreeItem::canProvideDebugConfiguration() const
{
    return canProvideTestConfiguration();
}

TestConfiguration *QuickTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return nullptr);

    QuickTestConfiguration *config = nullptr;
    switch (type()) {
    case TestCase: {
        QStringList testFunctions;
        for (int row = 0, count = childCount(); row < count; ++row) {
            const TestTreeItem *child = childItem(row);
            if (child->type() == TestTreeItem::TestSpecialFunction)
                continue;
            testFunctions << name() + "::" + child->name();
        }
        config = new QuickTestConfiguration;
        config->setTestCases(testFunctions);
        config->setProjectFile(proFile());
        config->setProject(project);
        break;
    }
    case TestFunctionOrSet: {
        TestTreeItem *parent = parentItem();
        QStringList testFunction(parent->name() + "::" + name());
        config = new QuickTestConfiguration;
        config->setTestCases(testFunction);
        config->setProjectFile(parent->proFile());
        config->setProject(project);
        break;
    }
    default:
        return nullptr;
    }
    if (config)
        config->setInternalTargets(internalTargets());
    return config;
}

static void testConfigurationFromCheckState(const TestTreeItem *item,
                                            QHash<QString, QuickTestConfiguration *> &foundProFiles)
{
    QTC_ASSERT(item, return);
    if (item->type() == TestTreeItem::GroupNode) {
        for (int row = 0, count = item->childCount(); row < count; ++row)
            testConfigurationFromCheckState(item->childItem(row), foundProFiles);
        return;
    }
    QTC_ASSERT(item->type() == TestTreeItem::TestCase, return);
    QuickTestConfiguration *tc = nullptr;
    if (item->checked() == Qt::Unchecked)
        return;

    QStringList testFunctions;
    const int childCount = item->childCount();
    for (int childRow = 0; childRow < childCount; ++childRow) {
        const TestTreeItem *child = item->childItem(childRow);
        if (child->checked() != Qt::Checked || child->type() != TestTreeItem::TestFunctionOrSet)
            continue;
        testFunctions << item->name() + "::" + child->name();
    }
    if (foundProFiles.contains(item->proFile())) {
        tc = foundProFiles[item->proFile()];
        QStringList oldFunctions(tc->testCases());
        oldFunctions << testFunctions;
        tc->setTestCases(oldFunctions);
    } else {
        tc = new QuickTestConfiguration;
        tc->setTestCases(testFunctions);
        tc->setProjectFile(item->proFile());
        tc->setProject(ProjectExplorer::SessionManager::startupProject());
        tc->setInternalTargets(item->internalTargets());
        foundProFiles.insert(item->proFile(), tc);
    }
}

TestConfiguration *QuickTestTreeItem::debugConfiguration() const
{
    QuickTestConfiguration *config = static_cast<QuickTestConfiguration *>(testConfiguration());
    if (config)
        config->setRunMode(TestRunMode::Debug);
    return config;
}

struct Tests {
    int testCount = 0;
    QSet<QString> internalTargets;
};

static void addTestsForItem(Tests &tests, const TestTreeItem *item)
{
    tests.testCount += item->childCount();
    tests.internalTargets = item->internalTargets();
}

QList<TestConfiguration *> QuickTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<QString, Tests> testsForProfile;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);
        // unnamed Quick Tests must be handled separately
        if (child->name().isEmpty()) {
            for (int childRow = 0, ccount = child->childCount(); childRow < ccount; ++ childRow) {
                const TestTreeItem *grandChild = child->childItem(childRow);
                const QString &proFile = grandChild->proFile();
                ++(testsForProfile[proFile].testCount);
                testsForProfile[proFile].internalTargets = grandChild->internalTargets();
            }
            continue;
        }
        // named Quick Test
        if (child->type() == TestCase) {
            addTestsForItem(testsForProfile[child->proFile()], child);
        } else if (child->type() == GroupNode) {
            const int groupCount = child->childCount();
            for (int groupRow = 0; groupRow < groupCount; ++groupRow) {
                const TestTreeItem *grandChild = child->childItem(groupRow);
                addTestsForItem(testsForProfile[grandChild->proFile()], grandChild);
            }
        }
    }
    // create TestConfiguration for each project file
    for (auto it = testsForProfile.begin(), end = testsForProfile.end(); it != end; ++it) {
        QuickTestConfiguration *tc = new QuickTestConfiguration;
        tc->setTestCaseCount(it.value().testCount);
        tc->setProjectFile(it.key());
        tc->setProject(project);
        tc->setInternalTargets(it.value().internalTargets);
        result << tc;
    }
    return result;
}

QList<TestConfiguration *> QuickTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<QString, QuickTestConfiguration *> foundProFiles;

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);
        // unnamed Quick Tests cannot get selected explicitly
        if (child->name().isEmpty())
            continue;

        // named Quick Tests
        testConfigurationFromCheckState(child, foundProFiles);
    }

    for (auto it = foundProFiles.begin(), end = foundProFiles.end(); it != end; ++it) {
        QuickTestConfiguration *config = it.value();
        if (!config->unnamedOnly())
            result << config;
        else
            delete config;
    }

    return result;
}

TestTreeItem *QuickTestTreeItem::find(const TestParseResult *result)
{
    QTC_ASSERT(result, return nullptr);

    switch (type()) {
    case Root:
        if (result->name.isEmpty())
            return unnamedQuickTests();
        if (TestFrameworkManager::instance()->groupingEnabled(result->frameworkId)) {
            const QString path = QFileInfo(result->fileName).absolutePath();
            for (int row = 0; row < childCount(); ++row) {
                TestTreeItem *group = childItem(row);
                if (group->filePath() != path)
                    continue;
                if (auto groupChild = group->findChildByFile(result->fileName))
                    return groupChild;
            }
            return nullptr;
        }
        return findChildByFile(result->fileName);
    case GroupNode:
        return findChildByFile(result->fileName);
    case TestCase:
        return name().isEmpty() ? findChildByNameAndFile(result->name, result->fileName)
                                : findChildByName(result->name);
    default:
        return nullptr;
    }
}

bool QuickTestTreeItem::modify(const TestParseResult *result)
{
    QTC_ASSERT(result, return false);

    switch (type()) {
    case TestCase:
        return result->name.isEmpty() ? false : modifyTestCaseContent(result);
    case TestFunctionOrSet:
    case TestDataFunction:
    case TestSpecialFunction:
        return name().isEmpty() ? modifyLineAndColumn(result)
                                : modifyTestFunctionContent(result);
    default:
        return false;
    }
}

bool QuickTestTreeItem::lessThan(const TestTreeItem *other, TestTreeItem::SortMode mode) const
{
    // handle special item (<unnamed>)
    if (name().isEmpty())
        return false;
    if (other->name().isEmpty())
        return true;
    return TestTreeItem::lessThan(other, mode);
}

bool QuickTestTreeItem::isGroupNodeFor(const TestTreeItem *other) const
{
    QTC_ASSERT(other, return false);
    if (other->name().isEmpty()) // unnamed quick tests will not get grouped
        return false;
    return TestTreeItem::isGroupNodeFor(other);
}

bool QuickTestTreeItem::removeOnSweepIfEmpty() const
{
    return TestTreeItem::removeOnSweepIfEmpty()
            || (type() == TestCase && name().isEmpty()); // remove pseudo item '<unnamed>'
}

TestTreeItem *QuickTestTreeItem::createParentGroupNode() const
{
    if (filePath().isEmpty() || name().isEmpty())
        return nullptr;
    if (type() == TestFunctionOrSet)
        return nullptr;

    const QFileInfo fileInfo(filePath());
    const QFileInfo base(fileInfo.absolutePath());
    return new QuickTestTreeItem(base.baseName(), fileInfo.absolutePath(), TestTreeItem::GroupNode);
}

QSet<QString> QuickTestTreeItem::internalTargets() const
{
    QSet<QString> result;
    const auto cppMM = CppTools::CppModelManager::instance();
    const auto projectInfo = cppMM->projectInfo(ProjectExplorer::SessionManager::startupProject());
    for (const CppTools::ProjectPart::Ptr projectPart : projectInfo.projectParts()) {
        if (projectPart->buildTargetType != CppTools::ProjectPart::Executable)
            continue;
        if (projectPart->projectFile == proFile()) {
            result.insert(projectPart->buildSystemTarget + '|' + projectPart->projectFile);
            break;
        }
    }
    return result;
}

TestTreeItem *QuickTestTreeItem::unnamedQuickTests() const
{
    if (type() != Root)
        return nullptr;

    for (int row = 0, count = childCount(); row < count; ++row) {
        TestTreeItem *child = childItem(row);
        if (child->name().isEmpty())
            return child;
    }
    return nullptr;
}

} // namespace Internal
} // namespace Autotest
