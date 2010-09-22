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

#include "watchwindow.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggerengine.h"
#include "debuggerdialogs.h"
#include "watchhandler.h"
#include "watchdelegatewidgets.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

#include <QtGui/QAction>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QItemDelegate>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>

using namespace Debugger;
using namespace Debugger::Internal;

/////////////////////////////////////////////////////////////////////
//
// WatchDelegate
//
/////////////////////////////////////////////////////////////////////

class WatchDelegate : public QItemDelegate
{
public:
    WatchDelegate(QObject *parent) : QItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &index) const
    {
        // Value column: Custom editor. Apply integer-specific settings.
        if (index.column() == 1) {
            const QVariant::Type type = static_cast<QVariant::Type>(index.data(LocalsEditTypeRole).toInt());
            switch (type) {
            case QVariant::Bool:
                return new BooleanComboBox(parent);
            default:
                break;
            }
            WatchLineEdit *edit = WatchLineEdit::create(type, parent);
            if (IntegerWatchLineEdit *intEdit = qobject_cast<IntegerWatchLineEdit *>(edit))
                intEdit->setBase(index.data(LocalsIntegerBaseRole).toInt());
            return edit;
        }
        // Standard line edits for the rest
        return new QLineEdit(parent);
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        if (index.column() == 1) {
            editor->setProperty("modelData", index.data(Qt::EditRole));
        } else {
            QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
            QTC_ASSERT(lineEdit, return);
            lineEdit->setText(index.data(LocalsExpressionRole).toString());
        }
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
        const QModelIndex &index) const
    {
        const QString exp = index.data(LocalsExpressionRole).toString();
        if (index.column() == 1) { // The value column.
            const QVariant value = editor->property("modelData");
            QTC_ASSERT(value.isValid(), return);
            const QString command = exp + QLatin1Char('=') + value.toString();
            model->setData(index, QVariant(command), RequestAssignValueRole);
            return;
        }
        //qDebug() << "SET MODEL DATA";
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
        QTC_ASSERT(lineEdit, return);
        const QString value = lineEdit->text();
        model->setData(index, value, Qt::EditRole);
        if (index.column() == 2) {
            // The type column.
            model->setData(index, QString(exp + '=' + value), RequestAssignTypeRole);
        } else if (index.column() == 0) {
            // The watcher name column.
            model->setData(index, exp, RequestRemoveWatchExpressionRole);
            model->setData(index, value, RequestWatchExpressionRole);
        }
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
        const QModelIndex &) const
    {
        editor->setGeometry(option.rect);
    }
};


/////////////////////////////////////////////////////////////////////
//
// WatchWindow
//
/////////////////////////////////////////////////////////////////////

WatchWindow::WatchWindow(Type type, QWidget *parent)
  : QTreeView(parent),
    m_alwaysResizeColumnsToContents(true),
    m_type(type)
{
    m_grabbing = false;

    QAction *act = theDebuggerAction(UseAlternatingRowColors);
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setWindowTitle(tr("Locals and Watchers"));
    setAlternatingRowColors(act->isChecked());
    setIndentation(indentation() * 9/10);
    setUniformRowHeights(true);
    setItemDelegate(new WatchDelegate(this));
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);

    connect(act, SIGNAL(toggled(bool)),
        this, SLOT(setAlternatingRowColorsHelper(bool)));
    connect(this, SIGNAL(expanded(QModelIndex)),
        this, SLOT(expandNode(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)),
        this, SLOT(collapseNode(QModelIndex)));
}

void WatchWindow::expandNode(const QModelIndex &idx)
{
    setModelData(LocalsExpandedRole, true, idx);
}

void WatchWindow::collapseNode(const QModelIndex &idx)
{
    setModelData(LocalsExpandedRole, false, idx);
}

void WatchWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Delete && m_type == WatchersType) {
        QModelIndex idx = currentIndex();
        QModelIndex idx1 = idx.sibling(idx.row(), 0);
        QString exp = idx1.data().toString();
        removeWatchExpression(exp);
    } else if (ev->key() == Qt::Key_Return
            && ev->modifiers() == Qt::ControlModifier
            && m_type == LocalsType) {
        QModelIndex idx = currentIndex();
        QModelIndex idx1 = idx.sibling(idx.row(), 0);
        QString exp = model()->data(idx1).toString();
        watchExpression(exp);
    }
    QTreeView::keyPressEvent(ev);
}

void WatchWindow::dragEnterEvent(QDragEnterEvent *ev)
{
    //QTreeView::dragEnterEvent(ev);
    if (ev->mimeData()->hasFormat("text/plain")) {
        ev->setDropAction(Qt::CopyAction);
        ev->accept();
    }
}

void WatchWindow::dragMoveEvent(QDragMoveEvent *ev)
{
    //QTreeView::dragMoveEvent(ev);
    if (ev->mimeData()->hasFormat("text/plain")) {
        ev->setDropAction(Qt::CopyAction);
        ev->accept();
    }
}

void WatchWindow::dropEvent(QDropEvent *ev)
{
    if (ev->mimeData()->hasFormat("text/plain")) {
        watchExpression(ev->mimeData()->text());
        //ev->acceptProposedAction();
        ev->setDropAction(Qt::CopyAction);
        ev->accept();
    }
    //QTreeView::dropEvent(ev);
}

void WatchWindow::mouseDoubleClickEvent(QMouseEvent *ev)
{
    const QModelIndex idx = indexAt(ev->pos());
    if (!idx.isValid()) {
        // The "<Edit>" case.
        watchExpression(QString());
        return;
    }
    QTreeView::mouseDoubleClickEvent(ev);
}

void WatchWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    const QModelIndex idx = indexAt(ev->pos());
    const QModelIndex mi0 = idx.sibling(idx.row(), 0);
    const QModelIndex mi1 = idx.sibling(idx.row(), 1);
    const QModelIndex mi2 = idx.sibling(idx.row(), 2);
    const quint64 address = mi0.data(LocalsAddressRole).toULongLong();
    const quint64 pointerValue = mi0.data(LocalsPointerValueRole).toULongLong();
    const QString exp = mi0.data(LocalsExpressionRole).toString();
    const QString type = mi2.data().toString();

    const QStringList alternativeFormats =
        mi0.data(LocalsTypeFormatListRole).toStringList();
    const int typeFormat =
        mi0.data(LocalsTypeFormatRole).toInt();
    const int individualFormat =
        mi0.data(LocalsIndividualFormatRole).toInt();
    const int effectiveIndividualFormat =
        individualFormat == -1 ? typeFormat : individualFormat;

    QMenu typeFormatMenu;
    QList<QAction *> typeFormatActions;
    QAction *clearTypeFormatAction = 0;
    if (idx.isValid()) {
        typeFormatMenu.setTitle(
            tr("Change Format for Type \"%1\"").arg(type));
        if (alternativeFormats.isEmpty()) {
            typeFormatMenu.setEnabled(false);
        } else {
            clearTypeFormatAction = typeFormatMenu.addAction(tr("Automatic"));
            clearTypeFormatAction->setEnabled(typeFormat != -1);
            clearTypeFormatAction->setCheckable(true);
            clearTypeFormatAction->setChecked(typeFormat == -1);
            typeFormatMenu.addSeparator();
            for (int i = 0; i != alternativeFormats.size(); ++i) {
                const QString format = alternativeFormats.at(i);
                QAction *act = new QAction(format, &typeFormatMenu);
                act->setCheckable(true);
                if (i == typeFormat)
                    act->setChecked(true);
                typeFormatMenu.addAction(act);
                typeFormatActions.append(act);
            }
        }
    } else {
        typeFormatMenu.setTitle(tr("Change Format for Type"));
        typeFormatMenu.setEnabled(false);
    }

    QMenu individualFormatMenu;
    QList<QAction *> individualFormatActions;
    QAction *clearIndividualFormatAction = 0;
    if (idx.isValid()) {
        individualFormatMenu.setTitle(
            tr("Change Format for Object Named \"%1\"").arg(mi0.data().toString()));
        if (alternativeFormats.isEmpty()) {
            individualFormatMenu.setEnabled(false);
        } else {
            clearIndividualFormatAction
                = individualFormatMenu.addAction(tr("Automatic"));
            clearIndividualFormatAction->setEnabled(individualFormat != -1);
            clearIndividualFormatAction->setCheckable(true);
            clearIndividualFormatAction->setChecked(individualFormat == -1);
            individualFormatMenu.addSeparator();
            for (int i = 0; i != alternativeFormats.size(); ++i) {
                const QString format = alternativeFormats.at(i);
                QAction *act = new QAction(format, &individualFormatMenu);
                act->setCheckable(true);
                if (i == effectiveIndividualFormat)
                    act->setChecked(true);
                individualFormatMenu.addAction(act);
                individualFormatActions.append(act);
            }
        }
    } else {
        individualFormatMenu.setTitle(tr("Change Format for Object"));
        individualFormatMenu.setEnabled(false);
    }

    const bool actionsEnabled = modelData(EngineActionsEnabledRole).toBool();
    const unsigned engineCapabilities = modelData(EngineCapabilitiesRole).toUInt();
    const bool canHandleWatches =
        actionsEnabled && (engineCapabilities & AddWatcherCapability);

    QMenu menu;
    QAction *actInsertNewWatchItem = menu.addAction(tr("Insert New Watch Item"));
    actInsertNewWatchItem->setEnabled(canHandleWatches);
    QAction *actSelectWidgetToWatch = menu.addAction(tr("Select Widget to Watch"));
    actSelectWidgetToWatch->setEnabled(canHandleWatches);

    QAction *actOpenMemoryEditAtVariableAddress = 0;
    QAction *actOpenMemoryEditAtPointerValue = 0;
    QAction *actOpenMemoryEditor =
        new QAction(tr("Open Memory Editor..."), &menu);
    const bool canShowMemory = engineCapabilities & ShowMemoryCapability;
    actOpenMemoryEditor->setEnabled(actionsEnabled && canShowMemory);

    // Offer to open address pointed to or variable address.
    const bool createPointerActions = pointerValue && pointerValue != address;

    if (canShowMemory && address)
        actOpenMemoryEditAtVariableAddress =
            new QAction(tr("Open Memory Editor at Object's Address (0x%1)")
                .arg(address, 0, 16), &menu);
    if (createPointerActions)
        actOpenMemoryEditAtPointerValue =
            new QAction(tr("Open Memory Editor at Referenced Address (0x%1)")
                .arg(pointerValue, 0, 16), &menu);
    menu.addSeparator();

    QAction *actSetWatchPointAtVariableAddress = 0;
    QAction *actSetWatchPointAtPointerValue = 0;
    const bool canSetWatchpoint = engineCapabilities & WatchpointCapability;
    if (canSetWatchpoint && address) {
        actSetWatchPointAtVariableAddress =
            new QAction(tr("Break on Changes at Object's Address (0x%1)")
                .arg(address, 0, 16), &menu);
        actSetWatchPointAtVariableAddress->setCheckable(true);
        actSetWatchPointAtVariableAddress->
            setChecked(mi0.data(LocalsIsWatchpointAtAddressRole).toBool());
        if (createPointerActions) {
            actSetWatchPointAtPointerValue =
                new QAction(tr("Break on Changes at Referenced Address (0x%1)")
                    .arg(pointerValue, 0, 16), &menu);
            actSetWatchPointAtPointerValue->setCheckable(true);
            actSetWatchPointAtPointerValue->
                setChecked(mi0.data(LocalsIsWatchpointAtPointerValueRole).toBool());
        }
    } else {
        actSetWatchPointAtVariableAddress =
            new QAction(tr("Break on Changing Contents"), &menu);
        actSetWatchPointAtVariableAddress->setEnabled(false);
    }

    QString actionName = exp.isEmpty() ? tr("Watch Expression")
        : tr("Watch Expression \"%1\"").arg(exp);
    QAction *actWatchExpression = new QAction(actionName, &menu);
    actWatchExpression->setEnabled(canHandleWatches && !exp.isEmpty());

    actionName = exp.isEmpty() ? tr("Remove Watch Expression")
        : tr("Remove Watch Expression \"%1\"").arg(exp);
    QAction *actRemoveWatchExpression = new QAction(actionName, &menu);
    actRemoveWatchExpression->setEnabled(canHandleWatches && !exp.isEmpty());

    if (m_type == LocalsType)
        menu.addAction(actWatchExpression);
    else
        menu.addAction(actRemoveWatchExpression);

    menu.addAction(actInsertNewWatchItem);
    menu.addAction(actSelectWidgetToWatch);
    menu.addMenu(&typeFormatMenu);
    menu.addMenu(&individualFormatMenu);
    if (actOpenMemoryEditAtVariableAddress)
        menu.addAction(actOpenMemoryEditAtVariableAddress);
    if (actOpenMemoryEditAtPointerValue)
        menu.addAction(actOpenMemoryEditAtPointerValue);
    menu.addAction(actOpenMemoryEditor);
    menu.addAction(actSetWatchPointAtVariableAddress);
    if (actSetWatchPointAtPointerValue)
        menu.addAction(actSetWatchPointAtPointerValue);
    menu.addSeparator();

    menu.addAction(theDebuggerAction(RecheckDebuggingHelpers));
    menu.addAction(theDebuggerAction(UseDebuggingHelpers));
    QAction *actClearCodeModelSnapshot
        = new QAction(tr("Refresh Code Model Snapshot"), &menu);
    actClearCodeModelSnapshot->setEnabled(actionsEnabled
        && theDebuggerAction(UseCodeModel)->isChecked());
    menu.addAction(actClearCodeModelSnapshot);
    QAction *actShowInEditor
        = new QAction(tr("Show View Contents in Editor"), &menu);
    actShowInEditor->setEnabled(actionsEnabled);
    menu.addAction(actShowInEditor);
    menu.addSeparator();
    menu.addAction(theDebuggerAction(UseToolTipsInLocalsView));

    menu.addAction(theDebuggerAction(AutoDerefPointers));
    menu.addAction(theDebuggerAction(ShowStdNamespace));
    menu.addAction(theDebuggerAction(ShowQtNamespace));

    QAction *actAdjustColumnWidths =
        menu.addAction(tr("Adjust Column Widths to Contents"));
    QAction *actAlwaysAdjustColumnWidth =
        menu.addAction(tr("Always Adjust Column Widths to Contents"));
    actAlwaysAdjustColumnWidth->setCheckable(true);
    actAlwaysAdjustColumnWidth->setChecked(m_alwaysResizeColumnsToContents);

    menu.addSeparator();
    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());
    if (act == 0)
        return;

    if (act == actAdjustColumnWidths) {
        resizeColumnsToContents();
    } else if (act == actAlwaysAdjustColumnWidth) {
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    } else if (act == actInsertNewWatchItem) {
        watchExpression(QString());
    } else if (act == actOpenMemoryEditAtVariableAddress) {
        setModelData(RequestShowMemoryRole, address);
    } else if (act == actOpenMemoryEditAtPointerValue) {
        setModelData(RequestShowMemoryRole, pointerValue);
    } else if (act == actOpenMemoryEditor) {
        AddressDialog dialog;
        if (dialog.exec() == QDialog::Accepted)
            setModelData(RequestShowMemoryRole, dialog.address());
    } else if (act == actSetWatchPointAtVariableAddress) {
        setModelData(RequestToggleWatchRole, address);
    } else if (act == actSetWatchPointAtPointerValue) {
        setModelData(RequestToggleWatchRole, pointerValue);
    } else if (act == actSelectWidgetToWatch) {
        grabMouse(Qt::CrossCursor);
        m_grabbing = true;
    } else if (act == actWatchExpression) {
        watchExpression(exp);
    } else if (act == actRemoveWatchExpression) {
        removeWatchExpression(exp);
    } else if (act == actClearCodeModelSnapshot) {
        setModelData(RequestClearCppCodeModelSnapshotRole);
    } else if (act == clearTypeFormatAction) {
        setModelData(LocalsTypeFormatRole, -1, mi1);
    } else if (act == clearIndividualFormatAction) {
        setModelData(LocalsIndividualFormatRole, -1, mi1);
    } else if (act == actShowInEditor) {
        setModelData(RequestShowInEditorRole);
    } else {
        for (int i = 0; i != typeFormatActions.size(); ++i) {
            if (act == typeFormatActions.at(i))
                setModelData(LocalsTypeFormatRole, i, mi1);
        }
        for (int i = 0; i != individualFormatActions.size(); ++i) {
            if (act == individualFormatActions.at(i))
                setModelData(LocalsIndividualFormatRole, i, mi1);
        }
    }
}

