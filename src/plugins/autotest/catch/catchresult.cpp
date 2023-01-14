// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "catchresult.h"
#include "catchtreeitem.h"

#include "../testframeworkmanager.h"

#include <utils/id.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

static ResultHooks::FindTestItemHook findTestItemHook()
{
    return [=](const TestResult &result) -> ITestTreeItem * {
        const Id id = Id(Constants::FRAMEWORK_PREFIX).withSuffix("Catch");
        ITestFramework *framework = TestFrameworkManager::frameworkForId(id);
        QTC_ASSERT(framework, return nullptr);
        const TestTreeItem *rootNode = framework->rootNode();
        if (!rootNode)
            return nullptr;

        return rootNode->findAnyChild([&](const TreeItem *item) {
            const auto treeItem = static_cast<const CatchTreeItem *>(item);
            if (!treeItem || treeItem->filePath() != result.fileName())
                return false;
            const bool parameterized = treeItem->states() & CatchTreeItem::Parameterized;
            return parameterized ? result.name().startsWith(treeItem->name() + " - ")
                                 : result.name() == treeItem->name();
        });
    };
}

struct CatchData
{
    int m_sectionDepth = 0;
};

static ResultHooks::DirectParentHook directParentHook(int depth)
{
    return [=](const TestResult &result, const TestResult &other, bool *) -> bool {
        if (!other.extraData().canConvert<CatchData>())
            return false;
        const CatchData otherData = other.extraData().value<CatchData>();

        if (result.result() != ResultType::TestStart)
            return false;

        if (other.result() == ResultType::TestStart) {
            if (result.fileName() != other.fileName())
                return false;

            return depth + 1 == otherData.m_sectionDepth;
        }

        if (depth <= otherData.m_sectionDepth && other.result() == ResultType::Pass)
            return true;

        if (result.fileName() != other.fileName() || depth > otherData.m_sectionDepth)
            return false;

        return result.name() == other.name();
    };
}

CatchResult::CatchResult(const QString &id, const QString &name, int depth)
    : TestResult(id, name, {QVariant::fromValue(CatchData{depth}),
                            {}, findTestItemHook(), directParentHook(depth)})
{}

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::CatchData);
