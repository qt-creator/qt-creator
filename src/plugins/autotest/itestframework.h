// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>
#include <utils/id.h>

namespace ProjectExplorer { struct TestCaseInfo; }
namespace Utils { class AspectContainer; }

namespace Autotest {

class ITestFramework;
class ITestParser;
class ITestTool;
class ITestTreeItem;
class TestTreeItem;

class ITestBase : public Utils::AspectContainer
{
public:
    enum TestBaseType
    {
        None      = 0x0,
        Framework = 0x1,
        Tool      = 0x2
    };

    ITestBase();
    virtual ~ITestBase() = default;

    QString displayName() const { return m_displayName; }
    TestBaseType type() const { return m_type; }
    Utils::Id id() const { return m_id; }
    int priority() const { return m_priority; }

    bool active() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    void resetRootNode();

    virtual ITestFramework *asFramework() { return nullptr; }
    virtual ITestTool *asTestTool() { return nullptr; }

protected:
    void setPriority(int priority) { m_priority = priority; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setType(const TestBaseType type) { m_type = type; }
    void setId(const Utils::Id id) { m_id = id; }

    virtual ITestTreeItem *createRootNode() = 0;

private:
    ITestTreeItem *m_rootNode = nullptr;
    bool m_active = false;
    TestBaseType m_type = None;
    int m_priority = 0;
    QString m_displayName;
    Utils::Id m_id;

    friend class ITestFramework;
    friend class ITestTool;
};

class ITestFramework : public ITestBase
{
public:
    ITestFramework();
    ~ITestFramework() override;

    TestTreeItem *rootNode();
    ITestParser *testParser();

    bool grouping() const { return m_grouping; }
    void setGrouping(bool group) { m_grouping = group; }
    // framework specific tool tip to be displayed on the general settings page
    virtual QString groupingToolTip() const { return {}; }

    ITestFramework *asFramework() final { return this; }
    // helper for matching a function symbol's name to a test of this framework
    virtual QStringList testNameForSymbolName(const QString &symbolName) const;

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
    ITestTool();

    ITestTreeItem *rootNode();

    virtual Utils::Id buildSystemId() const = 0;

    virtual ITestTreeItem *createItemFromTestCaseInfo(const ProjectExplorer::TestCaseInfo &tci) = 0;

    ITestTool *asTestTool() final { return this; }
};

using TestTools = QList<ITestTool *>;

} // namespace Autotest
