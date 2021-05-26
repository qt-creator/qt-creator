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
