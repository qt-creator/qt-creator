// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctesttool.h"
#include "ctesttreeitem.h"
#include "../autotesttr.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <projectexplorer/buildsystem.h>

namespace Autotest {
namespace Internal {

Utils::Id CTestTool::buildSystemId() const
{
    return Utils::Id(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
}

ITestTreeItem *CTestTool::createItemFromTestCaseInfo(const ProjectExplorer::TestCaseInfo &tci)
{
    CTestTreeItem *item = new CTestTreeItem(this, tci.name, tci.path, TestTreeItem::TestCase);
    item->setLine(tci.line);
    return item;
}

const char *CTestTool::name() const
{
    return "CTest";
}

QString CTestTool::displayName() const
{
    return Tr::tr("CTest");
}

ITestTreeItem *CTestTool::createRootNode()
{
    return new CTestTreeItem(this,
                             displayName(),
                             Utils::FilePath(), ITestTreeItem::Root);
}

} // namespace Internal
} // namespace Autotest
