/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "proeditor.h"
#include "proitems.h"
#include "proeditormodel.h"
#include "procommandmanager.h"
#include "proxml.h"

#include <QtGui/QMenu>
#include <QtGui/QKeyEvent>
#include <QtGui/QClipboard>

using namespace Qt4ProjectManager::Internal;

ProEditor::ProEditor(QWidget *parent, bool shortcuts)
    : QWidget(parent)
{
    m_shortcuts = shortcuts;
    m_advanced = false;
    setupUi(this);

    m_setFocusToListView = true;
    m_blockSelectionSignal = false;
    m_cutAction = new QAction(tr("Cut"), this);
    m_copyAction = new QAction(tr("Copy"), this);
    m_pasteAction = new QAction(tr("Paste"), this);
}

ProEditor::~ProEditor()
{
}

void ProEditor::initialize(ProEditorModel *model, ProItemInfoManager *infomanager)
{
    m_model = model;
    m_infomanager = infomanager;
    initialize();
}

ProScopeFilter *ProEditor::filterModel() const
{
    return m_filter;
}

void ProEditor::selectScope(const QModelIndex &scope)
{
    m_setFocusToListView = false;
    QModelIndex parent = m_filter->mapToSource(scope);
    m_editListView->setRootIndex(parent);
    m_editListView->setCurrentIndex(m_model->index(0,0,parent));
    m_setFocusToListView = true;
}

void ProEditor::initialize()
{
    m_model->setInfoManager(m_infomanager);
    m_filter = new ProScopeFilter(this);
    m_filter->setSourceModel(m_model);

    m_contextMenu = new QMenu(this);

    if (m_shortcuts) {
        m_cutAction->setShortcut(QKeySequence(tr("Ctrl+X")));
        m_copyAction->setShortcut(QKeySequence(tr("Ctrl+C")));
        m_pasteAction->setShortcut(QKeySequence(tr("Ctrl+V")));
        m_editListView->installEventFilter(this);
    }

    m_contextMenu->addAction(m_cutAction);
    m_contextMenu->addAction(m_copyAction);
    m_contextMenu->addAction(m_pasteAction);

    QMenu *addMenu = new QMenu(m_addToolButton);
    m_addVariable = addMenu->addAction(tr("Add Variable"), this, SLOT(addVariable()));
    m_addScope = addMenu->addAction(tr("Add Scope"), this, SLOT(addScope()));
    m_addBlock = addMenu->addAction(tr("Add Block"), this, SLOT(addBlock()));
    m_addToolButton->setMenu(addMenu);
    m_addToolButton->setPopupMode(QToolButton::InstantPopup);

    m_editListView->setModel(m_model);
    m_editListView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_editListView, SIGNAL(customContextMenuRequested(const QPoint &)),
        this, SLOT(showContextMenu(const QPoint &)));

    connect(m_editListView->selectionModel(),
        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(updateState()));

    connect(m_moveUpToolButton, SIGNAL(clicked()),
        this, SLOT(moveUp()));
    connect(m_moveDownToolButton, SIGNAL(clicked()),
        this, SLOT(moveDown()));
    connect(m_removeToolButton, SIGNAL(clicked()),
        this, SLOT(remove()));
    connect(m_cutAction, SIGNAL(triggered()),
        this, SLOT(cut()));
    connect(m_copyAction, SIGNAL(triggered()),
        this, SLOT(copy()));
    connect(m_pasteAction, SIGNAL(triggered()),
        this, SLOT(paste()));

    updatePasteAction();
}

bool ProEditor::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *k = static_cast<QKeyEvent*>(event);
        if (k->modifiers() == Qt::ControlModifier) {
            switch (k->key()) {
                case Qt::Key_X:
                    cut(); return true;
                case Qt::Key_C:
                    copy(); return true;
                case Qt::Key_V:
                    paste(); return true;
            }
        }
    } else if (event->type() == QEvent::FocusIn) {
        updateActions(true);
    } else if (event->type() == QEvent::FocusOut) {
        updateActions(false);
    }

    return false;
}

