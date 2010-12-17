/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QList>
#include <QtCore/QString>

#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QMenu>
#include <QtGui/QStackedWidget>

#include "rewriterview.h"

class QmlError;
class WelcomeScreen;

namespace QmlDesigner {
    class ItemLibraryController;
    class NavigatorController;
    class StatesEditorController;
    class AllPropertiesViewController;
    class MultipleDocumentsController;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);
    ~MainWindow();

    int documentCount() const;

public slots:
    void documentCountChanged(unsigned newCount);
    void openFile(const QString &fileName);
    void newFile(const QString &fileTemplate);

    void doNew();
    void doOpen();
    void doQuit();
    void doAbout();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void showRecentFilesMenu();
    void styleActionTriggered();

private:
    void createMenus();
    void createMainArea();
    void createMainEditArea();
    void createMainStyleArea();
    void createStatusBar();
    void updateActions();
    void updateMainArea();

    void newFile(const QByteArray &templateContents);

    QStringList recentFiles() const;
    void addRecentFile(const QString &path);

    QString serializeErrors(const QList<QmlDesigner::RewriterView::Error> &errors);

private:
    bool m_shutdown;
    QString m_lastPath;
    QList<QAction*> m_documentActions;
    unsigned int m_documentCount;

    QMenuBar* m_menuBar;

    QmlDesigner::ItemLibraryController* m_itemLibraryController;
    QmlDesigner::NavigatorController* m_navigatorController;
    QmlDesigner::StatesEditorController* m_statesEditorController;
    QmlDesigner::AllPropertiesViewController* m_propertiesViewController;
    QmlDesigner::MultipleDocumentsController* m_multipleDocumentsController;
    QAction* m_previewAction;
    QAction* m_previewWithDebugAction;
    QAction* m_showNewLookPropertiesAction;
    QAction* m_showTraditionalPropertiesAction;
    QMenu* m_recentFilesMenu;

    QStackedWidget *m_mainArea;
    WelcomeScreen *m_welcomeScreen;
};

#endif // MAINWINDOW_H
