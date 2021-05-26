/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "ctesttool.h"
#include "ctesttreeitem.h"

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

ITestTreeItem *CTestTool::createRootNode()
{
    return new CTestTreeItem(this,
                             QCoreApplication::translate("CTestTool", "CTest"),
                             Utils::FilePath(), ITestTreeItem::Root);
}

} // namespace Internal
} // namespace Autotest
