/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "openeditorsview.h"
#include "editormanager.h"
#include "editorview.h"
#include "icore.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QTimer>
#include <QtGui/QMenu>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QStyleOption>
#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#ifdef Q_WS_MAC
#include <qmacstyle_mac.h>
#endif

Q_DECLARE_METATYPE(Core::IEditor*)

using namespace Core;
using namespace Core::Internal;

OpenEditorsWidget::OpenEditorsWidget()
{
    m_ui.setupUi(this);
    setWindowTitle(tr("Open Documents"));
    setWindowIcon(QIcon(Constants::ICON_DIR));
    setFocusProxy(m_ui.editorList);
    m_ui.editorList->header()->hide();
    m_ui.editorList->setIndentation(0);
    m_ui.editorList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_ui.editorList->setTextElideMode(Qt::ElideMiddle);
    m_ui.editorList->installEventFilter(this);
    m_ui.editorList->setFrameStyle(QFrame::NoFrame);
    EditorManager *em = EditorManager::instance();
    m_ui.editorList->setModel(em->openedEditorsModel());
    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateCurrentItem(Core::IEditor*)));
    connect(m_ui.editorList, SIGNAL(activated(QModelIndex)),
            this, SLOT(selectEditor(QModelIndex)));
}

OpenEditorsWidget::~OpenEditorsWidget()
{
    
}

void OpenEditorsWidget::updateCurrentItem(Core::IEditor *editor)
{
    if (!editor) {
        m_ui.editorList->clearSelection();
        return;
    }
    EditorManager *em = EditorManager::instance();
    m_ui.editorList->clearSelection(); //we are in extended selectionmode
    m_ui.editorList->setCurrentIndex(em->openedEditorsModel()->indexOf(editor));
    m_ui.editorList->scrollTo(m_ui.editorList->currentIndex());
}

void OpenEditorsWidget::selectEditor(const QModelIndex &index)
{
    IEditor *editor = index.data(Qt::UserRole).value<IEditor*>();
    EditorManager::instance()->activateEditor(editor);
}


void OpenEditorsWidget::selectEditor()
{
    selectEditor(m_ui.editorList->currentIndex());
}

void OpenEditorsWidget::closeEditors()
{
    /* ### TODO
    QList<IFile *> selectedFiles;
    QList<IEditor *> selectedEditors;
    foreach (QTreeWidgetItem *item, m_ui.editorList->selectedItems()) {
        selectedEditors.append(item->data(0, Qt::UserRole).value<IEditor *>());
        selectedFiles.append(item->data(0, Qt::UserRole).value<IEditor *>()->file());
    }
    ICore *core = ICore::instance();
    bool cancelled = false;
    core->fileManager()->saveModifiedFiles(selectedFiles, &cancelled);
    if (cancelled)
        return;
    core->editorManager()->
            closeEditors(selectedEditors);
    updateEditorList();
    */
}

void OpenEditorsWidget::closeAllEditors()
{
    m_ui.editorList->selectAll();
    closeEditors();
}


bool OpenEditorsWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_ui.editorList) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            switch (keyEvent->key()) {
                case Qt::Key_Return:
                    selectEditor(m_ui.editorList->currentIndex());
                    return true;
                case Qt::Key_Delete: //fall through
                case Qt::Key_Backspace:
                    if (keyEvent->modifiers() == Qt::NoModifier) {
                        closeEditors();
                        return true;
                    }
                    break;
                default:
                    break;
            }
        }
        if (event->type() == QEvent::ContextMenu) {
            QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent *>(event);
            QMenu menu;
            menu.addAction(tr("&Select"), this, SLOT(selectEditor()));
//todo      menu.addAction(tr("&Close"), this, SLOT(closeEditors()));
//todo      menu.addAction(tr("Close &All"), this, SLOT(closeAllEditors()));
            if (m_ui.editorList->selectionModel()->selectedIndexes().isEmpty())
                menu.setEnabled(false);
            menu.exec(contextMenuEvent->globalPos());
            return true;
        }
    } else if (obj == m_widget) {
        if (event->type() == QEvent::FocusIn) {
            QFocusEvent *e = static_cast<QFocusEvent *>(event);
            if (e->reason() != Qt::MouseFocusReason) {
                // we should not set the focus in a event filter for a focus event,
                // so do it when the focus event is processed
                QTimer::singleShot(0, this, SLOT(putFocusToEditorList()));
            }
        }
    }
    return false;
}

void OpenEditorsWidget::putFocusToEditorList()
{
    m_ui.editorList->setFocus();
}

NavigationView OpenEditorsViewFactory::createWidget()
{
    NavigationView n;
    n.widget = new OpenEditorsWidget();
    return n;
}

QString OpenEditorsViewFactory::displayName()
{
    return "Open Documents";
}

QKeySequence OpenEditorsViewFactory::activationSequence()
{
    return QKeySequence(Qt::ALT + Qt::Key_O);
}

OpenEditorsViewFactory::OpenEditorsViewFactory()
{
}

OpenEditorsViewFactory::~OpenEditorsViewFactory()
{
}