void WatchWindow::resizeColumnsToContents()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
}

void WatchWindow::setAlwaysResizeColumnsToContents(bool on)
{
    if (!header())
        return;
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    header()->setResizeMode(1, mode);
}

bool WatchWindow::event(QEvent *ev)
{
    if (m_grabbing && ev->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mev = static_cast<QMouseEvent *>(ev);
        m_grabbing = false;
        releaseMouse();
        setModelData(RequestWatchPointRole, mapToGlobal(mev->pos()));
    }
    return QTreeView::event(ev);
}

void WatchWindow::editItem(const QModelIndex &idx)
{
    Q_UNUSED(idx) // FIXME
}

void WatchWindow::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);

    setRootIsDecorated(true);
    header()->setDefaultAlignment(Qt::AlignLeft);
    header()->setResizeMode(QHeaderView::ResizeToContents);
    if (m_type != LocalsType)
        header()->hide();

    connect(model, SIGNAL(layoutChanged()),
        this, SLOT(resetHelper()));
    connect(model, SIGNAL(enableUpdates(bool)),
        this, SLOT(setUpdatesEnabled(bool)));
}

void WatchWindow::setUpdatesEnabled(bool enable)
{
    //qDebug() << "ENABLING UPDATES: " << enable;
    QTreeView::setUpdatesEnabled(enable);
}

void WatchWindow::resetHelper()
{
    resetHelper(model()->index(0, 0));
}

void WatchWindow::resetHelper(const QModelIndex &idx)
{
    if (idx.data(LocalsExpandedRole).toBool()) {
        //qDebug() << "EXPANDING " << model()->data(idx, INameRole);
        expand(idx);
        for (int i = 0, n = model()->rowCount(idx); i != n; ++i) {
            QModelIndex idx1 = model()->index(i, 0, idx);
            resetHelper(idx1);
        }
    } else {
        //qDebug() << "COLLAPSING " << model()->data(idx, INameRole);
        collapse(idx);
    }
}

void WatchWindow::watchExpression(const QString &exp)
{
    setModelData(RequestWatchExpressionRole, exp);
}

void WatchWindow::removeWatchExpression(const QString &exp)
{
    setModelData(RequestRemoveWatchExpressionRole, exp);
}

void WatchWindow::setModelData
    (int role, const QVariant &value, const QModelIndex &index)
{
    QTC_ASSERT(model(), return);
    model()->setData(index, value, role);
}

QVariant WatchWindow::modelData(int role, const QModelIndex &index)
{
    QTC_ASSERT(model(), return QVariant());
    return model()->data(index, role);
}

