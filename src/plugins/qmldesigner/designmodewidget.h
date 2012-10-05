/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <designdocumentcontroller.h>
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
    QAction *goIntoComponentAction() const;

    void readSettings();
    void saveSettings();
    void setAutoSynchronization(bool sync);

    TextEditor::ITextEditor *textEditor() const {return m_textEditor.data(); }

    static DesignModeWidget *instance();
    DesignDocumentController *currentDesignDocumentController() const {return m_currentDesignDocumentController.data(); }

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
    void goIntoComponent();

    void enable();
    void disable(const QList<RewriterView::Error> &errors);
    void updateErrorStatus(const QList<RewriterView::Error> &errors);
    void updateAvailableSidebarItemsLeft();
    void updateAvailableSidebarItemsRight();

    void deleteSidebarWidgets();
    void qmlPuppetCrashed();

    void onGoBackClicked();
    void onGoForwardClicked();

    void onCrumblePathElementClicked(const QVariant &data);

protected:
    void resizeEvent(QResizeEvent *event);

private:
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
    QAction *m_goIntoComponentAction;

    QWeakPointer<ItemLibraryView> m_itemLibraryView;
    QWeakPointer<NavigatorView> m_navigatorView;
    QWeakPointer<PropertyEditor> m_propertyEditorView;
    QWeakPointer<StatesEditorView> m_statesEditorView;
    QWeakPointer<FormEditorView> m_formEditorView;
    QWeakPointer<ComponentView> m_componentView;
    QWeakPointer<NodeInstanceView> m_nodeInstanceView;

    bool m_syncWithTextEdit;

    void setup();
    bool isInNodeDefinition(int nodeOffset, int nodeLength, int cursorPos) const;
    QmlDesigner::ModelNode nodeForPosition(int cursorPos) const;
    void setupNavigatorHistory();
    void addNavigatorHistoryEntry(const QString &fileName);

    QWeakPointer<TextEditor::ITextEditor> m_textEditor;

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
    QStringList m_navigatorHistory;
    int m_navigatorHistoryCounter;
    bool m_keepNavigatorHistory;

    static DesignModeWidget *s_instance;
};

} // namespace Internal
} // namespace Designer

#endif // DESIGNMODEWIDGET_H
