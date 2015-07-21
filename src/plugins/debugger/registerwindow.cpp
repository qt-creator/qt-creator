/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "registerwindow.h"
#include "memoryview.h"
#include "memoryagent.h"
#include "debuggeractions.h"
#include "debuggerdialogs.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "registerhandler.h"
#include "watchdelegatewidgets.h"

#include <utils/savedaction.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QItemDelegate>
#include <QMenu>
#include <QPainter>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// RegisterDelegate
//
///////////////////////////////////////////////////////////////////////

class RegisterDelegate : public QItemDelegate
{
public:
    RegisterDelegate(QObject *parent)
        : QItemDelegate(parent)
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &index) const
    {
        IntegerWatchLineEdit *lineEdit = new IntegerWatchLineEdit(parent);
        const RegisterFormat format = RegisterFormat(index.data(RegisterFormatRole).toInt());
        const bool big = index.data(RegisterIsBigRole).toBool();
        // Big integers are assumed to be hexadecimal.
        int base = 16;
        if (!big) {
            if (format == DecimalFormat || format == SignedDecimalFormat)
                base = 10;
            else if (format == OctalFormat)
                base = 8;
            else if (format == BinaryFormat)
                base = 2;
        }
        lineEdit->setBigInt(big);
        lineEdit->setBase(base);
        lineEdit->setSigned(false);
        lineEdit->setAlignment(Qt::AlignRight);
        lineEdit->setFrame(false);
        return lineEdit;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        IntegerWatchLineEdit *lineEdit = qobject_cast<IntegerWatchLineEdit *>(editor);
        QTC_ASSERT(lineEdit, return);
        lineEdit->setModelData(index.data(Qt::EditRole));
    }

    void setModelData(QWidget *editor, QAbstractItemModel *,
        const QModelIndex &index) const
    {
        if (index.column() != 1)
            return;
        IntegerWatchLineEdit *lineEdit = qobject_cast<IntegerWatchLineEdit*>(editor);
        QTC_ASSERT(lineEdit, return);
        const RegisterFormat format = RegisterFormat(index.data(RegisterFormatRole).toInt());
        QString value = lineEdit->text();
        if (format == HexadecimalFormat && !value.startsWith(QLatin1String("0x")))
            value.insert(0, QLatin1String("0x"));
        currentEngine()->setRegisterValue(index.data(RegisterNameRole).toByteArray(), value);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
        const QModelIndex &) const
    {
        editor->setGeometry(option.rect);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const
    {
        if (index.column() == RegisterValueColumn) {
            const bool paintRed = index.data(RegisterChangedRole).toBool();
            QPen oldPen = painter->pen();
            const QColor lightColor(140, 140, 140);
            if (paintRed)
                painter->setPen(QColor(200, 0, 0));
            else
                painter->setPen(lightColor);
            // FIXME: performance? this changes only on real font changes.
            QFontMetrics fm(option.font);
            int charWidth = qMax(fm.width(QLatin1Char('x')), fm.width(QLatin1Char('0')));
            QString str = index.data(Qt::DisplayRole).toString();
            int x = option.rect.x();
            bool light = !paintRed;
            for (int i = 0; i < str.size(); ++i) {
                const QChar c = str.at(i);
                const int uc = c.unicode();
                if (light && (uc != 'x' && uc != '0')) {
                    light = false;
                    painter->setPen(oldPen.color());
                }
                if (uc == ' ') {
                    light = true;
                    painter->setPen(lightColor);
                } else {
                    QRect r = option.rect;
                    r.setX(x);
                    r.setWidth(charWidth);
                    painter->drawText(r, Qt::AlignHCenter, c);
                }
                x += charWidth;
            }
            painter->setPen(oldPen);
        } else {
            QItemDelegate::paint(painter, option, index);
        }
    }
};


///////////////////////////////////////////////////////////////////////
//
// RegisterWindow
//
///////////////////////////////////////////////////////////////////////

RegisterTreeView::RegisterTreeView()
{
    setItemDelegate(new RegisterDelegate(this));
    setRootIsDecorated(true);
}

