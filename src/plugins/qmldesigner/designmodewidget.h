/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DESIGNMODEWIDGET_H
#define DESIGNMODEWIDGET_H

#include <coreplugin/minisplitter.h>
#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/itexteditor.h>

#include <integrationcore.h>

#include <allpropertiesbox.h>
#include <designdocumentcontroller.h>
#include <itemlibrary.h>
#include <navigatorwidget.h>
#include <stateseditorwidget.h>
#include <modelnode.h>


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
    class OpenEditorsModel;
    class EditorToolBar;
}

namespace QmlDesigner {

namespace Internal {

class DesignMode;
class DocumentWidget;
class DesignModeWidget;

class DocumentWarningWidget : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY(DocumentWarningWidget)
public:
    explicit DocumentWarningWidget(DocumentWidget *documentWidget, QWidget *parent = 0);

    void setError(const RewriterView::Error &error);

private slots:
    void goToError();

private:
    QLabel *m_errorMessage;
    QLabel *m_goToError;
    DocumentWidget *m_documentWidget;
    RewriterView::Error m_error;
};

class DocumentWidget : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(DocumentWidget)
public:
    explicit DocumentWidget(TextEditor::ITextEditor *textEditor, QPlainTextEdit *textEdit, QmlDesigner::DesignDocumentController *document, DesignModeWidget *mainWidget);
    ~DocumentWidget();

    QmlDesigner::DesignDocumentController *document() const;
    TextEditor::ITextEditor *textEditor() const;

    void setAutoSynchronization(bool sync);

    void readSettings();
    void saveSettings();

protected:
    void resizeEvent(QResizeEvent *event);

private slots:
    void enable();
    void disable(const QList<RewriterView::Error> &errors);
    void updateErrorStatus(const QList<RewriterView::Error> &errors);

private:
    void setup();
    bool isInNodeDefinition(int nodeOffset, int nodeLength, int cursorPos) const;
    QmlDesigner::ModelNode nodeForPosition(int cursorPos) const;

    TextEditor::ITextEditor *m_textEditor;
    QWeakPointer<QPlainTextEdit> m_textBuffer;
    QmlDesigner::DesignDocumentController *m_document;
    DesignModeWidget *m_mainWidget;
    QSplitter *m_mainSplitter;
    Core::SideBar *m_leftSideBar;
    Core::SideBar *m_rightSideBar;

    QToolBar *m_designToolBar;
    Core::EditorToolBar *m_fakeToolBar;

    bool m_isDisabled;
    DocumentWarningWidget *m_warningWidget;
};

class DesignModeWidget : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(DesignModeWidget)
public:
    explicit DesignModeWidget(QWidget *parent = 0);
    ~DesignModeWidget();

//    void syncWithTextEdit(bool sync);

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

private slots:
    void undo();
    void redo();
    void deleteSelected();
    void cutSelected();
    void copySelected();
    void paste();
    void selectAll();

    void undoAvailable(bool isAvailable);
    void redoAvailable(bool isAvailable);

private:
    void setCurrentDocumentWidget(DocumentWidget *newDocumentWidget);

    QStackedWidget *m_documentWidgetStack;
    QHash<QPlainTextEdit*,DocumentWidget*> m_documentHash;
    DocumentWidget *m_currentDocumentWidget;
    QPlainTextEdit *m_currentTextEdit;

    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_deleteAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_selectAllAction;

    bool m_syncWithTextEdit;
};

} // namespace Internal
} // namespace Designer

#endif // DESIGNMODEWIDGET_H