void ProEditor::showContextMenu(const QPoint &pos)
{
    updatePasteAction();
    m_contextMenu->popup(m_editListView->viewport()->mapToGlobal(pos));
}

void ProEditor::updatePasteAction()
{
    bool pasteEnabled = false;

    const QMimeData *data = QApplication::clipboard()->mimeData();
    if (data && data->hasFormat(QLatin1String("application/x-problock")))
        pasteEnabled = true;

    m_pasteAction->setEnabled(pasteEnabled);
}

void ProEditor::updateActions(bool focus)
{
    bool copyEnabled = false;

    if (focus)
        copyEnabled = m_editListView->currentIndex().isValid();

    m_cutAction->setEnabled(copyEnabled);
    m_copyAction->setEnabled(copyEnabled);
}

void ProEditor::updateState()
{
    bool addEnabled = false;
    bool removeEnabled = false;
    bool upEnabled = false;
    bool downEnabled = false;

    QModelIndex parent = m_editListView->rootIndex();
    ProBlock *scope = m_model->proBlock(parent);

    if (scope) {
        addEnabled = true;
        QModelIndex index = m_editListView->currentIndex();
        if (index.isValid()) {
            removeEnabled = true;
            int count = m_model->rowCount(parent);
            int row = index.row();
            if (row > 0)
                upEnabled = true;
            if (row < (count - 1))
                downEnabled = true;
        }
    }

    if (!m_blockSelectionSignal) {
        emit itemSelected(m_editListView->currentIndex());
        if (m_setFocusToListView)
            m_editListView->setFocus(Qt::OtherFocusReason);
    }

    updateActions(m_editListView->hasFocus());

    m_addToolButton->setEnabled(addEnabled);
    m_removeToolButton->setEnabled(removeEnabled);
    m_moveUpToolButton->setEnabled(upEnabled);
    m_moveDownToolButton->setEnabled(downEnabled);
}

void ProEditor::moveUp()
{
    m_editListView->setFocus(Qt::OtherFocusReason);
    QModelIndex index = m_editListView->currentIndex();
    QModelIndex parent = index.parent();
    int row = index.row() - 1;

    m_blockSelectionSignal = true;
    m_model->moveItem(index, row);
    m_blockSelectionSignal = false;

    index = m_model->index(row, 0, parent);
    m_editListView->setCurrentIndex(index);
}

void ProEditor::moveDown()
{
    m_editListView->setFocus(Qt::OtherFocusReason);
    QModelIndex index = m_editListView->currentIndex();
    QModelIndex parent = index.parent();
    int row = index.row() + 1;

    m_blockSelectionSignal = true;
    m_model->moveItem(index, row);
    m_blockSelectionSignal = false;

    index = m_model->index(row, 0, parent);
    m_editListView->setCurrentIndex(index);
}

void ProEditor::remove()
{
    m_editListView->setFocus(Qt::OtherFocusReason);
    m_model->removeItem(m_editListView->currentIndex());
    updateState();
}

void ProEditor::cut()
{
    QModelIndex index = m_editListView->currentIndex();
    if (!index.isValid())
        return;

    if (ProItem *item = m_model->proItem(index)) {
        m_editListView->setFocus(Qt::OtherFocusReason);
        m_model->removeItem(index);

        QMimeData *data = new QMimeData();
        QString xml = ProXmlParser::itemToString(item);
        if (item->kind() == ProItem::ValueKind)
            data->setData(QLatin1String("application/x-provalue"), xml.toUtf8());
        else
            data->setData(QLatin1String("application/x-problock"), xml.toUtf8());
        QApplication::clipboard()->setMimeData(data);
    }
}