void RegisterTreeView::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;

    DebuggerEngine *engine = currentEngine();
    QTC_ASSERT(engine, return);
    RegisterHandler *handler = engine->registerHandler();

    const bool actionsEnabled = engine->debuggerActionsEnabled();
    const DebuggerState state = engine->state();

    QAction *actReload = menu.addAction(tr("Reload Register Listing"));
    actReload->setEnabled(engine->hasCapability(RegisterCapability)
        && (state == InferiorStopOk || state == InferiorUnrunnable));

    menu.addSeparator();

    const QModelIndex idx = indexAt(ev->pos());
    const quint64 address = idx.data(RegisterAsAddressRole).toULongLong();
    QAction *actViewMemory = menu.addAction(QString());
    QAction *actEditMemory = menu.addAction(QString());

    QAction *actShowDisassemblerAt = menu.addAction(QString());
    QAction *actShowDisassembler = menu.addAction(tr("Open Disassembler..."));
    actShowDisassembler->setEnabled(engine->hasCapability(DisassemblerCapability));

    const QByteArray registerName = idx.data(RegisterNameRole).toByteArray();
    const QString registerNameStr = QString::fromUtf8(registerName);
    if (address) {
        const bool canShow = actionsEnabled && engine->hasCapability(ShowMemoryCapability);
        actEditMemory->setText(tr("Open Memory Editor at 0x%1").arg(address, 0, 16));
        actEditMemory->setEnabled(canShow);
        actViewMemory->setText(tr("Open Memory View at Value of Register %1 0x%2")
            .arg(registerNameStr).arg(address, 0, 16));
        actShowDisassemblerAt->setText(tr("Open Disassembler at 0x%1")
            .arg(address, 0, 16));
        actShowDisassemblerAt->setEnabled(engine->hasCapability(DisassemblerCapability));
    } else {
        actEditMemory->setText(tr("Open Memory Editor"));
        actViewMemory->setText(tr("Open Memory View at Value of Register"));
        actEditMemory->setEnabled(false);
        actViewMemory->setEnabled(false);
        actShowDisassemblerAt->setText(tr("Open Disassembler"));
        actShowDisassemblerAt->setEnabled(false);
    }
    menu.addSeparator();

    const RegisterFormat format = RegisterFormat(idx.data(RegisterFormatRole).toInt());
    QAction *act16 = menu.addAction(tr("Hexadecimal"));
    act16->setCheckable(true);
    act16->setChecked(format == HexadecimalFormat);
    QAction *act10 = menu.addAction(tr("Decimal"));
    act10->setCheckable(true);
    act10->setChecked(format == DecimalFormat);
    QAction *act8 = menu.addAction(tr("Octal"));
    act8->setCheckable(true);
    act8->setChecked(format == OctalFormat);
    QAction *act2 = menu.addAction(tr("Binary"));
    act2->setCheckable(true);
    act2->setChecked(format == BinaryFormat);

    menu.addSeparator();
    menu.addAction(action(SettingsDialog));

    const QPoint position = ev->globalPos();
    QAction *act = menu.exec(position);

    if (act == actReload) {
        engine->reloadRegisters();
    } else if (act == actEditMemory) {
        MemoryViewSetupData data;
        data.startAddress = address;
        data.registerName = registerName;
        data.markup = RegisterMemoryView::registerMarkup(address, registerName);
        data.title = RegisterMemoryView::title(registerName);
        engine->openMemoryView(data);
    } else if (act == actViewMemory) {
        MemoryViewSetupData data;
        data.startAddress = address;
        data.flags = DebuggerEngine::MemoryTrackRegister|DebuggerEngine::MemoryView,
        data.registerName = registerName;
        data.pos = position;
        data.parent = this;
        engine->openMemoryView(data);
    } else if (act == actShowDisassembler) {
        AddressDialog dialog;
        if (address)
            dialog.setAddress(address);
        if (dialog.exec() == QDialog::Accepted)
            currentEngine()->openDisassemblerView(Location(dialog.address()));
    } else if (act == actShowDisassemblerAt) {
        engine->openDisassemblerView(Location(address));
    } else if (act == act16)
        handler->setNumberFormat(registerName, HexadecimalFormat);
    else if (act == act10)
        handler->setNumberFormat(registerName, DecimalFormat);
    else if (act == act8)
        handler->setNumberFormat(registerName, OctalFormat);
    else if (act == act2)
        handler->setNumberFormat(registerName, BinaryFormat);
}

void RegisterTreeView::reloadRegisters()
{
    // FIXME: Only trigger when becoming visible?
    currentEngine()->reloadRegisters();
}

} // namespace Internal
} // namespace Debugger
