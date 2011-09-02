/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "testselector.h"
#include "testcode.h"
#include "testsettings.h"
#include "qsystem.h"
#include "newtestcasedlg.h"
#include "testgenerator.h"
#include "dialogs.h"
#include "testconfigurations.h"
#include "testexecuter.h"
#include "testcontextmenu.h"

#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/imode.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>

#include <QDir>
#include <QPixmap>
#include <QStringList>
#include <QMouseEvent>
#include <QMenu>
#include <QDebug>

static const char *childAssigned_xpm[] = {
    "8 8 3 1",
    "         c #FFFFFF",
    ".        c #020202",
    "+        c #EF0928",
    "........",
    ".     +.",
    ".    ++.",
    ".   +++.",
    ".  ++++.",
    ". +++++.",
    ".++++++.",
    "........"
};

static const char *parentAssigned_xpm[] = {
    "8 8 3 1",
    "         c #FFFFFF",
    ".        c #020202",
    "+        c #EF0928",
    "........",
    ".++++++.",
    ".+++++ .",
    ".++++  .",
    ".+++   .",
    ".++    .",
    ".+     .",
    "........"
};

static const char *selected_xpm[] = {
    "8 8 3 1",
    "         c #FFFFFF",
    ".        c #020202",
    "+        c #EF0928",
    "........",
    ".++++++.",
    ".++++++.",
    ".++++++.",
    ".++++++.",
    ".++++++.",
    ".++++++.",
    "........"
};

static const char *unselected_xpm[] = {
    "8 8 3 1",
    "         c #FFFFFF",
    ".        c #020202",
    "+        c #EF0928",
    "........",
    ".      .",
    ".      .",
    ".      .",
    ".      .",
    ".      .",
    ".      .",
    "........"
};

