// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "registries_test.h"

#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "dialogs/ioptionspage.h"
#include "find/ifindfilter.h"
#include "locator/ilocatorfilter.h"
#include "locator/locator.h"
#include "locator/locatorwidget.h"

#include <utils/algorithm.h>

#include <QtTaskTree/qbarriertask.h>

#include <QTest>

#include <memory>

using namespace QtTaskTree;
using namespace Utils;

namespace Core::Internal {

const char findFilterId[] = "Core.Tests.FindFilter";
const char locatorFilterId[] = "Core.Tests.LocatorFilter";
const char blockingLocatorFilterId[] = "Core.Tests.BlockingLocatorFilter";

class TestFindFilter final : public IFindFilter
{
public:
    QString id() const override { return findFilterId; }
    QString displayName() const override { return "Test Find Filter"; }
    bool isEnabled() const override { return true; }
    void findAll(const QString &, FindFlags) override {}
};

class TestLocatorFilter final : public ILocatorFilter
{
public:
    TestLocatorFilter() { setId(locatorFilterId); }

private:
    LocatorMatcherTasks matchers() final { return {}; }
};

class BlockingLocatorFilter final : public ILocatorFilter
{
public:
    BlockingLocatorFilter()
    {
        setId(blockingLocatorFilterId);
        setDefaultShortcutString("coretestblock");
    }

    std::weak_ptr<int> matcherToken() const { return m_matcherToken; }

private:
    LocatorMatcherTasks matchers() final
    {
        // Lives as long as the tasks: the barrier is never advanced.
        const auto token = std::make_shared<int>();
        m_matcherToken = token;
        return {QBarrierTask([token](QBarrier &) {})};
    }

    std::weak_ptr<int> m_matcherToken;
};

class TestOptionsPage final : public IOptionsPage
{
public:
    TestOptionsPage()
    {
        setId("Core.Tests.OptionsPage");
        setCategory("Z.Core.Tests");
        setDisplayName("Test Options Page");
    }
};

class RegistriesTest final : public QObject
{
    Q_OBJECT

private slots:
    void testFindFilterAppearsAndDisappears()
    {
        const Id actionId = Id("FindFilter.").withSuffix(findFilterId);
        QVERIFY2(!ActionManager::command(actionId),
                 "the test filter's action exists before the filter does");

        {
            TestFindFilter filter;
            // The filter announces itself deferred.
            QTRY_VERIFY(ActionManager::command(actionId));
            QVERIFY(IFindFilter::allFindFilters().contains(&filter));
        }

        QVERIFY(!ActionManager::command(actionId));
    }

    void testOptionsPageActionAppearsAndDisappears()
    {
        const Id actionId("Preferences.Tests.OptionsPage");
        QVERIFY2(!ActionManager::command(actionId),
                 "the test page's action exists before the page does");

        {
            TestOptionsPage page;
            QTRY_VERIFY(ActionManager::command(actionId));
        }

        // The page's going is announced deferred as well.
        QTRY_VERIFY(!ActionManager::command(actionId));
    }

    void testLocatorFilterAppearsAndDisappears()
    {
        const int filterCount = Locator::filters().size();
        ILocatorFilter *added = nullptr;

        {
            TestLocatorFilter filter;
            added = &filter;
            QTRY_VERIFY(Locator::filters().contains(added));
        }

        // Only the pointer is compared - the filter itself is gone.
        QVERIFY(!Locator::filters().contains(added));
        QCOMPARE(Locator::filters().size(), filterCount);
    }

    void testLocatorSearchLetsGoOfFilter()
    {
        LocatorWidget widget(Locator::instance());
        std::weak_ptr<int> token;

        {
            BlockingLocatorFilter filter;
            QTRY_VERIFY(Locator::filters().contains(&filter));
            widget.showText("coretestblock text");
            token = filter.matcherToken();
            QVERIFY(!token.expired());
        }

        QVERIFY(token.expired());
    }
};

QObject *createRegistriesTest()
{
    return new RegistriesTest;
}

} // namespace Core::Internal

#include "registries_test.moc"
