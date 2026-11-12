// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testsettings.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "autotesttr.h"
#include "testframeworkmanager.h"
#include "testframeworkmanager.h"
#include "testsettings.h"
#include "testtreemodel.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/algorithm.h>
#include <utils/guiutils.h>
#include <utils/id.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

#include <QHeaderView>

#ifdef WITH_TESTS
#include <QLabel>
#include <QTest>
#endif

using namespace Utils;

namespace Autotest::Internal  {

static const char groupSuffix[]                 = ".group";

constexpr int defaultTimeout = 60000;

TestSettings &testSettings()
{
    static TestSettings theSettings;
    return theSettings;
}

enum TestBaseInfo
{
    BaseId = Qt::UserRole,
    BaseType
};

static FrameworksAspect::Data extractData(const QAbstractItemModel *model)
{
    QTC_ASSERT(model, return {});
    const int itemCount = TestFrameworkManager::registeredFrameworks().size();
    QTC_ASSERT(itemCount <= model->rowCount(), return {});
    FrameworksAspect::Data data;

    for (int row = 0; row < itemCount; ++row) {
        QModelIndex idx = model->index(row, 0);
        const Id id = Id::fromSetting(idx.data(BaseId));
        data.frameworks.insert(id, idx.data(Qt::CheckStateRole) == Qt::Checked);
        idx = model->index(row, 1);
        data.frameworksGrouping.insert(id, idx.data(Qt::CheckStateRole) == Qt::Checked);
    }

    int row = TestFrameworkManager::registeredFrameworks().size();
    const int end = model->rowCount();
    QTC_ASSERT(row <= end, return {});
    for ( ; row < end; ++row) {
        const QModelIndex idx = model->index(row, 0);
        const Id id = Id::fromSetting(idx.data(BaseId));
        data.tools.insert(id, idx.data(Qt::CheckStateRole) == Qt::Checked);
    }

    return data;
}

FrameworksAspect::FrameworksAspect(AspectContainer *container)
    : BaseAspect(container)
{}

bool FrameworksAspect::framework(Id id) const
{
    return m_data.frameworks.value(id, false);
}

bool FrameworksAspect::frameworkGrouping(Id id) const
{
    return m_data.frameworksGrouping.value(id, false);
}

bool FrameworksAspect::tool(Id id) const
{
    return m_data.tools.value(id, false);
}

static bool isSameData(const FrameworksAspect::Data &left, const FrameworksAspect::Data &right)
{
    for (const Id id : left.frameworksGrouping.keys()) {
        if (left.frameworksGrouping[id] != right.frameworksGrouping[id])
            return false;
        if (left.frameworks[id] != right.frameworks[id])
            return false;
    }
    for (const Id id : left.tools.keys()) {
        if (left.tools[id] != right.tools[id])
            return false;
    }
    return true;
}

void FrameworksAspect::apply()
{
    const Data tmp = m_volatileData;

    const QList<Utils::Id> changedIds = Utils::filtered(tmp.frameworksGrouping.keys(),
       [this, &tmp](Id id) {
        return tmp.frameworksGrouping[id] != m_data.frameworksGrouping[id];
    });

    m_data = tmp;

    writeSettings();

    for (ITestFramework *framework : TestFrameworkManager::registeredFrameworks()) {
        framework->setActive(tmp.frameworks.value(framework->id(), false));
        framework->setGrouping(tmp.frameworksGrouping.value(framework->id(), false));
    }

    for (ITestTool *testTool : TestFrameworkManager::registeredTestTools())
        testTool->setActive(tmp.tools.value(testTool->id(), false));

    TestTreeModel::instance()->synchronizeTestFrameworks();
    TestTreeModel::instance()->synchronizeTestTools();
    if (!changedIds.isEmpty())
        TestTreeModel::instance()->rebuild(changedIds);
}

void FrameworksAspect::cancel()
{
    setVolatileData(m_data);
}

bool FrameworksAspect::isDirty() const
{
    return !isSameData(m_volatileData, m_data);
}

void FrameworksAspect::setVolatileData(const Data &data)
{
    m_volatileData = data;
    emit volatileDataChanged(QPrivateSignal());
}

void FrameworksAspect::writeSettings() const
{
    QtcSettings &s = Utils::userSettings();
    s.beginGroup(Constants::SETTINGSGROUP);

    // store frameworks and their current active and grouping state
    for (auto it = m_data.frameworks.cbegin(); it != m_data.frameworks.cend(); ++it) {
        const Utils::Id &id = it.key();
        s.setValue(id.toKey(), it.value());
        s.setValue(id.toKey() + groupSuffix, m_data.frameworksGrouping.value(id));
    }
    // ..and the testtools as well
    for (auto it = m_data.tools.cbegin(); it != m_data.tools.cend(); ++it)
        s.setValue(it.key().toKey(), it.value());
    s.endGroup();
}

void FrameworksAspect::readSettings()
{
    QtcSettings &s = Utils::userSettings();
    s.beginGroup(Constants::SETTINGSGROUP);

    // try to get settings for registered frameworks
    const TestFrameworks &registered = TestFrameworkManager::registeredFrameworks();
    m_data.frameworks.clear();
    m_data.frameworksGrouping.clear();
    for (const ITestFramework *framework : registered) {
        // get their active state
        const Id id = framework->id();
        const Key key = id.toKey();
        m_data.frameworks.insert(id, s.value(key, framework->active()).toBool());
        // and whether grouping is enabled
        m_data.frameworksGrouping.insert(id, s.value(key + groupSuffix, framework->grouping()).toBool());
    }
    // ..and for test tools as well
    const TestTools &registeredTools = TestFrameworkManager::registeredTestTools();
    m_data.tools.clear();
    for (const ITestTool *testTool : registeredTools) {
        const Id id = testTool->id();
        m_data.tools.insert(id, s.value(id.toKey(), testTool->active()).toBool());
    }
    s.endGroup();
    setVolatileData(m_data);
}

static void populateTreeWidget(QTreeWidget *tree, const FrameworksAspect::Data &data)
{
    DirtySettingsGuard guard; // Prevent triggering dirty checks while populating
    const QSignalBlocker blocker(tree);

    const TestFrameworks &registered = TestFrameworkManager::registeredFrameworks();
    tree->clear();
    for (const ITestFramework *framework : registered) {
        const Id id = framework->id();
        auto item = new QTreeWidgetItem(tree, {framework->displayName()});
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState(0, data.frameworks.value(id) ? Qt::Checked : Qt::Unchecked);
        item->setData(0, BaseId, id.toSetting());
        item->setData(0, BaseType, ITestBase::Framework);
        item->setData(1, Qt::CheckStateRole,
                      data.frameworksGrouping.value(id) ? Qt::Checked : Qt::Unchecked);
        item->setToolTip(0, Tr::tr("Enable or disable test frameworks to be handled by the "
                                   "AutoTest plugin."));
        QString toolTip = framework->groupingToolTip();
        if (toolTip.isEmpty())
            toolTip = Tr::tr("Enable or disable grouping of test cases by folder.");
        item->setToolTip(1, toolTip);
    }
    // ...and now the test tools
    const TestTools &registeredTools = TestFrameworkManager::registeredTestTools();
    for (const ITestTool *testTool : registeredTools) {
        const Id id = testTool->id();
        auto item = new QTreeWidgetItem(tree, {testTool->displayName()});
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState(0, data.tools.value(id) ? Qt::Checked : Qt::Unchecked);
        item->setData(0, BaseId, id.toSetting());
        item->setData(0, BaseType, ITestBase::Tool);
    }
}

// Returns whether there is something to warn about.
static bool updateWarning(InfoLabel *label, const FrameworksAspect::Data &data)
{
    const bool frameworkEnabled = Utils::anyOf(data.frameworks, [](bool on) { return on; });
    const bool toolEnabled = Utils::anyOf(data.tools, [](bool on) { return on; });
    if (!frameworkEnabled && !toolEnabled) {
        label->setText(Tr::tr("No active test frameworks or tools."));
        label->setToolTip(Tr::tr("You will not be able to use the AutoTest plugin "
                                 "without having at least one active test framework."));
        return true;
    }
    if (frameworkEnabled && toolEnabled) {
        label->setText(Tr::tr("Mixing test frameworks and test tools."));
        label->setToolTip(Tr::tr("Mixing test frameworks and test tools can lead "
                                 "to duplicating run information when using "
                                 "\"Run All Tests\", for example."));
        return true;
    }
    return false;
}

void FrameworksAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto tree = createSubWidget<QTreeWidget>();
    tree->setRootIsDecorated(false);
    tree->setHeaderHidden(false);
    tree->setColumnCount(2);
    tree->header()->setDefaultSectionSize(150);
    tree->setToolTip(Tr::tr("Selects the test frameworks to be handled by the AutoTest plugin."));

