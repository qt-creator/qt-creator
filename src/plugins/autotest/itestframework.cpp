// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itestframework.h"

#include "autotestconstants.h"
#include "itestparser.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

using namespace Utils;

namespace Autotest {

ITestBase::ITestBase()
{}

void ITestBase::resetRootNode()
{
    if (!m_rootNode)
        return;
    if (m_rootNode->model())
        static_cast<TestTreeModel *>(m_rootNode->model())->takeItem(m_rootNode);
    delete m_rootNode;
    m_rootNode = nullptr;
}

ITestFramework::ITestFramework()
{
    setType(ITestBase::Framework);
    setAutoApply(false);
}

ITestFramework::~ITestFramework()
{
    delete m_testParser;
}

TestTreeItem *ITestFramework::rootNode()
{
    if (!m_rootNode)
        m_rootNode = createRootNode();
    // These are stored in the TestTreeModel and destroyed on shutdown there.
    return static_cast<TestTreeItem *>(m_rootNode);
}

ITestParser *ITestFramework::testParser()
{
    if (!m_testParser)
        m_testParser = createTestParser();
    return m_testParser;
}

QStringList ITestFramework::testNameForSymbolName(const QString &) const
{
    return {};
}

ITestTool::ITestTool()
{
    setType(ITestBase::Tool);
    setPriority(255);
}

ITestTreeItem *ITestTool::rootNode()
{
    if (!m_rootNode)
        m_rootNode = createRootNode();
    // These are stored in the TestTreeModel and destroyed on shutdown there.
    return m_rootNode;
}

} // namespace Autotest
