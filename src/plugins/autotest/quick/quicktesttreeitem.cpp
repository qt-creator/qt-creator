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
        case Root:
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

TestConfiguration *QuickTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return nullptr);

    QuickTestConfiguration *config = nullptr;
    switch (type()) {
    case TestCase: {
        QStringList testFunctions;
        for (int row = 0, count = childCount(); row < count; ++row)
            testFunctions << name() + "::" + childItem(row)->name();
        config = new QuickTestConfiguration;
        config->setTestCases(testFunctions);
        config->setProFile(proFile());
        config->setProject(project);
        break;
    }
    case TestFunctionOrSet: {
        TestTreeItem *parent = parentItem();
        QStringList testFunction(parent->name() + "::" + name());
        config = new QuickTestConfiguration;
        config->setTestCases(testFunction);
        config->setProFile(parent->proFile());
        config->setProject(project);
        break;
    }
    default:
        return nullptr;
    }
    return config;
}

QList<TestConfiguration *> QuickTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<QString, int> foundProFiles;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);
        // unnamed Quick Tests must be handled separately
        if (child->name().isEmpty()) {
            for (int childRow = 0, ccount = child->childCount(); childRow < ccount; ++ childRow) {
                const TestTreeItem *grandChild = child->childItem(childRow);
                const QString &proFile = grandChild->proFile();
                foundProFiles.insert(proFile, foundProFiles[proFile] + 1);
            }
            continue;
        }
        // named Quick Test
        const QString &proFile = child->proFile();
        foundProFiles.insert(proFile, foundProFiles[proFile] + child->childCount());
    }
    // create TestConfiguration for each project file
    QHash<QString, int>::ConstIterator it = foundProFiles.begin();
    QHash<QString, int>::ConstIterator end = foundProFiles.end();
    for ( ; it != end; ++it) {
        QuickTestConfiguration *tc = new QuickTestConfiguration;
        tc->setTestCaseCount(it.value());
        tc->setProFile(it.key());
        tc->setProject(project);
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

    QuickTestConfiguration *tc = nullptr;
    QHash<QString, QuickTestConfiguration *> foundProFiles;
    // unnamed Quick Tests must be handled first
    if (TestTreeItem *unnamed = unnamedQuickTests()) {
        for (int childRow = 0, ccount = unnamed->childCount(); childRow < ccount; ++ childRow) {
            const TestTreeItem *grandChild = unnamed->childItem(childRow);
            const QString &proFile = grandChild->proFile();
            if (foundProFiles.contains(proFile)) {
                QTC_ASSERT(tc,
                           qWarning() << "Illegal state (unnamed Quick Test listed as named)";
                           return QList<TestConfiguration *>());
                foundProFiles[proFile]->setTestCaseCount(tc->testCaseCount() + 1);
            } else {
                tc = new QuickTestConfiguration;
                tc->setTestCaseCount(1);
                tc->setUnnamedOnly(true);
                tc->setProFile(proFile);
                tc->setProject(project);
                foundProFiles.insert(proFile, tc);
            }
        }
    }

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);
        // unnamed Quick Tests have been handled separately already
        if (child->name().isEmpty())
            continue;

        // named Quick Tests
        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked:
        case Qt::PartiallyChecked:
        default:
            QStringList testFunctions;
            int grandChildCount = child->childCount();
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->childItem(grandChildRow);
                if (grandChild->checked() != Qt::Checked || grandChild->type() != TestFunctionOrSet)
                    continue;
                testFunctions << child->name() + "::" + grandChild->name();
            }
            if (foundProFiles.contains(child->proFile())) {
                tc = foundProFiles[child->proFile()];
                QStringList oldFunctions(tc->testCases());
                // if oldFunctions.size() is 0 this test configuration is used for at least one
                // unnamed test case
                if (oldFunctions.size() == 0) {
                    tc->setTestCaseCount(tc->testCaseCount() + testFunctions.size());
                    tc->setUnnamedOnly(false);
                } else {
                    oldFunctions << testFunctions;
                    tc->setTestCases(oldFunctions);
                }
            } else {
                tc = new QuickTestConfiguration;
                tc->setTestCases(testFunctions);
                tc->setProFile(child->proFile());
                tc->setProject(project);
                foundProFiles.insert(child->proFile(), tc);
            }
            break;
        }
    }
    QHash<QString, QuickTestConfiguration *>::ConstIterator it = foundProFiles.begin();
    QHash<QString, QuickTestConfiguration *>::ConstIterator end = foundProFiles.end();
    for ( ; it != end; ++it) {
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
        return result->name.isEmpty() ? unnamedQuickTests() : findChildByFile(result->fileName);
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
        return result->name.isEmpty() ? false : modifyTestCaseContent(result->name, result->line,
                                                                      result->column);
    case TestFunctionOrSet:
    case TestDataFunction:
    case TestSpecialFunction:
        return name().isEmpty() ? modifyLineAndColumn(result->line, result->column)
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
