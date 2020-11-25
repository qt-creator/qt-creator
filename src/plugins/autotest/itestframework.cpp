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

#include "itestframework.h"

#include "autotestconstants.h"
#include "itestparser.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

namespace Autotest {

ITestBase::ITestBase(bool activeByDefault, const ITestBase::TestBaseType type)
    : m_active(activeByDefault)
    , m_type(type)
{}

Utils::Id ITestBase::settingsId() const
{
    return Utils::Id(Constants::SETTINGSPAGE_PREFIX)
            .withSuffix(QString("%1.%2").arg(priority()).arg(QLatin1String(name())));
}

Utils::Id ITestBase::id() const
{
    return Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix(name());
}

void ITestBase::resetRootNode()
{
    if (!m_rootNode)
        return;
    if (m_rootNode->model())
        static_cast<TestTreeModel *>(m_rootNode->model())->takeItem(m_rootNode);
    delete m_rootNode;
    m_rootNode = nullptr;
}


ITestFramework::ITestFramework(bool activeByDefault)
    : ITestBase(activeByDefault, ITestBase::Framework)
{}

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

ITestTool::ITestTool(bool activeByDefault)
    : ITestBase(activeByDefault, ITestBase::Tool)
{}

ITestTreeItem *ITestTool::rootNode()
{
    if (!m_rootNode)
        m_rootNode = createRootNode();
    // These are stored in the TestTreeModel and destroyed on shutdown there.
    return m_rootNode;
}

} // namespace Autotest
