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

#include "testtreeitem.h"
#include "itestparser.h"

namespace Autotest {

class ITestSettings;

class ITestBase
{
public:
    explicit ITestBase(bool activeByDefault);
    virtual ~ITestBase() = default;

    virtual const char *name() const = 0;
    virtual unsigned priority() const = 0;          // should this be modifyable?

    virtual ITestSettings *testSettings() { return nullptr; }

    TestTreeItem *rootNode();

    Utils::Id settingsId() const;
    Utils::Id id() const;

    bool active() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    void resetRootNode();

protected:
    virtual TestTreeItem *createRootNode() = 0;

private:
    TestTreeItem *m_rootNode = nullptr;
    bool m_active = false;
};

class ITestFramework : public ITestBase
{
public:
    explicit ITestFramework(bool activeByDefault);
    ~ITestFramework() override;

    ITestParser *testParser();

    bool grouping() const { return m_grouping; }
    void setGrouping(bool group) { m_grouping = group; }
    // framework specific tool tip to be displayed on the general settings page
    virtual QString groupingToolTip() const { return QString(); }

protected:
    virtual ITestParser *createTestParser() = 0;

private:
    ITestParser *m_testParser = nullptr;
    bool m_grouping = false;
};

using TestFrameworks = QList<ITestFramework *>;

} // namespace Autotest
