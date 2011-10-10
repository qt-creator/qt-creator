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

#ifndef TESTCONTEXTMENU_H
#define TESTCONTEXTMENU_H

#include "testsettings.h"

namespace Core {
class IEditor;
}

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

class TestContextMenuPrivate;

class TestContextMenu : public QObject
{
    Q_OBJECT
public:
    TestContextMenu(QObject *widget);
    ~TestContextMenu();
    void init(QMenu *testMenu, int mode, QObject *widget);
    void languageChange();
    void updateActions(bool testVisible, bool testBusy, bool testStopped);
    void updateToggleAction(const QString &testName);
    void updateSingleTestAction(const QString &testName);

signals:
    void selectAllTests();
    void deselectAllTests();
    void selectAllManualTests();
    void toggleSelection();

private:
    static TestContextMenuPrivate *m_instance;
    static int m_refCount;
};

class TestContextMenuPrivate : public QObject
{
    Q_OBJECT
public:
    TestContextMenuPrivate(QObject *widget);
    ~TestContextMenuPrivate();

    void init(QMenu *testMenu, int mode, QObject *widget);
    void languageChange();
    void updateActions(bool testVisible, bool testBusy, bool testStopped);
    void updateToggleAction(const QString &testName);
    void updateSingleTestAction(const QString &testName);
    void enableIncludeFile(const QString &fileName);

    QAction *m_testInsertUnitOrSystemTestAction;
    QAction *m_testRunAction;
    QAction *m_testRunAsManualAction;
    QAction *m_testDebugAction;
    QAction *m_testStopTestingAction;

    QAction *m_testLearnAction;
    QAction *m_testLearnAllAction;

    QAction *m_testLocalSettingsAction;
    QAction *m_testToggleCurrentSelectAction;
    QAction *m_testSelectAllTestsAction;
    QAction *m_testSelectAllManualTestsAction;
    QAction *m_testDeselectAllTestsAction;
    QAction *m_testGroupsAction;
    QAction *m_testRescanAction;
    QAction *m_testOpenIncludeFileAction;

    QAction *m_editorInsertTestFunctionAction;
    QAction *m_editorRunSingleTestAction;
    QAction *m_editorStopTestingAction;

signals:
    void openIncludeFile(const QString &filename);

private slots:
    void onOpenIncludeFile();
    void editorChanged(Core::IEditor *iface);
    void onLearnChanged();
    void onLearnAllChanged();

private:
    QString m_includeFile;
    TestSettings m_testSettings;
};

#endif // TESTCONTEXTMENU_H