#define NEXT_SIBLING(B,A) {\
    if (A->parent()) {\
        A = (B*)A->parent()->child(A ## Ind++);\
    } else {\
        A = (B*)topLevelItem(A ## Ind++);\
    }\
}

class TestSuiteItem : public TestViewItem
{
public:
    TestSuiteItem(const QString &basePath, QTreeWidget *parent);
    TestSuiteItem(const QString &basePath, TestSuiteItem *parent);
    virtual ~TestSuiteItem();
    virtual QString key(int column, bool ascending) const;
    virtual TestSuiteItem *addSuite(const QString &suiteName);
    virtual TestSuiteItem *findSuite(const QString &suiteName);
    virtual TestCaseItem *addCase(TestCode *code);
    virtual TestCaseItem *findCase(const QString &tcFileName);
};

class TestCaseItem : public TestViewItem
{
public:
    TestCaseItem(TestCode *code, TestSuiteItem *parent);
    virtual ~TestCaseItem();
    virtual QString key(int column, bool ascending) const;
    virtual TestFunctionItem *addFunction(const QString &funcName, bool manualTest, int startLine);
    virtual bool findFunction(const QString &funcName, TestFunctionItem *&item);
    virtual bool findFunction(const QString &funcName, TestViewItem *&item) { return TestViewItem::findChild(funcName, item); };
    virtual int functionCount() { return childCount(); };

    QPointer<TestCode> m_code;
};

class TestFunctionItem : public TestViewItem
{
public:
    TestFunctionItem(TestCaseItem *parent, const QString &label1, bool manualTest, int startLine);
    virtual QString key(int column, bool ascending) const;
    bool m_manualTest;
    int m_startLine;
};

const QPixmap TestViewItem::m_selectpxm(selected_xpm);
const QPixmap TestViewItem::m_unselectpxm(unselected_xpm);
const QPixmap TestViewItem::m_parentAssignpxm(parentAssigned_xpm);
const QPixmap TestViewItem::m_childAssignpxm(childAssigned_xpm);

TestViewItem::TestViewItem(TestViewItem *parent, const QString &name, bool testSuite, int type) :
    QTreeWidgetItem(parent, QStringList() << name, type),
    m_errored(false)
{
    m_parent = parent;
    m_name = name;
    m_isTestSuite = testSuite;
    resetAssignment();
    if (parent && parent->isAssigned())
        parentAssign(false);
}

TestViewItem::TestViewItem(QTreeWidget *parent, const QString &name, bool testSuite, int type) :
    QTreeWidgetItem(parent, QStringList() << name, type),
    m_errored(false)
{
    m_parent = 0;
    m_name = name;
    m_isTestSuite = testSuite;
    resetAssignment();
}

TestViewItem::~TestViewItem()
{
}

TestSuiteItem::~TestSuiteItem()
{
}

TestCaseItem::~TestCaseItem()
{
}

QString TestViewItem::baseSuiteName()
{
    if (m_parent)
        return m_parent->baseSuiteName();
    return m_name;
}

QString TestViewItem::suiteName()
{
    QString ret;
    if (m_parent != 0)
        ret = m_parent->suiteName();
    if (m_isTestSuite) {
        if (!ret.isEmpty()) ret += QLatin1Char('/');
        ret += m_name;
    }
    return QDir::convertSeparators(ret);
}

QString TestViewItem::fullName()
{
    QString ret = suiteName();
    if (!m_isTestSuite) {
        ret += QLatin1Char('/')+ m_name;
    }
    return QDir::convertSeparators(ret);
}

void TestViewItem::toggle()
{
    if (m_assigned)
        unassign();
    else
        assign();
}

void TestViewItem::resetAssignment()
{
    m_assigned = false;
    m_childAssigned = 0;
    m_parentAssigned = 0;
    updatePixmap();
}

void TestViewItem::assign()
{
    if (!m_assigned) {
        m_assigned = true;
        if (parent())
            static_cast<TestViewItem *>(parent())->childAssign();
        parentAssign(true);
        updatePixmap();
    }
}

void TestViewItem::unassign()
{
    if (m_assigned) {
        m_assigned = false;
        if (parent())
            static_cast<TestViewItem *>(parent())->childUnassign();
        parentUnassign(true);
        updatePixmap();
    }
}

void TestViewItem::childAssign()
{
    ++m_childAssigned;
    if (parent())
        static_cast<TestViewItem *>(parent())->childAssign();
    if (m_childAssigned == 1)
        updatePixmap();
}

void TestViewItem::childUnassign()
{
    if (m_childAssigned > 0) {
        --m_childAssigned;
        if (parent())
            static_cast<TestViewItem *>(parent())->childUnassign();
    }
    if (m_childAssigned == 0)
        updatePixmap();
}

void TestViewItem::parentAssign(bool local)
{
    if (!local)
        ++m_parentAssigned;

    int mychildInd = 1;
    TestViewItem *mychild = static_cast<TestViewItem *>(child(0));
    while (mychild != 0) {
        mychild->parentAssign(false);
        mychild = static_cast<TestViewItem *>(child(mychildInd++));
    }
    if (m_parentAssigned == 1)
        updatePixmap();
}

void TestViewItem::parentUnassign(bool local)
{
    if ((m_parentAssigned > 0) && (!local))
        --m_parentAssigned;

    int mychildInd = 1;
    TestViewItem *mychild = static_cast<TestViewItem *>(child(0));
    while (mychild != 0) {
        mychild->parentUnassign(false);
        mychild = static_cast<TestViewItem *>(child(mychildInd++));
    }

    if (m_parentAssigned == 0)
        updatePixmap();
}

bool TestViewItem::findChild(const QString &className, TestViewItem *&item)
{
    int itemInd = 0;
    item = static_cast<TestViewItem *>(child(itemInd++));

    while (item != 0) {
        if (item->text(0) == className)
            return true;
        item = static_cast<TestViewItem *>(child(itemInd++));
    }

    return false;
}

// returns NULL if this item is not a TestViewItem
TestViewItem *TestSelector::recastItem(QTreeWidgetItem *item)
{
    TestViewItem *result = 0;
    if (item && (item->type() == TestViewItem::TestItemType
        || item->type() == TestViewItem::TestSuiteItemType
        || item->type() == TestViewItem::TestCaseItemType
        || item->type() == TestViewItem::TestFunctionItemType)) {
        result = static_cast<TestViewItem *>(item);
    }
    return result;
}

void TestViewItem::updatePixmap()
{
    if (m_errored)
        return;

    if (m_assigned)
        setIcon(0, QIcon(m_selectpxm));
    else if (m_parentAssigned > 0)
        setIcon(0, QIcon(m_parentAssignpxm));
    else if (m_childAssigned > 0)
        setIcon(0, QIcon(m_childAssignpxm));
    else
        setIcon(0, QIcon(m_unselectpxm));
}

void TestViewItem::removeAllChildren()
{
    takeChildren();
}

bool TestViewItem::isTestSuite()
{
    return m_isTestSuite;
}

TestSuiteItem *TestViewItem::parentSuite()
{
    if (m_parent != 0 && m_parent->isTestSuite())
        return static_cast<TestSuiteItem *>(m_parent);
    return 0;
}

TestSuiteItem::TestSuiteItem(const QString &name, QTreeWidget *parent) :
    TestViewItem(parent, name, true, TestViewItem::TestSuiteItemType)
{
    m_parent = 0;
}

TestSuiteItem::TestSuiteItem(const QString &name, TestSuiteItem *parent) :
    TestViewItem(parent, name, true, TestViewItem::TestSuiteItemType)
{
    m_parent = parent;
}

QString TestSuiteItem::key(int column, bool /*ascending*/) const
{
    return text(column);
}

bool splitSuiteName(const QString &fullName, QString &first, QString &rest)
{
    QStringList L = fullName.split(QDir::separator());
    if (L.count() == 0)
        return false;

    first = L[0];
    rest.clear();
    if (L.count() > 1) {
        for (int i = 1; i < L.count(); ++i) {
            if (!rest.isEmpty())
                rest += QDir::separator();
            rest += L[i];
        }
    }
    return true;
}

TestSuiteItem *TestSuiteItem::addSuite(const QString &suiteName)
{
    QString sn, next;
    splitSuiteName(suiteName, sn, next);
    TestSuiteItem *suite = findSuite(sn);

    if (!suite) {
        suite = new TestSuiteItem(sn, this);
        addChild(suite);
        sortChildren(0, Qt::AscendingOrder);
    }

    if (next.isEmpty())
        return suite;
    return suite->addSuite(next);
}

TestSuiteItem *TestSuiteItem::findSuite(const QString &suiteName)
{
    QString sn, next;
    splitSuiteName(suiteName, sn, next);

    int itemInd = 0;
    TestViewItem *item = static_cast<TestViewItem *>(child(itemInd++));

    while (item != 0) {
        if (sn == item->name()) {
            if (item->isTestSuite()) {
                if (next.isEmpty()) {
                    return static_cast<TestSuiteItem *>(item);
                } else {
                    TestSuiteItem *tmp = static_cast<TestSuiteItem *>(item);
                    return static_cast<TestSuiteItem *>(tmp->findSuite(next));
                }
            } else {
                return item->parentSuite();
            }
        }
        item = static_cast<TestViewItem *>(child(itemInd++));
    }
    return 0;
}

TestCaseItem *TestSuiteItem::addCase(TestCode *code)
{
    TestSettings testSettings;
    QString fname = code->visualFileName(testSettings.componentViewMode());

    TestCaseItem *item = findCase(fname);
    if (!item) {
        item = new TestCaseItem(code, this);
        addChild(item);
        sortChildren(0, Qt::AscendingOrder);
    }

    return item;
}

TestCaseItem *TestSuiteItem::findCase(const QString &tcFileName)
{
    int itemInd = 0;
    TestViewItem *item = static_cast<TestViewItem *>(child(itemInd++));
    while (item != 0) {
        if (item->fullName() == tcFileName)
            return static_cast<TestCaseItem *>(item);

        item = static_cast<TestViewItem*>(child(itemInd++));
    }
    return 0;
}

TestCaseItem::TestCaseItem(TestCode *code, TestSuiteItem *parent) :
    TestViewItem(parent, code->baseFileName(), false, TestViewItem::TestCaseItemType)
{
    m_code = code;
    m_parent = parent;
    if (code && code->isErrored()) {
        m_errored = true;
        setForeground(0, QBrush(QColor(192,192,192)));
        setIcon(0, QIcon(QLatin1String(":/error.png")));
        setToolTip(0, "This file contains syntax errors");
    }
}

QString TestCaseItem::key(int column, bool /*ascending*/) const
{
    return text(column);
}

TestFunctionItem *TestCaseItem::addFunction(const QString &funcName, bool manualTest, int startLine)
{
    if (m_errored) {
        m_errored = false;
        setForeground(0, QBrush(QColor(0,0,0)));
        setToolTip(0, QString());
        updatePixmap();
    }

    TestFunctionItem *item;
    if (!findFunction(funcName, item)) {
        item = new TestFunctionItem(this, funcName, manualTest, startLine);
    } else {
        item->setData(1, 0, startLine);
        if (manualTest)
            item->setForeground(0, QBrush(QColor(255,0,0)));
        else
            item->setForeground(0, QBrush(QColor(0,0,0)));
    }
    return item;
}

bool TestCaseItem::findFunction(const QString &className, TestFunctionItem *&item)
{
    TestViewItem *tmp;
    if (TestViewItem::findChild(className, tmp)) {
        item = static_cast<TestFunctionItem *>(tmp);
        return true;
    }
    return false;
}

TestFunctionItem::TestFunctionItem(TestCaseItem *parent, const QString &label1, bool manualTest, int startLine) :
    TestViewItem(parent, label1, false, TestViewItem::TestFunctionItemType)
{
    m_manualTest = manualTest;
    m_startLine = startLine;
    setData(1, 0, startLine);
    // show function name in RED if it's a manual test
    if (manualTest)
        setForeground(0, QBrush(QColor(255,0,0)));
}

QString TestFunctionItem::key(int column, bool /*ascending*/) const
{
    QString txt = text(column);
    return txt;
}

int TestSelector::m_refCount = 0;

TestSelector::TestSelector(QWidget *parent) :
    QTreeWidget(parent)
{
    TestSettings testSettings;
    m_hiddenTestTypes = static_cast<TestCode::TestTypes>(testSettings.hiddenTestTypes());

    m_curItem = 0;

    connect(TestExecuter::instance(), SIGNAL(testStarted()),
        this, SLOT(updateActions()), Qt::DirectConnection);
    connect(TestExecuter::instance(), SIGNAL(testFinished()),
        this, SLOT(updateActions()), Qt::DirectConnection);

    setColumnCount(1);
    setSortingEnabled(false);
    setRootIsDecorated(true);
    setSelectionMode(QAbstractItemView::SingleSelection);

    m_curTest.m_testFunction.clear();;
    m_curTest.m_line = -1;
    m_curTest.m_code = 0;

    connect(this, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
        this, SLOT(onSelectionChanged()), Qt::DirectConnection);
    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
        this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)), Qt::DirectConnection);
    connect(&m_testCollection, SIGNAL(testChanged(TestCode*)),
        this, SLOT(onChanged(TestCode*)));
    connect(&m_testCollection, SIGNAL(testRemoved(TestCode*)),
        this, SLOT(onRemoved(TestCode*)));
    connect(&m_testCollection, SIGNAL(changed()),
        this, SLOT(init()));
    connect(&TestConfigurations::instance(), SIGNAL(activeConfigurationChanged()),
        this, SLOT(onActiveConfigurationChanged()));
    connect(this, SIGNAL(activeConfigurationChanged()),
        &TestConfigurations::instance(), SLOT(onActiveConfigurationChanged()));
    connect(this, SIGNAL(testSelectionChanged(QStringList)),
        &TestConfigurations::instance(), SLOT(onTestSelectionChanged(QStringList)));
    connect(&TestConfigurations::instance(), SIGNAL(testSelectionChanged(QStringList, QObject*)),
        this, SLOT(onTestSelectionChanged(QStringList, QObject*)));

    setFocusPolicy(Qt::StrongFocus);
    header()->hide();

    // create context menu (to be used in various places)
    m_testContextMenu = new TestContextMenu(this);
    connect(m_testContextMenu, SIGNAL(selectAllTests()),
        this, SLOT(assignAll()), Qt::DirectConnection);
    connect(m_testContextMenu, SIGNAL(selectAllManualTests()),
        this, SLOT(assignAllManual()), Qt::DirectConnection);
    connect(m_testContextMenu, SIGNAL(deselectAllTests()),
        this, SLOT(unassignAll()), Qt::DirectConnection);
    connect(m_testContextMenu, SIGNAL(toggleSelection()),
        this, SLOT(toggleSelection()), Qt::DirectConnection);

    // add context menu to test selector widget
    m_testMenu = new QMenu;
    m_testContextMenu->init(m_testMenu, 1, this);

    m_componentViewMode = new QAction(tr("Component View"), this);
    m_componentViewMode->setCheckable(true);
    m_componentViewMode->setChecked(testSettings.componentViewMode());
    connect(m_componentViewMode, SIGNAL(toggled(bool)), this, SLOT(setComponentViewMode(bool)));

    m_showUnitTests = new QAction(tr("Show Unit Tests"), this);
    m_showUnitTests->setCheckable(true);
    m_showUnitTests->setChecked(!(m_hiddenTestTypes & TestCode::TypeUnitTest));
    connect(m_showUnitTests, SIGNAL(toggled(bool)), this, SLOT(showUnitTests(bool)));

    m_showIntegrationTests = new QAction(tr("Show Integration Tests"), this);
    m_showIntegrationTests->setCheckable(true);
    m_showIntegrationTests->setChecked(!(m_hiddenTestTypes & TestCode::TypeIntegrationTest));
    connect(m_showIntegrationTests, SIGNAL(toggled(bool)), this, SLOT(showIntegrationTests(bool)));

    m_showPerformanceTests = new QAction(tr("Show Performance Tests"), this);
    m_showPerformanceTests->setCheckable(true);
    m_showPerformanceTests->setChecked(!(m_hiddenTestTypes & TestCode::TypePerformanceTest));
    connect(m_showPerformanceTests, SIGNAL(toggled(bool)), this, SLOT(showPerformanceTests(bool)));

    m_showSystemTests = new QAction(tr("Show System Tests"), this);
    m_showSystemTests->setCheckable(true);
    m_showSystemTests->setChecked(!(m_hiddenTestTypes & TestCode::TypeSystemTest));
    connect(m_showSystemTests, SIGNAL(toggled(bool)), this, SLOT(showSystemTests(bool)));

    ++m_refCount;
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();
    QMenu *m = am->actionContainer(Core::Id("QtTestPlugin.TestMenu"))->menu();
    m->setEnabled(true);
}

