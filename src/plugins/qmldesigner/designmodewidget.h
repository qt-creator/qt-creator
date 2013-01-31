/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DESIGNMODEWIDGET_H
#define DESIGNMODEWIDGET_H

#include <coreplugin/minisplitter.h>
#include <coreplugin/editormanager/ieditor.h>
#include <utils/faketooltip.h>
#include <texteditor/itexteditor.h>

#include <designdocument.h>
#include <itemlibraryview.h>
#include <navigatorwidget.h>
#include <navigatorview.h>
#include <stateseditorview.h>
#include <componentview.h>
#include <modelnode.h>
#include <formeditorview.h>
#include <propertyeditor.h>

#include <QWeakPointer>
#include <QDeclarativeError>
#include <QHash>
#include <QWidget>
#include <QToolBar>
#include <QComboBox>
#include <QLabel>

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QTabWidget;
class QVBoxLayout;
class QToolButton;
QT_END_NAMESPACE

namespace Core {
    class SideBar;
    class SideBarItem;
    class OpenEditorsModel;
    class EditorToolBar;
    class OutputPanePlaceHolder;
}

namespace QmlDesigner {

class ItemLibraryWidget;

namespace Internal {

class DesignMode;
class DocumentWidget;
class DesignModeWidget;

class DocumentWarningWidget : public Utils::FakeToolTip
{
    Q_OBJECT

public:
    explicit DocumentWarningWidget(DesignModeWidget *parent = 0);

    void setError(const RewriterView::Error &error);

private slots:
    void goToError();

private:
    QLabel *m_errorMessage;
    QLabel *m_goToError;
    RewriterView::Error m_error;
    DesignModeWidget *m_designModeWidget;
};

class DesignModeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DesignModeWidget(QWidget *parent = 0);

    void showEditor(Core::IEditor *editor);
    void closeEditors(const QList<Core::IEditor*> editors);
    QString contextHelpId() const;

    void initialize();


    void readSettings();
    void saveSettings();

    TextEditor::ITextEditor *textEditor() const;

    DesignDocument *currentDesignDocument() const;
    ViewManager &viewManager();

    void enableWidgets();
    void disableWidgets();
    void showErrorMessage(const QList<RewriterView::Error> &errors);

public slots:
    void updateErrorStatus(const QList<RewriterView::Error> &errors);
    void restoreDefaultView();
    void toggleSidebars();
    void toggleLeftSidebar();
    void toggleRightSidebar();

private slots:
    void updateAvailableSidebarItemsLeft();
    void updateAvailableSidebarItemsRight();

    void deleteSidebarWidgets();
    void qmlPuppetCrashed();

    void onGoBackClicked();
    void onGoForwardClicked();

protected:
    void resizeEvent(QResizeEvent *event);

private: // functions
    enum InitializeStatus { NotInitialized, Initializing, Initialized };

    void setCurrentDesignDocument(DesignDocument *newDesignDocument);
    void setup();
    bool isInNodeDefinition(int nodeOffset, int nodeLength, int cursorPos) const;
    QmlDesigner::ModelNode nodeForPosition(int cursorPos) const;
    void setupNavigatorHistory(Core::IEditor *editor);
    void addNavigatorHistoryEntry(const QString &fileName);

private: // variables
    QSplitter *m_mainSplitter;
    Core::SideBar *m_leftSideBar;
    Core::SideBar *m_rightSideBar;
    Core::EditorToolBar *m_fakeToolBar;
    Core::OutputPanePlaceHolder *m_outputPanePlaceholder;
    Core::MiniSplitter *m_outputPlaceholderSplitter;
    QList<Core::SideBarItem*> m_sideBarItems;
    bool m_isDisabled;
    bool m_showSidebars;

    InitializeStatus m_initStatus;

    DocumentWarningWidget *m_warningWidget;
    QStringList m_navigatorHistory;
    int m_navigatorHistoryCounter;
    bool m_keepNavigatorHistory;
};

} // namespace Internal
} // namespace Designer

#endif // DESIGNMODEWIDGET_H
