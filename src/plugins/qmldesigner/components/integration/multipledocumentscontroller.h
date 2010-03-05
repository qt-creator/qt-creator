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

#ifndef MultipleDocumentsController_h
#define MultipleDocumentsController_h

#include <rewriterview.h>

#include <QWeakPointer>
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QWeakPointer>

#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QTabWidget>
#include <QtGui/QUndoGroup>

class QDeclarativeError;

namespace QmlDesigner {

class DesignDocumentController;

class MultipleDocumentsController : public QObject
{
    Q_OBJECT

public:
    MultipleDocumentsController(QWidget* parent = 0); // a bit of a hack
    ~MultipleDocumentsController();

    QTabWidget* tabWidget() const;

    QAction* undoAction();
    QAction* redoAction();
#ifdef ENABLE_TEXT_VIEW
    QAction* showFormAction(QMenu* parent);
    QAction* showTextAction(QMenu* parent);
#endif // ENABLE_TEXT_VIEW
    void open(DesignDocumentController* documentController);
    void closeAll(bool quitWhenAllEditorsClosed);
    unsigned activeDocumentCount() const { return m_documentControllers.size(); }

signals:
    void activeDocumentChanged(DesignDocumentController* documentController);
    void documentClosed(DesignDocumentController* documentController);
    void documentCountChanged(unsigned newDocumentCount);

    void previewVisibilityChanged(bool);
    void previewWithDebugVisibilityChanged(bool);

public slots:
    void doSave();
    void doSaveAs();
    void doPreview(bool visible);
    void doPreviewWithDebug(bool visible);
    void doDelete();
    void doCopy();
    void doCut();
    void doPaste();

    void documentDisplayNameChanged(const QString &fileName);
    void documentDirtyStateChanged(bool newState);
    void documentUndoAvailable(bool isAvailable);
    void documentRedoAvailable(bool isAvailable);
    void documentPreviewVisibilityChanged(bool visible);
    void documentPreviewWithDebugVisibilityChanged(bool visible);

protected slots:
    void currentTabChanged(int newCurrentTab);
    void tabCloseRequested(int tabIndex);
    void designDocumentClosed();
    void designDocumentError(const QList<RewriterView::Error> &errors);

private:
    DesignDocumentController* findActiveDocument(QWidget* documentWidget);
    void rewireDocumentActions(DesignDocumentController* oldActiveDocument, DesignDocumentController* newActiveDocument);

private:
    QWeakPointer<QTabWidget> m_tabWidget;
    QWeakPointer<QAction> m_undoAction;
    QWeakPointer<QAction> m_redoAction;
#ifdef ENABLE_TEXT_VIEW
    QWeakPointer<QAction> m_showTextAction;
    QWeakPointer<QAction> m_showFormAction;
#endif // ENABLE_TEXT_VIEW

    QList<QWeakPointer<DesignDocumentController> > m_documentControllers;
};

} // namespace QmlDesigner

#endif // MultipleDocumentsController_h