void TestSelector::clear()
{
    QTreeWidget::clear();

    while (topLevelItemCount() > 0)
        delete takeTopLevelItem(0);
}

void TestSelector::onActiveConfigurationChanged()
{
    clear();
    init();
}

void TestSelector::onTestSelectionChanged(const QStringList &selection, QObject *originator)
{
    if (originator != this) {
        QTreeWidgetItemIterator it(this);
        TestViewItem *child;
        while (*it) {
            child = recastItem(*it);
            if (child)
                child->unassign();
            ++it;
        }
        setSelectedTests(false, selection);
    }
}

void TestSelector::setComponentViewMode(bool filter)
{
    m_testSettings.setComponentViewMode(filter);
    emit activeConfigurationChanged();
}

void TestSelector::showUnitTests(bool show)
{
    if (show)
        m_hiddenTestTypes &= ~TestCode::TypeUnitTest;
    else
        m_hiddenTestTypes |= TestCode::TypeUnitTest;

    m_testSettings.setHiddenTestTypes(m_hiddenTestTypes);
    emit activeConfigurationChanged();
}

void TestSelector::showIntegrationTests(bool show)
{
    if (show)
        m_hiddenTestTypes &= ~TestCode::TypeIntegrationTest;
    else
        m_hiddenTestTypes |= TestCode::TypeIntegrationTest;

    m_testSettings.setHiddenTestTypes(m_hiddenTestTypes);
    emit activeConfigurationChanged();
}

