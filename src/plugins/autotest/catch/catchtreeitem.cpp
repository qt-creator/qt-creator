// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "catchtreeitem.h"

#include "catchconfiguration.h"
#include "../autotesttr.h"
#include "../testtreeitem.h"
#include "../itestparser.h"
#include "../itestframework.h"

#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/qtcassert.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

QString CatchTreeItem::testCasesString() const
{
    QString testcase = m_state & CatchTreeItem::Parameterized ? QString(name() + " -*") : name();
    // mask comma if it is part of the test case name
    return testcase.replace(',', "\\,");
}

static QString nonRootDisplayName(const CatchTreeItem *it)
{
    if (it->type() != TestTreeItem::TestSuite)
        return it->name();
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project)
        return it->name();
    TestTreeItem *parent = it->parentItem();
    int baseDirSize = (parent->type() == TestTreeItem::GroupNode)
            ? parent->filePath().toString().size() : project->projectDirectory().toString().size();
    return it->name().mid(baseDirSize + 1);
}

QVariant CatchTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (type() == Root)
            break;
        return QString(nonRootDisplayName(this) + stateSuffix());
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
            const FilePath path = result->fileName.absolutePath();
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
    const FilePath absPath = filePath().absolutePath();
    return new CatchTreeItem(framework(), absPath.baseName(), absPath, TestTreeItem::GroupNode);
}

bool CatchTreeItem::canProvideTestConfiguration() const
{
    return type() == TestCase;
}

bool CatchTreeItem::canProvideDebugConfiguration() const
{
    return canProvideTestConfiguration();
}

ITestConfiguration *CatchTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    QTC_ASSERT(project, return nullptr);

    if (type() != TestCase)
        return nullptr;

    CatchConfiguration *config = nullptr;
    config = new CatchConfiguration(framework());
    config->setTestCaseCount(childCount());
    config->setProjectFile(proFile());
    config->setProject(project);
    config->setTestCases(QStringList(testCasesString()));
    config->setInternalTargets(CppEditor::CppModelManager::internalTargets(filePath()));
    return config;
}

ITestConfiguration *CatchTreeItem::debugConfiguration() const
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
                            QHash<FilePath, CatchTestCases> &testCasesForProfile,
                            bool ignoreCheckState)
{
    QTC_ASSERT(item, return);
    const int childCount = item->childCount();
    if (item->type() == TestTreeItem::GroupNode) {
        item->forFirstLevelChildItems([&testCasesForProfile, ignoreCheckState](TestTreeItem *it) {
            collectTestInfo(it, testCasesForProfile, ignoreCheckState);
        });
        return;
    }

    QTC_ASSERT(childCount != 0, return);
    QTC_ASSERT(item->type() == TestTreeItem::TestSuite, return);
    if (ignoreCheckState || item->checked() == Qt::Checked) {
        const FilePath &projectFile = item->childItem(0)->proFile();
        item->forAllChildItems([&testCasesForProfile, &projectFile](TestTreeItem *it) {
            CatchTreeItem *current = static_cast<CatchTreeItem *>(it);
            testCasesForProfile[projectFile].names.append(current->testCasesString());
        });
        testCasesForProfile[projectFile].internalTargets.unite(
            CppEditor::CppModelManager::internalTargets(item->filePath()));
    } else if (item->checked() == Qt::PartiallyChecked) {
        item->forFirstLevelChildItems([&testCasesForProfile](TestTreeItem *child) {
            QTC_ASSERT(child->type() == TestTreeItem::TestCase, return);
            if (child->checked() == Qt::Checked) {
                CatchTreeItem *current = static_cast<CatchTreeItem *>(child);
                testCasesForProfile[child->proFile()].names.append(current->testCasesString());
                testCasesForProfile[child->proFile()].internalTargets.unite(
                            CppEditor::CppModelManager::internalTargets(child->filePath()));
            }

        });
    }
}

static void collectFailedTestInfo(const CatchTreeItem *item,
                                  QHash<FilePath, CatchTestCases> &testCasesForProfile)
{
    QTC_ASSERT(item, return);
    QTC_ASSERT(item->type() == TestTreeItem::Root, return);

    item->forAllChildItems([&testCasesForProfile](TestTreeItem *it) {
        QTC_ASSERT(it, return);
        QTC_ASSERT(it->parentItem(), return);
        if (it->type() == TestTreeItem::TestCase && it->data(0, FailedRole).toBool()) {
            CatchTreeItem *current = static_cast<CatchTreeItem *>(it);
            testCasesForProfile[it->proFile()].names.append(current->testCasesString());
            testCasesForProfile[it->proFile()].internalTargets.unite(
                        CppEditor::CppModelManager::internalTargets(it->filePath()));
        }
    });
}

QList<ITestConfiguration *> CatchTreeItem::getAllTestConfigurations() const
{
    return getTestConfigurations(true);
}

QList<ITestConfiguration *> CatchTreeItem::getSelectedTestConfigurations() const
{
    return getTestConfigurations(false);
}

QList<ITestConfiguration *> CatchTreeItem::getFailedTestConfigurations() const
{
    QList<ITestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<FilePath, CatchTestCases> testCasesForProFile;
    collectFailedTestInfo(this, testCasesForProFile);

    for (auto it = testCasesForProFile.begin(), end = testCasesForProFile.end(); it != end; ++it) {
        for (const QString &target : std::as_const(it.value().internalTargets)) {
            CatchConfiguration *tc = new CatchConfiguration(framework());
            tc->setTestCases(it.value().names);
            tc->setProjectFile(it.key());
            tc->setProject(project);
            tc->setInternalTarget(target);
            result << tc;
        }
    }

    return result;
}

QList<ITestConfiguration *> CatchTreeItem::getTestConfigurationsForFile(const FilePath &fileName) const
{
    QList<ITestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project || type() != Root)
        return result;

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *item = childItem(row);
        QTC_ASSERT(item, continue);

        if (childAt(row)->filePath() != fileName)
            continue;

        CatchConfiguration *testConfig = nullptr;
        QStringList testCases;

        item->forFirstLevelChildItems([&testCases](TestTreeItem *child) {
            CatchTreeItem *current = static_cast<CatchTreeItem *>(child);
            testCases << current->testCasesString();
        });

        testConfig = new CatchConfiguration(framework());
        testConfig->setTestCases(testCases);
        testConfig->setProjectFile(item->proFile());
        testConfig->setProject(ProjectExplorer::ProjectManager::startupProject());
        testConfig->setInternalTargets(
            CppEditor::CppModelManager::internalTargets(item->filePath()));
        result << testConfig;
    }

    return result;
}

QString CatchTreeItem::stateSuffix() const
{
    QStringList types;
    if (m_state & CatchTreeItem::Parameterized)
        types.append(Tr::tr("parameterized"));
    if (m_state & CatchTreeItem::Fixture)
        types.append(Tr::tr("fixture"));
    return types.isEmpty() ? QString() : QString(" [" + types.join(", ") + ']');
}

QList<ITestConfiguration *> CatchTreeItem::getTestConfigurations(bool ignoreCheckState) const
{
    QList<ITestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<FilePath, CatchTestCases> testCasesForProfile;
    for (int row = 0, end = childCount(); row < end; ++row)
        collectTestInfo(childItem(row), testCasesForProfile, ignoreCheckState);

    for (auto it = testCasesForProfile.begin(), end = testCasesForProfile.end(); it != end; ++it) {
        for (const QString &target : std::as_const(it.value().internalTargets)) {
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
