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

#ifndef TESTSELECTOR_H
#define TESTSELECTOR_H

#include "testsuite.h"
#include "testsettings.h"

#include <QString>
#include <QTreeWidget>

QT_BEGIN_NAMESPACE
class QMenu;
class QAction;
QT_END_NAMESPACE

class TestSuiteItem;
class TestCaseItem;
class TestFunctionItem;
class TestContextMenu;
class TestCode;

class TestViewItem : public QTreeWidgetItem
{
public:
    TestViewItem(TestViewItem *parent, const QString &name,
        bool testSuite, int type = TestViewItem::TestItemType);
    TestViewItem(QTreeWidget *parent, const QString &name,
        bool testSuite, int type=TestViewItem::TestItemType);
    virtual ~TestViewItem();

    QString name() { return m_name; }

    bool isAssigned() { return m_assigned; }
    bool childAssigned() { return m_childAssigned > 0; }

    void resetAssignment();
    void assign();
    void unassign();
    void childAssign();
    void childUnassign();
    void parentAssign(bool local);
    void parentUnassign(bool local);

    virtual bool findChild(const QString &className, TestViewItem *&item);

    void removeAllChildren();

    void toggle();

    bool isTestSuite();
    TestSuiteItem *parentSuite();

    QString baseSuiteName();
    QString suiteName();
    QString fullName();

    enum TestItemTypes {
        TestItemType = QTreeWidgetItem::UserType + 1,
        TestSuiteItemType,
        TestCaseItemType,
        TestFunctionItemType
    };

protected:
    void updatePixmap();

    QString m_name;
    TestViewItem *m_parent;
    bool m_errored;

private:
    uint m_childAssigned;
    uint m_parentAssigned;
    bool m_assigned;
    bool m_isTestSuite;

    static const QPixmap m_selectpxm;
    static const QPixmap m_unselectpxm;
    static const QPixmap m_parentAssignpxm;
    static const QPixmap m_childAssignpxm;
};

class TestSelector : public QTreeWidget
{
    Q_OBJECT

public:
    explicit TestSelector(QWidget *parent = 0);
    virtual ~TestSelector();

    void setContextMenu(TestContextMenu *contextMenu);

    void setSelectedTests(bool save, const QStringList &list);
    QStringList selectedTests();

protected:
    virtual void resize(int w, int h);

    TestViewItem *recastItem(QTreeWidgetItem *item);

    QString curTestSuite(bool fullPath = false);
    TestCode *curTestCode() { return m_curTest.m_code; }

    TestSuiteItem *addSuite(TestCode *code);
    TestSuiteItem *findSuite(const QString &suitePath);
    bool removeSuite(const QString &suitePath, bool force = false);

    TestCaseItem *findCase(const QString &tcFileName);
    TestCaseItem *addCase(TestCode *code);
    bool removeCase(TestCode *code, bool clean = false, bool force = false);
    uint caseCount(const QString &suiteName);

    void removeFunction(TestCaseItem *Case, const QString &funcName);

    virtual void clear();

public slots:
    void init();

    // Action handlers...
    virtual void testInsertUnitOrSystemTest();
    virtual void selectGroup();

    void updateActions();
    void rescan();

    void select(const QString &tcFileName, const QString &testFunc);
    void select(TestCode *tc, const QString &testFunc);
    void select(TestCaseRec rec);

    void assignAll();
    void assignAllManual();
    void unassignAll();
    void toggleSelection();

    void onChanged(TestCode *testCode);
    void onRemoved(TestCode *testCode);
    void onSelectionChanged();
    void onItemDoubleClicked(QTreeWidgetItem *, int);
    void onActiveConfigurationChanged();
    void onTestSelectionChanged(const QStringList &, QObject *);

    virtual void setGeometry (int x, int y, int w, int h);
    void setComponentViewMode(bool filter);
    void showUnitTests(bool);
    void showIntegrationTests(bool);
    void showPerformanceTests(bool);
    void showSystemTests(bool);

signals:
    void activeConfigurationChanged();
    void testSelectionChanged(const QStringList &);

protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);

public:
    QAction *m_componentViewMode;
    QAction *m_showUnitTests;
    QAction *m_showIntegrationTests;
    QAction *m_showPerformanceTests;
    QAction *m_showSystemTests;

private:
    bool removeSuite2(TestSuiteItem *base, const QString &suiteName, bool force);
    void checkSuite(TestSuiteItem *testSuite, bool &selected, bool &multiSelection);

    void getSelectedTests(TestSuiteItem *base, QStringList &list, bool isAssigned);

    void resetGroupsMenu();
    void addTest(TestCode *code);

    TestCaseRec m_curTest;
    TestContextMenu *m_testContextMenu;
    TestCollection m_testCollection;
    TestSettings m_testSettings;
    QMenu *m_testMenu;
    static int m_refCount;
    TestViewItem *m_curItem;
    QTimer m_initTimer;
    TestCode::TestTypes m_hiddenTestTypes;
};

#endif