void TestSelector::showPerformanceTests(bool show)
{
    if (show)
        m_hiddenTestTypes &= ~TestCode::TypePerformanceTest;
    else
        m_hiddenTestTypes |= TestCode::TypePerformanceTest;

    m_testSettings.setHiddenTestTypes(m_hiddenTestTypes);
    emit activeConfigurationChanged();
}

void TestSelector::showSystemTests(bool show)
{
    if (show)
        m_hiddenTestTypes &= ~TestCode::TypeSystemTest;
    else
        m_hiddenTestTypes |= TestCode::TypeSystemTest;

    m_testSettings.setHiddenTestTypes(m_hiddenTestTypes);
    emit activeConfigurationChanged();
}

void TestSelector::init()
{
    Core::ModeManager *mgr = Core::ModeManager::instance();
    if (mgr && mgr->currentMode()->id() == "Edit") {
        TestConfigurations *tc = &TestConfigurations::instance();
        if (tc->activeConfiguration()) {
            for (int i = 0; i < m_testCollection.count(); ++i)
                addTest(m_testCollection.testCode(i));
        }
        // apply settings we had last time
        select(tc->currentTestCase(), tc->currentTestFunction());
        setSelectedTests(false, tc->selectedTests());
    } else {
        // Try again later.
        m_initTimer.setSingleShot(true);
        m_initTimer.start(1000);
    }
}

TestSelector::~TestSelector()
{
    if (--m_refCount == 0) {
        Core::ICore *core = Core::ICore::instance();
        if (core) {
            Core::ActionManager *am = core->actionManager();
            if (am) {
                QMenu *m = am->actionContainer(Core::Id("QtTestPlugin.TestMenu"))->menu();
                m->setEnabled(false);
            }
        }
    }

    delete m_testContextMenu;
}

