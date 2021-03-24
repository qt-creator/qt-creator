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

#pragma once

#include <utils/id.h>

namespace ProjectExplorer { struct TestCaseInfo; }
namespace Utils { class AspectContainer; }

namespace Autotest {

class ITestFramework;
class ITestParser;
using ITestSettings = Utils::AspectContainer;
class ITestTool;
class ITestTreeItem;
class TestTreeItem;

class ITestBase
{
public:
    enum TestBaseType
    {
        None      = 0x0,
        Framework = 0x1,
        Tool      = 0x2
    };

    explicit ITestBase(bool activeByDefault, const TestBaseType type);
    virtual ~ITestBase() = default;

    virtual const char *name() const = 0;
    virtual unsigned priority() const = 0;          // should this be modifyable?
    TestBaseType type() const { return m_type; }

    virtual ITestSettings *testSettings() { return nullptr; }

    Utils::Id settingsId() const;
    Utils::Id id() const;

    bool active() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    void resetRootNode();

    virtual ITestFramework *asFramework() { return nullptr; }
    virtual ITestTool *asTestTool() { return nullptr; }

protected:
    virtual ITestTreeItem *createRootNode() = 0;

private:
    ITestTreeItem *m_rootNode = nullptr;
    bool m_active = false;
    TestBaseType m_type = None;

    friend class ITestFramework;
    friend class ITestTool;
};

class ITestFramework : public ITestBase
{
public:
    explicit ITestFramework(bool activeByDefault);
    ~ITestFramework() override;

    TestTreeItem *rootNode();
    ITestParser *testParser();

    bool grouping() const { return m_grouping; }
    void setGrouping(bool group) { m_grouping = group; }
    // framework specific tool tip to be displayed on the general settings page
    virtual QString groupingToolTip() const { return QString(); }

    ITestFramework *asFramework() final { return this; }

protected:
    virtual ITestParser *createTestParser() = 0;

private:
    ITestParser *m_testParser = nullptr;
    bool m_grouping = false;
};

using TestFrameworks = QList<ITestFramework *>;

class ITestTool : public ITestBase
{
public:
    explicit ITestTool(bool activeByDefault);

    ITestTreeItem *rootNode();

    virtual Utils::Id buildSystemId() const = 0;

    virtual ITestTreeItem *createItemFromTestCaseInfo(const ProjectExplorer::TestCaseInfo &tci) = 0;

    ITestTool *asTestTool() final { return this; }

private:
    unsigned priority() const final { return 255; }
};

using TestTools = QList<ITestTool *>;

} // namespace Autotest
