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

#include "multipledocumentscontroller.h"

#include <QtCore/QFileInfo>
#include <QtGui/QTextEdit>
#include <QtGui/QMessageBox>

#include <QmlError>

#include <model.h>
#include "designdocumentcontroller.h"
#include "documentcloser.h"


namespace QmlDesigner {

static QString createTitle(const QString& fileName)
{
    return QFileInfo(fileName).baseName();
}

MultipleDocumentsController::MultipleDocumentsController(QWidget* parent):
        QObject(parent),
        m_tabWidget(new QTabWidget),
        m_undoAction(new QAction(tr("&Undo"), this)),
        m_redoAction(new QAction(tr("&Redo"), this))
{
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);

    connect(tabWidget(), SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
    connect(tabWidget(), SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
}

MultipleDocumentsController::~MultipleDocumentsController()
{
}

QTabWidget *MultipleDocumentsController::tabWidget() const
{
    return m_tabWidget.data();
}

DesignDocumentController* MultipleDocumentsController::findActiveDocument(QWidget* documentWidget)
{
    foreach (const QWeakPointer<DesignDocumentController> &controller, m_documentControllers)
        if (controller->documentWidget() == documentWidget)
            return controller.data();

    return 0;
}

void MultipleDocumentsController::rewireDocumentActions(DesignDocumentController* oldActiveDocument, DesignDocumentController* newActiveDocument)
{
    if (oldActiveDocument) {
        disconnect(undoAction(), SIGNAL(triggered()), oldActiveDocument, SLOT(undo()));
        disconnect(redoAction(), SIGNAL(triggered()), oldActiveDocument, SLOT(redo()));
#ifdef ENABLE_TEXT_VIEW
        disconnect(m_showTextAction.data(), SIGNAL(triggered()), oldActiveDocument, SLOT(showText()));
        disconnect(m_showFormAction.data(), SIGNAL(triggered()), oldActiveDocument, SLOT(showForm()));
#endif // ENABLE_TEXT_VIEW
    }

    if (newActiveDocument) {
        connect(undoAction(), SIGNAL(triggered()), newActiveDocument, SLOT(undo()));
        connect(redoAction(), SIGNAL(triggered()), newActiveDocument, SLOT(redo()));
#ifdef ENABLE_TEXT_VIEW
        connect(m_showTextAction.data(), SIGNAL(triggered()), newActiveDocument, SLOT(showText()));
        connect(m_showFormAction.data(), SIGNAL(triggered()), newActiveDocument, SLOT(showForm()));
        if (m_showTextAction->isChecked())
            newActiveDocument->showText();
        else
            newActiveDocument->showForm();
#endif // ENABLE_TEXT_VIEW
    }
}

void MultipleDocumentsController::currentTabChanged(int newCurrentTab)
{
    DesignDocumentController* oldActiveDocument = 0;
    if (!m_documentControllers.isEmpty()) {
        oldActiveDocument = m_documentControllers.first().data();
    }

    if (newCurrentTab == -1) {
        emit activeDocumentChanged(0);
        if (oldActiveDocument && oldActiveDocument->previewVisible())
            emit previewVisibilityChanged(false);
        if (oldActiveDocument && oldActiveDocument->previewWithDebugVisible())
            emit previewWithDebugVisibilityChanged(false);

        if (oldActiveDocument)
            rewireDocumentActions(oldActiveDocument, 0);
    } else {
        DesignDocumentController* newActiveDocument = findActiveDocument(m_tabWidget->currentWidget());
        m_undoAction->setEnabled(newActiveDocument->isUndoAvailable());
        m_redoAction->setEnabled(newActiveDocument->isRedoAvailable());

        rewireDocumentActions(oldActiveDocument, newActiveDocument);

        m_documentControllers.removeOne(newActiveDocument);
        m_documentControllers.prepend(newActiveDocument);
        emit activeDocumentChanged(newActiveDocument);
        if (oldActiveDocument && oldActiveDocument->previewVisible())
            emit previewVisibilityChanged(newActiveDocument->previewVisible());
        if (oldActiveDocument && oldActiveDocument->previewWithDebugVisible())
            emit previewWithDebugVisibilityChanged(newActiveDocument->previewWithDebugVisible());
    }
}

void MultipleDocumentsController::tabCloseRequested(int tabIndex)
{
    DesignDocumentController* controller = findActiveDocument(m_tabWidget->widget(tabIndex));

    if (controller)
        DocumentCloser::close(controller);
}

void MultipleDocumentsController::documentDisplayNameChanged(const QString &/*fileName*/)
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    Q_ASSERT(documentController);
    documentDirtyStateChanged(documentController->isDirty());
}

void MultipleDocumentsController::documentDirtyStateChanged(bool newState)
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    Q_ASSERT(documentController);

    int tabIdx = m_tabWidget->indexOf(documentController->documentWidget());

    if (newState)
        m_tabWidget->setTabText(tabIdx, tr("* %1").arg(createTitle(documentController->displayName())));
    else
        m_tabWidget->setTabText(tabIdx, createTitle(documentController->displayName()));
}

void MultipleDocumentsController::documentUndoAvailable(bool isAvailable)
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    Q_ASSERT(documentController);

    if (m_documentControllers.first().data() == documentController)
        m_undoAction->setEnabled(isAvailable);
}