void TestSelector::setContextMenu(TestContextMenu *contextMenu)
{
    m_testContextMenu = contextMenu;
}

void TestSelector::resize (int w, int h)
{
    QTreeWidget::resize(w,h);
    setColumnWidth(0, w-4);
}

void TestSelector::setGeometry (int x, int y, int w, int h)
{
    QTreeWidget::setGeometry(x,y,w,h);
    setColumnWidth(0, w-4);
}

void TestSelector::onSelectionChanged()
{
    bool selected = false;
    bool multiSelection = false;
    m_testCollection.setCurrentEditedTest(0);
    m_curTest.m_code = 0;

    m_curTest.m_basePath.clear();
    checkSuite(0, selected, multiSelection);
    if (!selected || multiSelection) {
        // no function and no class selected
        m_curTest.m_code = 0;
        m_curTest.m_testFunction.clear();
        m_curTest.m_line = -1;
        m_testContextMenu->updateSingleTestAction(QString());
    }

    if (m_curTest.m_code) {
        QString cmd = m_curTest.m_code->testCase();
        if (!m_curTest.m_testFunction.isEmpty())
            cmd += "::"+ m_curTest.m_testFunction;
        m_testContextMenu->updateSingleTestAction(cmd);
    }

    updateActions();

    if (m_curTest.m_code) {
        if (m_curTest.m_code->openTestInEditor(m_curTest.m_testFunction)) {
            m_testCollection.setCurrentEditedTest(m_curTest.m_code);
            TestConfigurations::instance().setCurrentTest(m_curTest.m_code->testCase(),
                m_curTest.m_testFunction);
        }
    }
}

void TestSelector::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    m_curItem = recastItem(item);
    toggleSelection();
}

void TestSelector::updateActions()
{
    m_testContextMenu->updateActions(
        TestConfigurations::instance().activeConfiguration() != 0,
        TestExecuter::instance()->testBusy(),
        TestExecuter::instance()->testStopped());
}

void TestSelector::selectGroup()
{
    QPointer<SelectDlg> dlg = new SelectDlg("Select a group",
        "Please select the groups that you'd want to test",
        QAbstractItemView::MultiSelection,
        300, 400, QStringList() << "Groups", QByteArray(), this);

    QStringList groupsList;
    for (int i = 0; i < m_testCollection.count(); ++i) {
        TestCode *tc = m_testCollection.testCode(i);
        if (tc) {
            for (uint j = 0; j < tc->testFunctionCount(); ++j) {
                TestFunctionInfo *inf = tc->testFunction(j);
                if (inf && (inf->testStartLine() >= 0)) {
                    if (!inf->testGroups().isEmpty()) {
                        dlg->addSelectableItems(inf->testGroups()
                            .split(QLatin1Char(','), QString::SkipEmptyParts));
                        groupsList.append(tc->testCase() + "::" + inf->functionName()
                            + QLatin1Char('@') + inf->testGroups().split(QLatin1Char(','), QString::SkipEmptyParts).join(QString(QLatin1Char('@'))));
                    }
                }
            }
        }
    }

    if (dlg->exec()) {
        QStringList selectedGroups = dlg->selectedItems();
        unassignAll();
        int current = 0;
        while (current < groupsList.count()) {
            bool containsAGroup = false;
            for (int i = 0; i < selectedGroups.count(); ++i) {
                if (groupsList[current].contains(QString(QLatin1Char('@') + selectedGroups[i]))) {
                    containsAGroup = true;
                    break;
                }
            }
            if (!containsAGroup) {
                groupsList.removeAt(current);
            } else {
                groupsList[current] = groupsList[current].left(groupsList[current].indexOf(QLatin1Char('@')));
                ++current;
            }
        }
        setSelectedTests(true, groupsList);
    }
    delete dlg;
}

QString TestSelector::curTestSuite(bool fullPath)
{
    if ((m_curTest.m_code == 0) || m_curTest.m_code->actualFileName().isEmpty())
        return QString();
    if (fullPath)
        return m_curTest.m_code->fullVisualSuitePath(m_testSettings.componentViewMode());
    return m_curTest.m_code->visualBasePath();
}

