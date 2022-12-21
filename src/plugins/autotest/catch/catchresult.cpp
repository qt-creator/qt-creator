// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "catchresult.h"
#include "catchtreeitem.h"

#include "../testframeworkmanager.h"

#include <utils/id.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

CatchResult::CatchResult(const QString &id, const QString &name)
    : TestResult(id, name)
{
}

bool CatchResult::isDirectParentOf(const TestResult *other, bool *needsIntermediate) const
{
    if (!TestResult::isDirectParentOf(other, needsIntermediate))
        return false;
    const CatchResult *catchOther = static_cast<const CatchResult *>(other);

    if (result() != ResultType::TestStart)
        return false;

    if (catchOther->result() == ResultType::TestStart) {
        if (fileName() != catchOther->fileName())
            return false;

        return sectionDepth() + 1 == catchOther->sectionDepth();
    }

    if (sectionDepth() <= catchOther->sectionDepth() && catchOther->result() == ResultType::Pass)
        return true;

    if (fileName() != catchOther->fileName() || sectionDepth() > catchOther->sectionDepth())
        return false;

    return name() == catchOther->name();
}

const ITestTreeItem *CatchResult::findTestTreeItem() const
{
    auto id = Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix("Catch");
    ITestFramework *framework = TestFrameworkManager::frameworkForId(id);
    QTC_ASSERT(framework, return nullptr);
    const TestTreeItem *rootNode = framework->rootNode();
    if (!rootNode)
        return nullptr;

    const QString tcName = name();
    const Utils::FilePath tcFilePath = fileName();
    return rootNode->findAnyChild([&tcName, &tcFilePath](const Utils::TreeItem *item) {
        const auto treeItem = static_cast<const CatchTreeItem *>(item);
        if (!treeItem || treeItem->filePath() != tcFilePath)
            return false;
        const bool parameterized = treeItem->states() & CatchTreeItem::Parameterized;
        return parameterized ? tcName.startsWith(treeItem->name() + " - ")
                             : tcName == treeItem->name();
    });
}

} // namespace Internal
} // namespace Autotest