    QTreeWidgetItem *item = tree->headerItem();
    item->setText(0, Tr::tr("Framework"));
    item->setToolTip(0, Tr::tr("Selects the test frameworks to be handled by the AutoTest plugin."));
    item->setText(1, Tr::tr("Group"));
    item->setToolTip(1, Tr::tr("Enables grouping of test cases."));

    populateTreeWidget(tree, m_volatileData);

    auto warning = new InfoLabel;
    warning->setElideMode(Qt::ElideNone);
    warning->setType(InfoLabelType::Warning);
    if (!updateWarning(warning, m_volatileData))
        warning->setVisible(false);

    connect(tree, &QTreeWidget::itemChanged, this, [this, tree] {
        setVolatileData(Internal::extractData(tree->model()));
        checkSettingsDirty();
    });
    connect(this, &FrameworksAspect::volatileDataChanged, tree, [this, tree, warning] {
        // The tree that was edited already shows the change; do not reset it under the edit.
        if (!isSameData(Internal::extractData(tree->model()), m_volatileData))
            populateTreeWidget(tree, m_volatileData);
        warning->setVisible(updateWarning(warning, m_volatileData));
    });

    parent.addItem(tree);
    parent.addItem(warning);
}

TestSettings::TestSettings()
{
    setAutoApply(false);

    setSettingsGroup(Constants::SETTINGSGROUP);

    scanThreadLimit.setSettingsKey("ScanThreadLimit");
    scanThreadLimit.setDefaultValue(0);
    scanThreadLimit.setRange(0, QThread::idealThreadCount());
    scanThreadLimit.setSpecialValueText("Automatic");
    scanThreadLimit.setToolTip(Tr::tr("Number of worker threads used when scanning for tests."));

    useTimeout.setSettingsKey("UseTimeout");
    useTimeout.setDefaultValue(false);
    useTimeout.setLabelText(Tr::tr("Timeout:"));
    useTimeout.setToolTip(Tr::tr("Use a timeout while executing test cases."));

    timeout.setSettingsKey("Timeout");
    timeout.setDefaultValue(defaultTimeout);
    timeout.setRange(5000, 36'000'000); // 36 Mio ms = 36'000 s = 10 h
    timeout.setSuffix(Tr::tr(" s")); // we show seconds, but store milliseconds
    timeout.setDisplayScaleFactor(1000);
    timeout.setToolTip(Tr::tr("Timeout used when executing test cases. This will apply "
                              "for each test case on its own, not the whole project. "
                              "Overrides test framework or build system defaults."));

    omitInternalMsg.setSettingsKey("OmitInternal");
    omitInternalMsg.setDefaultValue(true);
    omitInternalMsg.setLabelText(Tr::tr("Omit internal messages"));
    omitInternalMsg.setToolTip(Tr::tr("Hides internal messages by default. "
        "You can still enable them by using the test results filter."));

    omitRunConfigWarn.setSettingsKey("OmitRCWarnings");
    omitRunConfigWarn.setLabelText(Tr::tr("Omit run configuration warnings"));
    omitRunConfigWarn.setToolTip(Tr::tr("Hides warnings related to a deduced run configuration."));

    limitResultOutput.setSettingsKey("LimitResultOutput");
    limitResultOutput.setDefaultValue(true);
    limitResultOutput.setLabelText(Tr::tr("Limit result output"));
    limitResultOutput.setToolTip(Tr::tr("Limits result output to 100000 characters."));

    limitResultDescription.setSettingsKey("LimitResultDescription");
    limitResultDescription.setLabelText(Tr::tr("Limit result description:"));
    limitResultDescription.setToolTip(
        Tr::tr("Limit number of lines shown in test result tooltip and description."));

    resultDescriptionMaxSize.setSettingsKey("ResultDescriptionMaxSize");
    resultDescriptionMaxSize.setDefaultValue(10);
    resultDescriptionMaxSize.setRange(1, 100000);

    autoScroll.setSettingsKey("AutoScrollResults");
    autoScroll.setDefaultValue(true);
    autoScroll.setLabelText(Tr::tr("Automatically scroll results"));
    autoScroll.setToolTip(Tr::tr("Automatically scrolls down when new items are added "
                                 "and scrollbar is at bottom."));

    processArgs.setSettingsKey("ProcessArgs");
    processArgs.setLabelText(Tr::tr("Process arguments"));
    processArgs.setToolTip(
        Tr::tr("Allow passing arguments specified on the respective run configuration.\n"
               "Warning: this is an experimental feature and might lead to failing to "
               "execute the test executable."));

    displayApplication.setSettingsKey("DisplayApp");
    displayApplication.setLabelText(Tr::tr("Group results by application"));

    popupOnStart.setSettingsKey("PopupOnStart");
    popupOnStart.setLabelText(Tr::tr("Open results when tests start"));
    popupOnStart.setToolTip(
        Tr::tr("Displays test results automatically when tests are started."));

    popupOnFinish.setSettingsKey("PopupOnFinish");
    popupOnFinish.setDefaultValue(true);
    popupOnFinish.setLabelText(Tr::tr("Open results when tests finish"));
    popupOnFinish.setToolTip(
        Tr::tr("Displays test results automatically when tests are finished."));

    popupOnFail.setSettingsKey("PopupOnFail");
    popupOnFail.setLabelText(Tr::tr("Only for unsuccessful test runs"));
    popupOnFail.setToolTip(Tr::tr("Displays test results only if the test run contains "
                                  "failed, fatal or unexpectedly passed tests."));

    // UI not in settings page, but inside the filter menu of the navigation widget
    showTreeFilterTextInput.setSettingsKey("ShowTreeFilter");
    showTreeFilterTextInput.setDefaultValue(true);

    runAfterBuild.setSettingsKey("RunAfterBuild");
    runAfterBuild.setDisplayStyle(Utils::SelectionAspect::DisplayStyle::ComboBox);
    runAfterBuild.setToolTip(Tr::tr("Runs chosen tests automatically if a build succeeded."));
    runAfterBuild.addOption(Tr::tr("No Tests"));
    runAfterBuild.addOption(Tr::tr("All", "Run tests after build"));
    runAfterBuild.addOption(Tr::tr("Selected"));

    setLayouter([this] {
        auto scanThreadLabel = new QLabel(Tr::tr("Scan threads:"));
        scanThreadLabel->setToolTip("Number of worker threads used when scanning for tests.");

        using namespace Layouting;

        PushButton resetChoicesButton {
            text(Tr::tr("Reset Cached Choices")),
            Layouting::toolTip(Tr::tr("Clear all cached choices of run configurations for "
                           "tests where the executable could not be deduced.")),
            onClicked(this, &clearChoiceCache)
        };

        Group generalGroup {
            title(Tr::tr("General")),
            Column {
                Row { scanThreadLabel, scanThreadLimit, st },
                omitInternalMsg,
                omitRunConfigWarn,
                limitResultOutput,
                Row { limitResultDescription, resultDescriptionMaxSize, st },
                popupOnStart,
                popupOnFinish,
                Row { Space(20), popupOnFail },
                autoScroll,
                displayApplication,
                processArgs,
                Row { Tr::tr("Automatically run"), runAfterBuild, st },
                Row { useTimeout, timeout, st },
                Row { resetChoicesButton, st }
             }
        };

        Group activeFrameworks {
            title(Tr::tr("Active Test Frameworks")),
            Column {
                frameworks
            }
        };

        return Column {
            Row {
                Column { generalGroup, st },
                Column { activeFrameworks, st }
            },
            st
        };
    });

    readSettings();

    timeout.setEnabler(&useTimeout);
    resultDescriptionMaxSize.setEnabler(&limitResultDescription);
    popupOnFail.setEnabler(&popupOnFinish);
}