void MultipleDocumentsController::documentRedoAvailable(bool isAvailable)
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    Q_ASSERT(documentController);

    if (m_documentControllers.first().data() == documentController)
        m_redoAction->setEnabled(isAvailable);
}

void MultipleDocumentsController::documentPreviewVisibilityChanged(bool visible)
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    Q_ASSERT(documentController);

    if (m_documentControllers.first().data() == documentController)
        emit previewVisibilityChanged(visible);
}

void MultipleDocumentsController::documentPreviewWithDebugVisibilityChanged(bool visible)
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    Q_ASSERT(documentController);

    if (m_documentControllers.first().data() == documentController)
        emit previewWithDebugVisibilityChanged(visible);
}

void MultipleDocumentsController::open(DesignDocumentController* documentController)
{
    m_documentControllers.append(documentController);

    int newTabIndex = m_tabWidget->addTab(documentController->documentWidget(), createTitle(documentController->displayName()));
    m_tabWidget->setCurrentIndex(newTabIndex);

    connect(documentController, SIGNAL(displayNameChanged(QString)),
            this, SLOT(documentDisplayNameChanged(QString)));
    connect(documentController, SIGNAL(dirtyStateChanged(bool)),
            this, SLOT(documentDirtyStateChanged(bool)));
    connect(documentController, SIGNAL(undoAvailable(bool)),
            this, SLOT(documentUndoAvailable(bool)));
    connect(documentController, SIGNAL(redoAvailable(bool)),
            this, SLOT(documentRedoAvailable(bool)));
    connect(documentController, SIGNAL(previewVisibilityChanged(bool)),
            this, SLOT(documentPreviewVisibilityChanged(bool)));
    connect(documentController, SIGNAL(previewWithDebugVisibilityChanged(bool)),
            this, SLOT(documentPreviewWithDebugVisibilityChanged(bool)));
    connect(documentController, SIGNAL(designDocumentClosed()),
            this, SLOT(designDocumentClosed()));
    connect(documentController, SIGNAL(qmlErrorsChanged(QList<RewriterView::Error>)),
            this, SLOT(designDocumentError(QList<RewriterView::Error>)));

    emit documentCountChanged(m_documentControllers.size());
}

void MultipleDocumentsController::designDocumentClosed()
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    Q_ASSERT(documentController);

    rewireDocumentActions(documentController, 0);

    m_tabWidget->removeTab(m_tabWidget->indexOf(documentController->documentWidget()));
    m_documentControllers.removeOne(documentController);
    emit documentClosed(documentController);
    emit documentCountChanged(m_documentControllers.size());
}

void MultipleDocumentsController::designDocumentError(const QList<RewriterView::Error> &error)
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    Q_ASSERT(documentController);

    if (error.isEmpty())
        return;

    QMessageBox msgBox(documentController->documentWidget());
    msgBox.setWindowFlags(Qt::Sheet | Qt::MSWindowsFixedSizeDialogHint);
    msgBox.setWindowTitle("Invalid qml");
    msgBox.setText(error.first().toString());
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
}

void MultipleDocumentsController::doSave()
{
    if (!m_documentControllers.isEmpty())
        m_documentControllers.first()->save(dynamic_cast<QWidget*>(parent()));
}

void MultipleDocumentsController::doSaveAs()
{
    QWidget* parentWindow = dynamic_cast<QWidget*>(parent());

    if (!m_documentControllers.isEmpty())
        m_documentControllers.first()->saveAs(parentWindow);
}

void MultipleDocumentsController::doPreview(bool visible)
{
    if (m_documentControllers.isEmpty())
        return;

    m_documentControllers.first()->togglePreview(visible);
}

void MultipleDocumentsController::doPreviewWithDebug(bool visible)
{
    if (m_documentControllers.isEmpty())
        return;

    m_documentControllers.first()->toggleWithDebugPreview(visible);
}

void MultipleDocumentsController::doDelete()
{
    m_documentControllers.first()->deleteSelected();
}

void MultipleDocumentsController::doCopy()
{
    m_documentControllers.first()->copySelected();
}

void MultipleDocumentsController::doCut()
{
    m_documentControllers.first()->cutSelected();
}

void MultipleDocumentsController::doPaste()
{
    m_documentControllers.first()->paste();
}

void MultipleDocumentsController::closeAll(bool quitWhenAllEditorsClosed)
{
    DocumentCloser::close(m_documentControllers, quitWhenAllEditorsClosed);
}

QAction* MultipleDocumentsController::undoAction()
{
    return m_undoAction.data();
}

QAction* MultipleDocumentsController::redoAction()
{
    return m_redoAction.data();
}

#ifdef ENABLE_TEXT_VIEW
QAction* MultipleDocumentsController::showFormAction(QMenu* parent)
{
    if (!m_showFormAction) {
        m_showFormAction = new QAction(tr("&Form"), parent);
        m_showFormAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
        m_showFormAction->setCheckable(true);
    }

    return m_showFormAction.data();
}
#endif // ENABLE_TEXT_VIEW

#ifdef ENABLE_TEXT_VIEW
QAction* MultipleDocumentsController::showTextAction(QMenu* parent)
{
    if (!m_showTextAction) {
        m_showTextAction = new QAction(tr("&Text"), parent);
        m_showTextAction->setShortcut(QKeySequence("Ctrl+Shift+T"));
        m_showTextAction->setCheckable(true);
    }

    return m_showTextAction.data();
}

#endif // ENABLE_TEXT_VIEW

} // namespace QmlDesigner