void ProEditor::copy()
{
    QModelIndex index = m_editListView->currentIndex();
    if (!index.isValid())
        return;

    if (ProItem *item = m_model->proItem(index)) {
        m_editListView->setFocus(Qt::OtherFocusReason);
        QMimeData *data = new QMimeData();
        QString xml = ProXmlParser::itemToString(item);
        if (item->kind() == ProItem::ValueKind)
            data->setData(QLatin1String("application/x-provalue"), xml.toUtf8());
        else
            data->setData(QLatin1String("application/x-problock"), xml.toUtf8());
        QApplication::clipboard()->setMimeData(data);
    }
}

void ProEditor::paste()
{
    if (const QMimeData *data = QApplication::clipboard()->mimeData()) {
        m_editListView->setFocus(Qt::OtherFocusReason);
        QModelIndex parent = m_editListView->rootIndex();
        ProBlock *block = m_model->proBlock(parent);
        if (!block)
            return;

        QString xml;
        if (data->hasFormat(QLatin1String("application/x-provalue"))) {
            xml = QString::fromUtf8(data->data(QLatin1String("application/x-provalue")));
        } else if (data->hasFormat(QLatin1String("application/x-problock"))) {
            xml = QString::fromUtf8(data->data(QLatin1String("application/x-problock")));
        }

        if (ProItem *item = ProXmlParser::stringToItem(xml)) {
            QModelIndex parent = m_editListView->rootIndex();
            int row = m_model->rowCount(parent);
            m_model->insertItem(item, row, parent);
            m_editListView->setCurrentIndex(m_model->index(row,0,parent));
        }
    }
}

void ProEditor::addVariable()
{
    QModelIndex parent = m_editListView->rootIndex();
    if (ProBlock *pblock = m_model->proBlock(parent)) {
        m_editListView->setFocus(Qt::OtherFocusReason);
        int row = m_model->rowCount(parent);

        QString defid("...");
        ProVariable::VariableOperator op = ProVariable::SetOperator;
        QList<ProVariableInfo *> vars = m_infomanager->variables();
        if (!vars.isEmpty()) {
            defid = vars.first()->id();
            op = vars.first()->defaultOperator();
        }

        ProVariable *var = new ProVariable(defid, pblock);
        var->setVariableOperator(op);

        m_model->insertItem(var, row, parent);
        m_editListView->setCurrentIndex(m_model->index(row,0,parent));
    }
}

void ProEditor::addScope()
{
    QModelIndex parent = m_editListView->rootIndex();
    if (ProBlock *pblock = m_model->proBlock(parent)) {
        m_editListView->setFocus(Qt::OtherFocusReason);
        int row = m_model->rowCount(parent);
        ProBlock *scope = new ProBlock(pblock);
        scope->setBlockKind(ProBlock::ScopeKind);
        ProBlock *scopecontents = new ProBlock(scope);
        scopecontents->setBlockKind(ProBlock::ScopeContentsKind);

        QString defid("...");
        QList<ProScopeInfo *> vars = m_infomanager->scopes();
        if (!vars.isEmpty())
            defid = vars.first()->id();

        scope->setItems(QList<ProItem *>() << new ProCondition(defid) << scopecontents);
        m_model->insertItem(scope, row, parent);
        m_editListView->setCurrentIndex(m_model->index(row,0,parent));
    }
}

void ProEditor::addBlock()
{
    QModelIndex parent = m_editListView->rootIndex();
    if (ProBlock *pblock = m_model->proBlock(parent)) {
        m_editListView->setFocus(Qt::OtherFocusReason);
        int row = m_model->rowCount(parent);
        ProBlock *block = new ProBlock(pblock);
        block->setBlockKind(ProBlock::NormalKind);
        block->setItems(QList<ProItem *>() << new ProFunction("..."));
        m_model->insertItem(block, row, parent);
        m_editListView->setCurrentIndex(m_model->index(row,0,parent));
    }
}