void TestSelector::checkSuite(TestSuiteItem *base, bool &selected, bool &multiSelection)
{
    TestViewItem *tmpViewItem;
    int tmpViewItemInd = 1;
    if (base == 0)
        tmpViewItem = static_cast<TestViewItem *>(topLevelItem(0));
    else
        tmpViewItem = static_cast<TestViewItem *>(base->child(0));

    while (tmpViewItem != 0) {

        // Find out if the suite is selected
        if (tmpViewItem->isTestSuite()) {

            TestSuiteItem *T = static_cast<TestSuiteItem *>(tmpViewItem);

            if (T->isSelected()) {
                if (selected)
                    multiSelection = true;
                selected = true;
                if (!multiSelection) {
                    m_curTest.m_testFunction.clear();
                    m_curTest.m_line = -1;
                    if (!m_curTest.m_code
                        || (m_curTest.m_code->visualFileName(m_testSettings.componentViewMode()) != T->fullName())) {

                        m_curTest.m_code = m_testCollection.findCodeByVisibleName(T->fullName(),
                            m_testSettings.componentViewMode());
                    }
                    m_curTest.m_basePath = T->baseSuiteName();
                }
            }

            checkSuite(T, selected, multiSelection);
        } else {

            TestCaseItem *testCase = static_cast<TestCaseItem *>(tmpViewItem);
            // Find out if the classItem is selected
            if (testCase->isSelected()) {
                if (selected)
                    multiSelection = true;
                selected = true;
                if (!multiSelection) {
                    m_curTest.m_testFunction.clear();
                    m_curTest.m_line = -1;
                    if (!m_curTest.m_code
                        || (m_curTest.m_code->visualFileName(m_testSettings.componentViewMode()) != testCase->fullName())) {

                        m_curTest.m_code = m_testCollection.findCodeByVisibleName(testCase->fullName(),
                            m_testSettings.componentViewMode());
                    }
                    if (m_curTest.m_code)
                        m_curTest.m_basePath = m_curTest.m_code->actualBasePath();
                }
            }

            // Find out if there's a testfunction selected
            int testFunctionInd = 0;
            TestFunctionItem *testFunction = static_cast<TestFunctionItem *>(testCase->child(testFunctionInd++));
            while (testFunction != 0) {
                if (testFunction->isSelected()) {
                    if (selected)
                        multiSelection = true;
                    selected = true;
                    if (!multiSelection) {
                        m_curTest.m_testFunction = testFunction->name();
                        m_curTest.m_line = -1;
                        if (!m_curTest.m_code
                            || (m_curTest.m_code->visualFileName(m_testSettings.componentViewMode()) != testCase->fullName())) {

                            m_curTest.m_code = m_testCollection.findCodeByVisibleName(testCase->fullName(),
                                m_testSettings.componentViewMode());
                        }
                    }
                }
                NEXT_SIBLING(TestFunctionItem, testFunction);
            }
        }
        NEXT_SIBLING(TestViewItem, tmpViewItem);
    }
}

QStringList TestSelector::selectedTests()
{
    QStringList list;
    getSelectedTests(0, list, false);
    return list;
}

void TestSelector::getSelectedTests(TestSuiteItem *base, QStringList &list, bool isAssigned)
{
    TestViewItem *tmpViewItem;
    int tmpViewItemInd = 1;
    if (base == 0)
        tmpViewItem = static_cast<TestViewItem *>(topLevelItem(0));
    else
        tmpViewItem = static_cast<TestViewItem *>(base->child(0));

    while (tmpViewItem != 0) {
        if (tmpViewItem->isTestSuite()) {

            TestSuiteItem *T = static_cast<TestSuiteItem *>(tmpViewItem);
            getSelectedTests(T, list, isAssigned || T->isAssigned());

        } else {

            int testFunctionInd = 1;
            TestCaseItem *testCase = static_cast<TestCaseItem *>(tmpViewItem);
            if (isAssigned || testCase->isAssigned()) {

                TestFunctionItem *testFunction = static_cast<TestFunctionItem *>(testCase->child(0));
                while (testFunction != 0) {
                    if (testCase->m_code) {
                        list.append(testCase->m_code->testCase()
                            + "::" + testFunction->name());
                    }
                    NEXT_SIBLING(TestFunctionItem, testFunction);
                }
            } else {
                if (testCase->childAssigned()) {
                    TestFunctionItem *testFunction = static_cast<TestFunctionItem *>(testCase->child(0));
                    while (testFunction != 0) {
                        if (testFunction->isAssigned()) {
                            if (testCase->m_code) {
                                list.append(testCase->m_code->testCase()
                                    + "::" + testFunction->name());
                            }
                        }
                        NEXT_SIBLING(TestFunctionItem, testFunction);
                    }
                }
            }
        }
        NEXT_SIBLING(TestViewItem, tmpViewItem);
    }
}

void TestSelector::setSelectedTests(bool save, const QStringList &list)
{
    foreach (const QString &sel, list) {
        QString tcName = sel.left(sel.indexOf("::"));
        QString function = sel.mid(sel.indexOf("::")+2);
        TestCode *tmp = m_testCollection.findCodeByTestCaseName(tcName);
        if (tmp) {
            QString fname = tmp->visualFileName(m_testSettings.componentViewMode());

            int testFunctionInd = 1;
            TestCaseItem *testCase = findCase(fname);
            if (testCase) {
                TestFunctionItem *testFunction = static_cast<TestFunctionItem *>(testCase->child(0));
                while (testFunction != 0) {
                    if (testFunction->name() == function) {
                        testFunction->assign();
                        break;
                    }
                    NEXT_SIBLING(TestFunctionItem, testFunction);
                }
            }
        }
    }

    if (save)
        emit testSelectionChanged(list);
}

void TestSelector::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Space) {
        m_curItem = recastItem(currentItem());
        toggleSelection();
        e->accept();
    } else {
        QTreeWidget::keyPressEvent(e);
    }
}

void TestSelector::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Space)
        e->accept();
    else
        QTreeWidget::keyReleaseEvent(e);
}

void TestSelector::mousePressEvent(QMouseEvent *e)
{
    m_curItem = 0;
    if (itemAt(e->pos()))
        m_curItem = recastItem(itemAt(e->pos()));

    if (e->button() == Qt::RightButton
#ifdef Q_OS_MAC
        || (e->button() == Qt::LeftButton
        && (e->modifiers() == Qt::AltModifier || e->modifiers() == Qt::MetaModifier))
#endif
    ) {
        if (e->modifiers() == Qt::AltModifier) {
            toggleSelection();
        } else if (e->button() == Qt::RightButton
            || (e->button() == Qt::LeftButton && e->modifiers() == Qt::MetaModifier)) {
            if (m_testContextMenu) {
                updateActions();
                QString cmd;
                if (m_curItem) {
                    if (m_curItem->isAssigned())
                        cmd = tr("Unselect '%1'").arg(m_curItem->name());
                    else
                        cmd = tr("Select '%1'").arg(m_curItem->name());
                }
                m_testContextMenu->updateToggleAction(cmd);

                e->accept();
                QPoint p = mapToGlobal(e->pos());
                p.setX(p.x()+3);
                m_testMenu->exec(p);
            }
        }
    } else {
        QTreeWidget::mousePressEvent(e);
    }
}

