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

#include "qttesttreeitem.h"
#include "qttestconfiguration.h"
#include "qttestparser.h"
#include "../autotest_utils.h"

#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

QtTestTreeItem::QtTestTreeItem(const QString &name, const QString &filePath, TestTreeItem::Type type)
    : TestTreeItem(name, filePath, type)
{
    if (type == TestDataTag)
        setChecked(Qt::Checked);
}

QVariant QtTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (type() == Root)
            break;
        return QVariant(name() + nameSuffix());
    case Qt::CheckStateRole:
        switch (type()) {
        case Root:
        case TestDataFunction:
        case TestSpecialFunction:
            return QVariant();
        default:
            return checked();
        }
    case ItalicRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            return true;
        default:
            return false;
        }
    }
    return TestTreeItem::data(column, role);
}

Qt::ItemFlags QtTestTreeItem::flags(int column) const
{
    static const Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    switch (type()) {
    case TestDataTag:
        return defaultFlags | Qt::ItemIsUserCheckable;
    case TestFunctionOrSet:
        return defaultFlags | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable;
    default:
        return TestTreeItem::flags(column);
    }
}

bool QtTestTreeItem::canProvideTestConfiguration() const
{
    switch (type()) {
    case TestCase:
    case TestFunctionOrSet:
    case TestDataTag:
        return true;
    default:
        return false;
    }
}

bool QtTestTreeItem::canProvideDebugConfiguration() const
{
    return canProvideTestConfiguration();
}

TestConfiguration *QtTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return nullptr);

    QtTestConfiguration *config = nullptr;
    switch (type()) {
    case TestCase:
        config = new QtTestConfiguration;
        config->setTestCaseCount(childCount());
        config->setProjectFile(proFile());
        config->setProject(project);
        config->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(filePath(), proFile()));
        break;
    case TestFunctionOrSet: {
        TestTreeItem *parent = parentItem();
        config = new QtTestConfiguration();
        config->setTestCases(QStringList(name()));
        config->setProjectFile(parent->proFile());
        config->setProject(project);
        config->setDisplayName(
                TestUtils::getCMakeDisplayNameIfNecessary(filePath(), parent->proFile()));
        break;
    }
    case TestDataTag: {
        const TestTreeItem *function = parentItem();
        const TestTreeItem *parent = function ? function->parentItem() : nullptr;
        if (!parent)
            return nullptr;
        const QString functionWithTag = function->name() + ':' + name();
        config = new QtTestConfiguration();
        config->setTestCases(QStringList(functionWithTag));
        config->setProjectFile(parent->proFile());
        config->setProject(project);
        config->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(filePath(),
                                                                         parent->proFile()));
        break;
    }
    default:
        return nullptr;
    }
    return config;
}

TestConfiguration *QtTestTreeItem::debugConfiguration() const
{
    QtTestConfiguration *config = static_cast<QtTestConfiguration *>(testConfiguration());
    if (config)
        config->setRunMode(DebuggableTestConfiguration::Debug);
    return config;
}

QList<TestConfiguration *> QtTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);

        TestConfiguration *tc = new QtTestConfiguration();
        tc->setTestCaseCount(child->childCount());
        tc->setProjectFile(child->proFile());
        tc->setProject(project);
        tc->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(),
                                                                     child->proFile()));
        result << tc;
    }
    return result;
}

QList<TestConfiguration *> QtTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QtTestConfiguration *testConfiguration = nullptr;

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);

        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked:
            testConfiguration = new QtTestConfiguration();
            testConfiguration->setTestCaseCount(child->childCount());
            testConfiguration->setProjectFile(child->proFile());
            testConfiguration->setProject(project);
            testConfiguration->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(), child->proFile()));
            result << testConfiguration;
            continue;
        case Qt::PartiallyChecked:
        default:
            int grandChildCount = child->childCount();
            QStringList testCases;
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->childItem(grandChildRow);
                if (grandChild->checked() == Qt::Checked) {
                    testCases << grandChild->name();
                } else if (grandChild->checked() == Qt::PartiallyChecked) {
                    const int dtCount = grandChild->childCount();
                    const QString funcName = grandChild->name();
                    for (int dtRow = 0; dtRow < dtCount; ++dtRow) {
                        const TestTreeItem *dataTag = grandChild->childItem(dtRow);
                        if (dataTag->checked() == Qt::Checked)
                            testCases << funcName + ':' + dataTag->name();
                    }
                }
            }

            testConfiguration = new QtTestConfiguration();
            testConfiguration->setTestCases(testCases);
            testConfiguration->setProjectFile(child->proFile());
            testConfiguration->setProject(project);
            testConfiguration->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(), child->proFile()));
            result << testConfiguration;
        }
    }

    return result;
}

TestTreeItem *QtTestTreeItem::find(const TestParseResult *result)
{
    QTC_ASSERT(result, return nullptr);

    switch (type()) {
    case Root:
        return findChildByFile(result->fileName);
    case TestCase: {
        const QtTestParseResult *qtResult = static_cast<const QtTestParseResult *>(result);
        return findChildByNameAndInheritance(qtResult->displayName, qtResult->inherited());
    }
    case TestFunctionOrSet:
    case TestDataFunction:
    case TestSpecialFunction:
        return findChildByName(result->name);
    default:
        return nullptr;
    }
}

bool QtTestTreeItem::modify(const TestParseResult *result)
{
    QTC_ASSERT(result, return false);

    switch (type()) {
    case TestCase:
        return modifyTestCaseContent(result->name, result->line, result->column);
    case TestFunctionOrSet:
    case TestDataFunction:
    case TestSpecialFunction:
        return modifyTestFunctionContent(result);
    case TestDataTag:
        return modifyDataTagContent(result->name, result->fileName, result->line, result->column);
    default:
        return false;
    }
}

TestTreeItem *QtTestTreeItem::findChildByNameAndInheritance(const QString &name, bool inherited) const
{
    return findChildBy([name, inherited](const TestTreeItem *other) -> bool {
        const QtTestTreeItem *qtOther = static_cast<const QtTestTreeItem *>(other);
        return qtOther->inherited() == inherited && qtOther->name() == name;
    });
}

QString QtTestTreeItem::nameSuffix() const
{
    static QString inheritedSuffix = QString(" [")
                + QCoreApplication::translate("QtTestTreeItem", "inherited")
                + QString("]");
    return m_inherited ? inheritedSuffix : QString();
}

} // namespace Internal
} // namespace Autotest
