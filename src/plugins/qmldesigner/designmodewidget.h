/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DESIGNMODEWIDGET_H
#define DESIGNMODEWIDGET_H

#include <coreplugin/minisplitter.h>
#include <coreplugin/editormanager/ieditor.h>
#include <utils/faketooltip.h>
#include <texteditor/itexteditor.h>

#include <integrationcore.h>

#include <allpropertiesbox.h>
#include <designdocumentcontroller.h>
#include <itemlibraryview.h>
#include <navigatorwidget.h>
#include <navigatorview.h>
#include <stateseditorview.h>
#include <modelnode.h>
#include <formeditorview.h>

#include <QWeakPointer>
#include <QDeclarativeError>
#include <QtCore/QHash>
#include <QtGui/QWidget>
#include <QtGui/QToolBar>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>

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

class DocumentWarningWidget : public  Utils::FakeToolTip
{
    Q_OBJECT
    Q_DISABLE_COPY(DocumentWarningWidget)
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
    Q_DISABLE_COPY(DesignModeWidget)
public:
    explicit DesignModeWidget(QWidget *parent = 0);
    ~DesignModeWidget();

    void showEditor(Core::IEditor *editor);
    void closeEditors(const QList<Core::IEditor*> editors);
    QString contextHelpId() const;

    QAction *undoAction() const;
    QAction *redoAction() const;
    QAction *deleteAction() const;
    QAction *cutAction() const;
    QAction *copyAction() const;
    QAction *pasteAction() const;
    QAction *selectAllAction() const;
    QAction *hideSidebarsAction() const;
    QAction *toggleLeftSidebarAction() const;
    QAction *toggleRightSidebarAction() const;
    QAction *restoreDefaultViewAction() const;

    void readSettings();
    void saveSettings();
    void setAutoSynchronization(bool sync);

    TextEditor::ITextEditor *textEditor() const {return m_textEditor; }

private slots:
    void undo();
    void redo();
    void deleteSelected();
    void cutSelected();
    void copySelected();
    void paste();
    void selectAll();
    void closeCurrentEditor();
    void toggleSidebars();
    void toggleLeftSidebar();
    void toggleRightSidebar();
    void restoreDefaultView();
    void undoAvailable(bool isAvailable);
    void redoAvailable(bool isAvailable);

    void enable();
    void disable(const QList<RewriterView::Error> &errors);
    void updateErrorStatus(const QList<RewriterView::Error> &errors);
    void updateAvailableSidebarItemsLeft();
    void updateAvailableSidebarItemsRight();

    void deleteSidebarWidgets();

protected:
    void resizeEvent(QResizeEvent *event);

private:
    DesignDocumentController *currentDesignDocumentController() const {return m_currentDesignDocumentController.data(); }
    void setCurrentDocument(DesignDocumentController *newDesignDocumentController);

    //QStackedWidget *m_documentWidgetStack;
    QHash<QPlainTextEdit*,QWeakPointer<DesignDocumentController> > m_documentHash;
    QWeakPointer<DesignDocumentController> m_currentDesignDocumentController;
    QWeakPointer<QPlainTextEdit> m_currentTextEdit;

    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_deleteAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_selectAllAction;
    QAction *m_hideSidebarsAction;
    QAction *m_restoreDefaultViewAction;
    QAction *m_toggleLeftSidebarAction;
    QAction *m_toggleRightSidebarAction;

    QWeakPointer<ItemLibraryView> m_itemLibraryView;
    QWeakPointer<NavigatorView> m_navigator;
    QWeakPointer<AllPropertiesBox> m_allPropertiesBox;
    QWeakPointer<StatesEditorView> m_statesEditorView;
    QWeakPointer<FormEditorView> m_formEditorView;
    QWeakPointer<NodeInstanceView> m_nodeInstanceView;

    bool m_syncWithTextEdit;

    void setup();
    bool isInNodeDefinition(int nodeOffset, int nodeLength, int cursorPos) const;
    QmlDesigner::ModelNode nodeForPosition(int cursorPos) const;

    TextEditor::ITextEditor *m_textEditor;

    QSplitter *m_mainSplitter;
    Core::SideBar *m_leftSideBar;
    Core::SideBar *m_rightSideBar;
    Core::EditorToolBar *m_fakeToolBar;
    Core::OutputPanePlaceHolder *m_outputPanePlaceholder;
    Core::MiniSplitter *m_outputPlaceholderSplitter;
    QList<Core::SideBarItem*> m_sideBarItems;
    bool m_isDisabled;
    bool m_showSidebars;

    enum InitializeStatus { NotInitialized, Initializing, Initialized };
    InitializeStatus m_initStatus;

    DocumentWarningWidget *m_warningWidget;
};

} // namespace Internal
} // namespace Designer

#endif // DESIGNMODEWIDGET_H