void TestSelector::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton
#ifdef Q_OS_MAC
        || (e->button() == Qt::LeftButton
        && (e->modifiers() == Qt::AltModifier || e->modifiers() == Qt::MetaModifier))
#endif
    ) {
        e->accept();
    } else {
        QTreeWidget::mouseReleaseEvent(e);
    }
}

void TestSelector::toggleSelection()
{
    if (m_curItem)
        m_curItem->toggle();
    emit testSelectionChanged(selectedTests());
}

void TestSelector::rescan()
{
    clear();
    TestConfigurations::instance().rescan();
}

void TestSelector::onChanged(TestCode *testCode)
{
    if (testCode)
        addTest(testCode);
}

void TestSelector::onRemoved(TestCode *testCode)
{
    if (testCode)
        removeCase(testCode, true, true);
}

void TestSelector::select(TestCaseRec rec)
{
    QString fname;
    if (rec.m_code)
        fname = rec.m_code->visualFileName(m_testSettings.componentViewMode());
    select(fname, rec.m_testFunction);
}

void TestSelector::select(const QString &testCaseName, const QString &funcName)
{
    TestCode *tc = m_testCollection.findCodeByTestCaseName(testCaseName);
    select(tc, funcName);
}

void TestSelector::select(TestCode *tc, const QString &funcName)
{
    QList<QTreeWidgetItem *> items = selectedItems();
    for (int i = 0; i < items.size(); ++i)
        if (items.at(i)->isSelected())
            items.at(i)->setSelected(false);

    if (tc) {
        QString tcFileName = tc->visualFileName(m_testSettings.componentViewMode());
        TestCaseItem *testCase = findCase(tcFileName);
        if (testCase) {
            if (!funcName.isEmpty()) {
                TestFunctionItem *testFunction;
                if (testCase->findFunction(funcName, testFunction)) {
                    expandItem(testFunction);
                    scrollToItem(testFunction);
                    testFunction->setSelected(true);
                    onSelectionChanged();
                }
            } else {
                expandItem(testCase);
                scrollToItem(testCase);
                testCase->setSelected(true);
                onSelectionChanged();
            }
        } else {
            TestSuiteItem *testSuite = findSuite(tcFileName);
            if (testSuite) {
                expandItem(testSuite);
                scrollToItem(testSuite);
                testSuite->setSelected(true);
                onSelectionChanged();
            }
        }
    }
}

TestSuiteItem *TestSelector::addSuite(TestCode *code)
{
    if (!code)
        return 0;

    QString bs = code->visualBasePath();
    TestSuiteItem *base = findSuite(bs);
    if (!base) {
        // no base suite exists yet, so create it
        base = new TestSuiteItem(bs, this);
    }
    QString rest = code->fullVisualSuitePath(m_testSettings.componentViewMode()).mid(bs.length()+1);
    if (rest.isEmpty())
        return base;
    if (base)
        return base->addSuite(rest);
    return 0;
}

bool TestSelector::removeSuite(const QString &suiteName, bool force)
{
    return removeSuite2(static_cast<TestSuiteItem *>(topLevelItem(0)), suiteName, force);
}

bool TestSelector::removeSuite2(TestSuiteItem *base, const QString &suiteName, bool force)
{
    if (base == 0)
        return false;

    QStringList L = suiteName.split(QDir::separator());

    int tmpItemInd = 0;
    if (base) {
        if (base->parent())
            tmpItemInd = base->parent()->indexOfChild(base)+1;
        else
            tmpItemInd = indexOfTopLevelItem(base)+1;
    }

    if (L.count() > 1) {
        TestSuiteItem *tmpItem = base;
        while (tmpItem != 0) {
            if (tmpItem->text(0) == L[0]) {
                QString next;
                for (int i = 1; i < L.count(); ++i) {
                    next += L[i];
                    next += QDir::separator();
                }
                // remove the last '/'
                next = next.left(next.length()-1);

                // I should actually remove the suite as well if it doesn't contain any more children...
                // but I ignore that fact for the moment.
                return removeSuite2(static_cast<TestSuiteItem *>(tmpItem->child(0)), next, force);
            }

            NEXT_SIBLING(TestSuiteItem, tmpItem);
        }
    } else {
        TestSuiteItem* tmpItem = base;
        while (tmpItem != 0) {
            if (tmpItem->text(0) == suiteName) {
                if (caseCount(suiteName) > 0) {
                    if (force)
                        tmpItem->removeAllChildren();
                    else
                        return false;
                }

                delete tmpItem;
                return true;
            } else {
                NEXT_SIBLING(TestSuiteItem, tmpItem);
            }
        }
    }

    return false;
}

TestSuiteItem* TestSelector::findSuite(const QString &suiteName)
{
    int tsInd = 0;
    TestSuiteItem *item = static_cast<TestSuiteItem *>(topLevelItem(tsInd));
    while (item != 0) {
        if (suiteName == item->suiteName()) {
            return item;
        }
        if (suiteName.startsWith(item->suiteName())) {
            QString rest = suiteName;
            rest.remove(item->suiteName() + QDir::separator());
            return item->findSuite(rest);
        }
        item = static_cast<TestSuiteItem *>(topLevelItem(tsInd++));
    }
    return 0;
}

