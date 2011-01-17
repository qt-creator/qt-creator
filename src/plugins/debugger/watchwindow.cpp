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

#include "watchwindow.h"

#include "breakhandler.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "watchdelegatewidgets.h"
#include "watchhandler.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QVariant>

#include <QtGui/QContextMenuEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QItemDelegate>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>


/////////////////////////////////////////////////////////////////////
//
// WatchDelegate
//
/////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

static DebuggerEngine *currentEngine()
{
    return debuggerCore()->currentEngine();
}

class WatchDelegate : public QItemDelegate
{
public:
    explicit WatchDelegate(WatchWindow *parent)
        : QItemDelegate(parent), m_watchWindow(parent)
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &index) const
    {
        // Value column: Custom editor. Apply integer-specific settings.
        if (index.column() == 1) {
            const QVariant::Type type =
                static_cast<QVariant::Type>(index.data(LocalsEditTypeRole).toInt());
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

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const
    {
        // Standard handling for anything but the watcher name column (change
        // expression), which removes/recreates a row, which cannot be done
        // in model->setData().
        if (index.column() != 0) {
            QItemDelegate::setModelData(editor, model, index);
            return;
        }
        const QMetaProperty userProperty = editor->metaObject()->userProperty();
        QTC_ASSERT(userProperty.isValid(), return);
        const QString value = editor->property(userProperty.name()).toString();
        const QString exp = index.data(LocalsExpressionRole).toString();
        if (exp == value)
            return;
        m_watchWindow->removeWatchExpression(exp);
        m_watchWindow->watchExpression(value);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
        const QModelIndex &) const
    {
        editor->setGeometry(option.rect);
    }

private:
    WatchWindow *m_watchWindow;
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

    QAction *act = debuggerCore()->action(UseAlternatingRowColors);
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

// Text for add watch action with truncated expression
static inline QString addWatchActionText(QString exp)
{
    if (exp.isEmpty())
        return WatchWindow::tr("Watch Expression");
    if (exp.size() > 30) {
        exp.truncate(30);
        exp.append(QLatin1String("..."));
    }
    return WatchWindow::tr("Watch Expression \"%1\"").arg(exp);
}

// Text for add watch action with truncated expression
static inline QString removeWatchActionText(QString exp)
{
    if (exp.isEmpty())
        return WatchWindow::tr("Remove Watch Expression");
    if (exp.size() > 30) {
        exp.truncate(30);
        exp.append(QLatin1String("..."));
    }
    return WatchWindow::tr("Remove Watch Expression \"%1\"").arg(exp);
}

void WatchWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    DebuggerEngine *engine = currentEngine();
    WatchHandler *handler = engine->watchHandler();

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

    QMenu formatMenu;
    QList<QAction *> typeFormatActions;
    QList<QAction *> individualFormatActions;
    QAction *clearTypeFormatAction = 0;
    QAction *clearIndividualFormatAction = 0;
    formatMenu.setTitle(tr("Change Display Format..."));
    if (idx.isValid() && !alternativeFormats.isEmpty()) {
        QAction *dummy = formatMenu.addAction(
            tr("Change Display for Type \"%1\"").arg(type));
        dummy->setEnabled(false);
        formatMenu.addSeparator();
        clearTypeFormatAction = formatMenu.addAction(tr("Automatic"));
        //clearTypeFormatAction->setEnabled(typeFormat != -1);
        //clearTypeFormatAction->setEnabled(individualFormat != -1);
        clearTypeFormatAction->setCheckable(true);
        clearTypeFormatAction->setChecked(typeFormat == -1);
        formatMenu.addSeparator();
        for (int i = 0; i != alternativeFormats.size(); ++i) {
            const QString format = alternativeFormats.at(i);
            QAction *act = new QAction(format, &formatMenu);
            act->setCheckable(true);
            //act->setEnabled(individualFormat != -1);
            if (i == typeFormat)
                act->setChecked(true);
            formatMenu.addAction(act);
            typeFormatActions.append(act);
        }
        formatMenu.addSeparator();
        dummy = formatMenu.addAction(
            tr("Change Display for Object Named \"%1\"").arg(mi0.data().toString()));
        dummy->setEnabled(false);
        formatMenu.addSeparator();
        clearIndividualFormatAction
            = formatMenu.addAction(tr("Use Display Format Based on Type"));
        //clearIndividualFormatAction->setEnabled(individualFormat != -1);
        clearIndividualFormatAction->setCheckable(true);
        clearIndividualFormatAction->setChecked(effectiveIndividualFormat == -1);
        formatMenu.addSeparator();
        for (int i = 0; i != alternativeFormats.size(); ++i) {
            const QString format = alternativeFormats.at(i);
            QAction *act = new QAction(format, &formatMenu);
            act->setCheckable(true);
            if (i == effectiveIndividualFormat)
                act->setChecked(true);
            formatMenu.addAction(act);
            individualFormatActions.append(act);
        }
    } else {
        formatMenu.setEnabled(false);
    }

    const bool actionsEnabled = engine->debuggerActionsEnabled();
    const unsigned engineCapabilities = engine->debuggerCapabilities();
    const bool canHandleWatches =
        actionsEnabled && (engineCapabilities & AddWatcherCapability);
    const DebuggerState state = engine->state();

    QMenu menu;
    QAction *actInsertNewWatchItem = menu.addAction(tr("Insert New Watch Item"));
    actInsertNewWatchItem->setEnabled(canHandleWatches);
    QAction *actSelectWidgetToWatch = menu.addAction(tr("Select Widget to Watch"));
    actSelectWidgetToWatch->setEnabled(canHandleWatches);

    // Offer to open address pointed to or variable address.
    const bool createPointerActions = pointerValue && pointerValue != address;

    menu.addSeparator();

    QAction *actSetWatchpointAtVariableAddress = 0;
    QAction *actSetWatchpointAtPointerValue = 0;
    const bool canSetWatchpoint = engineCapabilities & WatchpointCapability;
    if (canSetWatchpoint && address) {
        actSetWatchpointAtVariableAddress =
            new QAction(tr("Add Watchpoint at Object's Address (0x%1)")
                .arg(address, 0, 16), &menu);
        actSetWatchpointAtVariableAddress->
            setChecked(mi0.data(LocalsIsWatchpointAtAddressRole).toBool());
        if (createPointerActions) {
            actSetWatchpointAtPointerValue =
                new QAction(tr("Add Watchpoint at Referenced Address (0x%1)")
                    .arg(pointerValue, 0, 16), &menu);
            actSetWatchpointAtPointerValue->setCheckable(true);
            actSetWatchpointAtPointerValue->
                setChecked(mi0.data(LocalsIsWatchpointAtPointerValueRole).toBool());
        }
    } else {
        actSetWatchpointAtVariableAddress =
            new QAction(tr("Add Watchpoint"), &menu);
        actSetWatchpointAtVariableAddress->setEnabled(false);
    }
    actSetWatchpointAtVariableAddress->setToolTip(
        tr("Setting a watchpoint on an address will cause the program "
           "to stop when the data at the address it modified."));

    QAction *actWatchExpression = new QAction(addWatchActionText(exp), &menu);
    actWatchExpression->setEnabled(canHandleWatches && !exp.isEmpty());

    // Can remove watch if engine can handle it or session engine.
    QAction *actRemoveWatchExpression = new QAction(removeWatchActionText(exp), &menu);
    actRemoveWatchExpression->setEnabled(
        (canHandleWatches || state == DebuggerNotReady) && !exp.isEmpty());
    QAction *actRemoveWatches = new QAction(tr("Remove All Watch Items"), &menu);
    actRemoveWatches->setEnabled(!WatchHandler::watcherNames().isEmpty());

    if (m_type == LocalsType)
        menu.addAction(actWatchExpression);
    else {
        menu.addAction(actRemoveWatchExpression);
        menu.addAction(actRemoveWatches);
    }

    QMenu memoryMenu;
    memoryMenu.setTitle(tr("Open Memory Editor..."));
    QAction *actOpenMemoryEditAtVariableAddress = new QAction(&memoryMenu);
    QAction *actOpenMemoryEditAtPointerValue = new QAction(&memoryMenu);
    QAction *actOpenMemoryEditor = new QAction(&memoryMenu);
    if (engineCapabilities & ShowMemoryCapability) {
        actOpenMemoryEditor->setText(tr("Open Memory Editor..."));
        if (address) {
            actOpenMemoryEditAtVariableAddress->setText(
                tr("Open Memory Editor at Object's Address (0x%1)")
                    .arg(address, 0, 16));
        } else {
            actOpenMemoryEditAtVariableAddress->setText(
                tr("Open Memory Editor at Object's Address"));
            actOpenMemoryEditAtVariableAddress->setEnabled(false);
        }
        if (createPointerActions) {
            actOpenMemoryEditAtPointerValue->setText(
                tr("Open Memory Editor at Referenced Address (0x%1)")
                    .arg(pointerValue, 0, 16));
        } else {
            actOpenMemoryEditAtPointerValue->setText(
                tr("Open Memory Editor at Referenced Address"));
            actOpenMemoryEditAtPointerValue->setEnabled(false);
        }
        memoryMenu.addAction(actOpenMemoryEditAtVariableAddress);
        memoryMenu.addAction(actOpenMemoryEditAtPointerValue);
        memoryMenu.addAction(actOpenMemoryEditor);
    } else {
        memoryMenu.setEnabled(false);
    }

    menu.addAction(actInsertNewWatchItem);
    menu.addAction(actSelectWidgetToWatch);
    menu.addMenu(&formatMenu);
    menu.addMenu(&memoryMenu);
    menu.addAction(actSetWatchpointAtVariableAddress);
    if (actSetWatchpointAtPointerValue)
        menu.addAction(actSetWatchpointAtPointerValue);
    menu.addSeparator();

    menu.addAction(debuggerCore()->action(UseDebuggingHelpers));
    menu.addAction(debuggerCore()->action(UseToolTipsInLocalsView));
    menu.addAction(debuggerCore()->action(AutoDerefPointers));
    menu.addAction(debuggerCore()->action(ShowStdNamespace));
    menu.addAction(debuggerCore()->action(ShowQtNamespace));
    menu.addAction(debuggerCore()->action(SortStructMembers));

    QAction *actAdjustColumnWidths =
        menu.addAction(tr("Adjust Column Widths to Contents"));
    QAction *actAlwaysAdjustColumnWidth =
        menu.addAction(tr("Always Adjust Column Widths to Contents"));
    actAlwaysAdjustColumnWidth->setCheckable(true);
    actAlwaysAdjustColumnWidth->setChecked(m_alwaysResizeColumnsToContents);

    menu.addSeparator();
    QAction *actClearCodeModelSnapshot
        = new QAction(tr("Refresh Code Model Snapshot"), &menu);
    actClearCodeModelSnapshot->setEnabled(actionsEnabled
        && debuggerCore()->action(UseCodeModel)->isChecked());
    menu.addAction(actClearCodeModelSnapshot);
    QAction *actShowInEditor
        = new QAction(tr("Show View Contents in Editor"), &menu);
    actShowInEditor->setEnabled(actionsEnabled);
    menu.addAction(actShowInEditor);
    menu.addAction(debuggerCore()->action(SettingsDialog));

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
        currentEngine()->openMemoryView(address);
    } else if (act == actOpenMemoryEditAtPointerValue) {
        currentEngine()->openMemoryView(pointerValue);
    } else if (act == actOpenMemoryEditor) {
        AddressDialog dialog;
        if (dialog.exec() == QDialog::Accepted)
            currentEngine()->openMemoryView(dialog.address());
    } else if (act == actSetWatchpointAtVariableAddress) {
        setWatchpoint(address);
    } else if (act == actSetWatchpointAtPointerValue) {
        setWatchpoint(pointerValue);
    } else if (act == actSelectWidgetToWatch) {
        grabMouse(Qt::CrossCursor);
        m_grabbing = true;
    } else if (act == actWatchExpression) {
        watchExpression(exp);
    } else if (act == actRemoveWatchExpression) {
        removeWatchExpression(exp);
    } else if (act == actRemoveWatches) {
        currentEngine()->watchHandler()->clearWatches();
    } else if (act == actClearCodeModelSnapshot) {
        debuggerCore()->clearCppCodeModelSnapshot();
    } else if (act == clearTypeFormatAction) {
        setModelData(LocalsTypeFormatRole, -1, mi1);
    } else if (act == clearIndividualFormatAction) {
        setModelData(LocalsIndividualFormatRole, -1, mi1);
    } else if (act == actShowInEditor) {
        QString contents = handler->editorContents();
        debuggerCore()->openTextEditor(tr("Locals & Watchers"), contents);
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
        currentEngine()->watchPoint(mapToGlobal(mev->pos()));
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

    connect(model, SIGNAL(layoutChanged()), SLOT(resetHelper()));
    connect(model, SIGNAL(enableUpdates(bool)), SLOT(setUpdatesEnabled(bool)));
}

void WatchWindow::setUpdatesEnabled(bool enable)
{
    //qDebug() << "ENABLING UPDATES: " << enable;
    QTreeView::setUpdatesEnabled(enable);
}

void WatchWindow::resetHelper()
{
    bool old = updatesEnabled();
    setUpdatesEnabled(false);
    resetHelper(model()->index(0, 0));
    setUpdatesEnabled(old);
}

void WatchWindow::resetHelper(const QModelIndex &idx)
{
    if (idx.data(LocalsExpandedRole).toBool()) {
        //qDebug() << "EXPANDING " << model()->data(idx, INameRole);
        if (!isExpanded(idx)) {
            expand(idx);
            for (int i = 0, n = model()->rowCount(idx); i != n; ++i) {
                QModelIndex idx1 = model()->index(i, 0, idx);
                resetHelper(idx1);
            }
        }
    } else {
        //qDebug() << "COLLAPSING " << model()->data(idx, INameRole);
        if (isExpanded(idx))
            collapse(idx);
    }
}

void WatchWindow::watchExpression(const QString &exp)
{
    currentEngine()->watchHandler()->watchExpression(exp);
}

void WatchWindow::removeWatchExpression(const QString &exp)
{
    currentEngine()->watchHandler()->removeWatchExpression(exp);
}

void WatchWindow::setModelData
    (int role, const QVariant &value, const QModelIndex &index)
{
    QTC_ASSERT(model(), return);
    model()->setData(index, value, role);
}

void WatchWindow::setWatchpoint(quint64 address)
{
    breakHandler()->setWatchpointByAddress(address);
}

} // namespace Internal
} // namespace Debugger

