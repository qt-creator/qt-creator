/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