RunAfterBuildMode TestSettings::runAfterBuildMode() const
{
    return static_cast<RunAfterBuildMode>(runAfterBuild());
}

// TestSettingsPage

class TestSettingsPage final : public Core::IOptionsPage
{
public:
    TestSettingsPage()
    {
        setId(Constants::AUTOTEST_SETTINGS_ID);
        setDisplayName(Tr::tr("General"));
        setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &testSettings(); });
    }
};

void setupTestSettings()
{
    static TestSettingsPage theTestSettingsPage;
}

#ifdef WITH_TESTS
class FrameworksAspectTest final : public QObject
{
    Q_OBJECT

private slots:
    void testUpdatesEveryTree()
    {
        FrameworksAspect frameworks(nullptr);
        BaseAspect &aspect = frameworks;
        aspect.readSettings();
        const std::unique_ptr<QWidget> first(Layouting::Column{aspect}.emerge());
        const std::unique_ptr<QWidget> second(Layouting::Column{aspect}.emerge());
        QTreeWidget *firstTree = first->findChild<QTreeWidget *>();
        QTreeWidget *secondTree = second->findChild<QTreeWidget *>();
        QVERIFY(firstTree);
        QVERIFY(secondTree);
        QVERIFY(firstTree->topLevelItemCount() > 0);
        const Qt::CheckState before = firstTree->topLevelItem(0)->checkState(0);
        const Qt::CheckState toggled = before == Qt::Checked ? Qt::Unchecked : Qt::Checked;

        firstTree->topLevelItem(0)->setCheckState(0, toggled);
        QCOMPARE(secondTree->topLevelItem(0)->checkState(0), toggled);
        QVERIFY(aspect.isDirty());

        aspect.cancel();
        QVERIFY(!aspect.isDirty());
        QCOMPARE(firstTree->topLevelItem(0)->checkState(0), before);
        QCOMPARE(secondTree->topLevelItem(0)->checkState(0), before);

        const Qt::CheckState groupBefore = firstTree->topLevelItem(0)->checkState(1);
        const Qt::CheckState groupToggled = groupBefore == Qt::Checked ? Qt::Unchecked
                                                                       : Qt::Checked;
        firstTree->topLevelItem(0)->setCheckState(1, groupToggled);
        QCOMPARE(secondTree->topLevelItem(0)->checkState(1), groupToggled);

        for (int i = 0; i < firstTree->topLevelItemCount(); ++i)
            firstTree->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
        // InfoLabel has no Q_OBJECT.
        const QLabel *secondWarning = Utils::findOrDefault(
            second->findChildren<QLabel *>(), [](const QLabel *label) {
                return label->text() == Tr::tr("No active test frameworks or tools.");
            });
        QVERIFY(secondWarning);
        QVERIFY(secondWarning->isVisibleTo(second.get()));

        aspect.cancel();
        QCOMPARE(secondTree->topLevelItem(0)->checkState(1), groupBefore);
    }
};

QObject *createFrameworksAspectTest()
{
    return new FrameworksAspectTest;
}
#endif // WITH_TESTS

} // Autotest::Internal

#ifdef WITH_TESTS
#include <testsettings.moc>
#endif