TestCaseItem *TestSelector::findCase(const QString &tcFileName)
{
    TestSuiteItem *testSuite = findSuite(tcFileName);
    if (testSuite) {
        TestCaseItem *tmp = testSuite->findCase(tcFileName);
        return tmp;
    }
    return 0;
}

TestCaseItem *TestSelector::addCase(TestCode *code)
{
    TestSuiteItem *testSuite = addSuite(code);
    if (testSuite) {
        TestCaseItem *testCase = testSuite->addCase(code);
        if (testCase != 0)
            return testCase;
    }
    return 0;
}

bool TestSelector::removeCase(TestCode *code, bool clean, bool force)
{
    TestSuiteItem *suite = findSuite(code->fullVisualSuitePath(m_testSettings.componentViewMode()));
    if (!suite)
        return false;

    TestCaseItem *Case = suite->findCase(code->visualFileName(m_testSettings.componentViewMode()));
    if (!Case)
        return false;

    if (Case->functionCount() > 0) {
        if (force)
            Case->removeAllChildren();
        else
            return false;
    }

    suite->takeChild(suite->indexOfChild(Case));
    if (clean)
        removeSuite(code->fullVisualSuitePath(m_testSettings.componentViewMode()), false);
    return true;
}

uint TestSelector::caseCount(const QString &suiteName)
{
    TestSuiteItem *suite = findSuite(suiteName);
    if (!suite)
        return 0;

    return suite->childCount();
}

void TestSelector::removeFunction(TestCaseItem *testCase, const QString &funcName)
{
    if (!testCase)
        return;
    TestFunctionItem *testFunction;
    if (testCase->findFunction(funcName, testFunction))
        testCase->takeChild(testCase->indexOfChild(testFunction));
}

void TestSelector::assignAllManual()
{
    QString startPath;
    if (m_curItem)
        startPath = m_curItem->suiteName();

    TestCollection tc;
    setSelectedTests(true, tc.manualTests(startPath, m_testSettings.componentViewMode()));
}

void TestSelector::assignAll()
{
    QTreeWidgetItemIterator it(this);
    TestViewItem *child;
    while (*it) {
        if ((*it)->childCount() == 0) {
            child = recastItem(*it);
            if (child)
                child->assign();
        }
        ++it;
    }
    emit testSelectionChanged(selectedTests());
}

void TestSelector::unassignAll()
{
    QTreeWidgetItemIterator it(this);
    TestViewItem *child;
    while (*it) {
        child = recastItem(*it);
        if (child)
            child->unassign();
        ++it;
    }
    emit testSelectionChanged(QStringList());
}

void TestSelector::addTest(TestCode *code)
{
    bool newCase = false;
    if (!code->isInitialized())
        return;

    if (m_hiddenTestTypes & code->testType())
        return;

    if (!findSuite(code->visualBasePath()))
        new TestSuiteItem(code->visualBasePath(), this);

    TestCaseItem *testCase = findCase(code->visualFileName(m_testSettings.componentViewMode()));
    if (!testCase) {
        testCase = addCase(code);
        newCase = true;
    }

    // Check if there are test functions that are not yet in the tree
    setUpdatesEnabled(false);
    for (uint i = 0; i < code->testFunctionCount(); ++i) {
        TestFunctionInfo *testInfo = code->testFunction(i);
        bool manualTest = testInfo->isManualTest();
        int startLine = testInfo->testStartLine();
        QString S = testInfo->functionName();
        if ((S != "init") && (S != "cleanup") && (S != "initTestCase")
            && (S != "cleanupTestCase") && !S.endsWith("_data")) {
            testCase->addFunction(S, manualTest, startLine);
        }
    }

    if (!newCase) {
        // Remove test functions that no longer exist
        TestFunctionItem *testFunction;
        for (int tfInd = testCase->childCount()-1; tfInd >= 0; --tfInd) {
            testFunction = static_cast<TestFunctionItem *>(testCase->child(tfInd));
            if (testFunction) {
                QString funcName = testFunction->text(0);
                if (!code->testFunctionExists(funcName))
                    removeFunction(testCase, funcName);
            }
        }
    }

    testCase->sortChildren(1, Qt::AscendingOrder);
    setUpdatesEnabled(true);
}

//**************** Actions from Context menu *******************
void TestSelector::testInsertUnitOrSystemTest()
{
    QString basePath = m_curTest.m_basePath;
    if (basePath.isEmpty()) {
        TestConfig *cfg = TestConfigurations::instance().activeConfiguration();
        if (cfg)
            basePath = cfg->srcPath();
    }
    QPointer<NewTestCaseDlg> dlg = new NewTestCaseDlg(basePath, this);
    if (dlg->exec() == QDialog::Accepted) {
        TestGenerator gen;
        QString classFile;

        gen.enableComponentInTestName(dlg->componentInName());
        gen.setTestCase(dlg->mode(),
            dlg->location(),
            QString(), // use automatic "subdir" selection
            dlg->testCaseName(),
            dlg->testedComponent(),
            dlg->testedClassName(),
            classFile);

        if (gen.generateTest()) {
            TestCode *tc = m_testCollection.findCode(gen.generatedFileName(),
                basePath, QString());
            if (tc) {
                // and select so that this is the file we are showing
                select(tc, QString());
            }
        }
    }
    delete dlg;
}
