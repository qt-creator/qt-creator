/****************************************************************************
**
** Copyright (C) 2019 Jochen Seemann
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

#include "catchtreeitem.h"
#include "catchtestparser.h"
#include "catchconfiguration.h"
#include "catchframework.h"

#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

QString CatchTreeItem::testCasesString() const
{
    return m_state & CatchTreeItem::Parameterized ? QString(name() + " -*") : name();
}

QVariant CatchTreeItem::data(int column, int role) const
{

    switch (role) {
    case Qt::DisplayRole:
        if (type() == Root)
            break;
        return QString(name() + stateSuffix());
    case Qt::CheckStateRole:
        switch (type()) {
        case Root:
        case GroupNode:
        case TestSuite:
        case TestCase:
            return checked();
        default:
            return QVariant();
        }
    }
    return TestTreeItem::data(column, role);
}

TestTreeItem *CatchTreeItem::copyWithoutChildren()
{
    CatchTreeItem *copied = new CatchTreeItem(framework());
    copied->copyBasicDataFrom(this);
    return copied;
}

TestTreeItem *CatchTreeItem::find(const TestParseResult *result)
{
    QTC_ASSERT(result, return nullptr);

    switch (type()) {
    case Root:
        if (result->framework->grouping()) {
            const QString path = QFileInfo(result->fileName).absolutePath();
            for (int row = 0; row < childCount(); ++row) {
                TestTreeItem *group = childAt(row);
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
    case TestSuite:
        return findChildByNameAndFile(result->name, result->fileName);
    default:
        return nullptr;
    }
}

TestTreeItem *CatchTreeItem::findChild(const TestTreeItem *other)
{
    QTC_ASSERT(other, return nullptr);

    switch (type()) {
    case Root:
        return findChildByFileAndType(other->filePath(), other->type());
    case GroupNode:
        return other->type() == TestSuite ? findChildByFile(other->filePath()) : nullptr;
    case TestSuite:
        return findChildByNameAndFile(other->name(), other->filePath());
    default:
        return nullptr;
    }
}

bool CatchTreeItem::modify(const TestParseResult *result)
{
    QTC_ASSERT(result, return false);

    switch (type()) {
    case TestSuite:
    case TestCase:
        return modifyTestFunctionContent(result);
    default:
        return false;
    }
}

TestTreeItem *CatchTreeItem::createParentGroupNode() const
{
    const QFileInfo fileInfo(filePath());
    const QFileInfo base(fileInfo.absolutePath());
    return new CatchTreeItem(framework(), base.baseName(), fileInfo.absolutePath(), TestTreeItem::GroupNode);
}

bool CatchTreeItem::canProvideTestConfiguration() const
{
    return type() == TestCase;
}

bool CatchTreeItem::canProvideDebugConfiguration() const
{
    return canProvideTestConfiguration();
}

TestConfiguration *CatchTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return nullptr);

    if (type() != TestCase)
        return nullptr;

    CatchConfiguration *config = nullptr;
    config = new CatchConfiguration(framework());
    config->setTestCaseCount(childCount());
    config->setProjectFile(proFile());
    config->setProject(project);
    config->setTestCases(QStringList(testCasesString()));
    config->setInternalTargets(internalTargets());
    return config;
}

TestConfiguration *CatchTreeItem::debugConfiguration() const
{
    CatchConfiguration *config = static_cast<CatchConfiguration *>(testConfiguration());
    if (config)
        config->setRunMode(TestRunMode::Debug);
    return config;
}

struct CatchTestCases
{
    QStringList names;
    QSet<QString> internalTargets;
};

static void collectTestInfo(const TestTreeItem *item,
                            QHash<QString, CatchTestCases> &testCasesForProfile,
                            bool ignoreCheckState)
{
    QTC_ASSERT(item, return);
    const int childCount = item->childCount();
    if (item->type() == TestTreeItem::GroupNode) {
        item->forFirstLevelChildren([&testCasesForProfile, ignoreCheckState](TestTreeItem *it) {
            collectTestInfo(it, testCasesForProfile, ignoreCheckState);
        });
        return;
    }

    QTC_ASSERT(childCount != 0, return);
    QTC_ASSERT(item->type() == TestTreeItem::TestSuite, return);
    if (ignoreCheckState || item->checked() == Qt::Checked) {
        const QString &projectFile = item->childAt(0)->proFile();
        item->forAllChildren([&testCasesForProfile, &projectFile](TestTreeItem *it) {
            CatchTreeItem *current = static_cast<CatchTreeItem *>(it);
            testCasesForProfile[projectFile].names.append(current->testCasesString());
        });
        testCasesForProfile[projectFile].internalTargets.unite(item->internalTargets());
    } else if (item->checked() == Qt::PartiallyChecked) {
        item->forFirstLevelChildren([&testCasesForProfile](TestTreeItem *child) {
            QTC_ASSERT(child->type() == TestTreeItem::TestCase, return);
            if (child->checked() == Qt::Checked) {
                CatchTreeItem *current = static_cast<CatchTreeItem *>(child);
                testCasesForProfile[child->proFile()].names.append(current->testCasesString());
                testCasesForProfile[child->proFile()].internalTargets.unite(
                            child->internalTargets());
            }

        });
    }
}

QList<TestConfiguration *> CatchTreeItem::getAllTestConfigurations() const
{
    return getTestConfigurations(true);
}

QList<TestConfiguration *> CatchTreeItem::getSelectedTestConfigurations() const
{
    return getTestConfigurations(false);
}

QList<TestConfiguration *> CatchTreeItem::getTestConfigurationsForFile(const Utils::FilePath &fileName) const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    const QString filePath = fileName.toString();
    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *item = childAt(row);
        QTC_ASSERT(item, continue);

        if (childAt(row)->name() != filePath)
            continue;

        CatchConfiguration *testConfig = nullptr;
        QStringList testCases;

        item->forFirstLevelChildren([&testCases](TestTreeItem *child) {
            CatchTreeItem *current = static_cast<CatchTreeItem *>(child);
            testCases << current->testCasesString();
        });

        testConfig = new CatchConfiguration(framework());
        testConfig->setTestCases(testCases);
        testConfig->setProjectFile(item->proFile());
        testConfig->setProject(ProjectExplorer::SessionManager::startupProject());
        testConfig->setInternalTargets(item->internalTargets());
        result << testConfig;
    }

    return result;
}

QString CatchTreeItem::stateSuffix() const
{
    QStringList types;
    if (m_state & CatchTreeItem::Parameterized)
        types.append(QCoreApplication::translate("CatchTreeItem", "parameterized"));
    if (m_state & CatchTreeItem::Fixture)
        types.append(QCoreApplication::translate("CatchTreeItem", "fixture"));
    return types.isEmpty() ? QString() : QString(" [" + types.join(", ") + ']');
}

QList<TestConfiguration *> CatchTreeItem::getTestConfigurations(bool ignoreCheckState) const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<QString, CatchTestCases> testCasesForProfile;
    forFirstLevelChildren([&testCasesForProfile, ignoreCheckState](TestTreeItem *item) {
        collectTestInfo(item, testCasesForProfile, ignoreCheckState);
    });

    for (auto it = testCasesForProfile.begin(), end = testCasesForProfile.end(); it != end; ++it) {
        for (const QString &target : qAsConst(it.value().internalTargets)) {
            CatchConfiguration *tc = new CatchConfiguration(framework());
            tc->setTestCases(it.value().names);
            if (ignoreCheckState)
                tc->setTestCaseCount(0);
            tc->setProjectFile(it.key());
            tc->setProject(project);
            tc->setInternalTarget(target);
            result << tc;
        }
    }
    return result;
}

} // namespace Internal
} // namespace Autotest
