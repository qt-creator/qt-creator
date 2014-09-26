/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_DEBUGGERTOOLTIPMANAGER_H
#define DEBUGGER_DEBUGGERTOOLTIPMANAGER_H

#include "debuggerconstants.h"

#include <QDate>
#include <QPointer>
#include <QTreeView>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Core { class IEditor; }
namespace TextEditor { class BaseTextEditor; class TextEditorWidget; }

namespace Debugger {
class DebuggerEngine;

namespace Internal {

class DebuggerToolTipContext
{
public:
    DebuggerToolTipContext();
    bool isValid() const { return !expression.isEmpty(); }
    bool matchesFrame(const QString &frameFile, const QString &frameFunction) const;
    bool isSame(const DebuggerToolTipContext &other) const;

    QString fileName;
    int position;
    int line;
    int column;
    QString function; //!< Optional function. This must be set by the engine as it is language-specific.
    QString engineType;
    QDate creationDate;

    QPoint mousePosition;
    QString expression;
    QByteArray iname;
};

typedef QList<DebuggerToolTipContext> DebuggerToolTipContexts;


QDebug operator<<(QDebug, const DebuggerToolTipContext &);

class DebuggerToolTipTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit DebuggerToolTipTreeView(QWidget *parent = 0);

    QAbstractItemModel *swapModel(QAbstractItemModel *model);
    QSize sizeHint() const { return m_size; }

private slots:
    void computeSize();
    void expandNode(const QModelIndex &idx);
    void collapseNode(const QModelIndex &idx);

private:
    int computeHeight(const QModelIndex &index) const;

    QSize m_size;
};

class DebuggerToolTipManager : public QObject
{
    Q_OBJECT

public:
    explicit DebuggerToolTipManager(QObject *parent = 0);
    ~DebuggerToolTipManager();

    static void registerEngine(DebuggerEngine *engine);
    static void deregisterEngine(DebuggerEngine *engine);
    static void updateEngine(DebuggerEngine *engine);
    static bool hasToolTips();

    // Collect all expressions of DebuggerTreeViewToolTipWidget
    static DebuggerToolTipContexts treeWidgetExpressions(DebuggerEngine *engine,
        const QString &fileName, const QString &function = QString());

    static void showToolTip(const DebuggerToolTipContext &context,
                            DebuggerEngine *engine);

    virtual bool eventFilter(QObject *, QEvent *);

    static QString treeModelClipboardContents(const QAbstractItemModel *model);

public slots:
    void debugModeEntered();
    void leavingDebugMode();
    void sessionAboutToChange();
    static void loadSessionData();
    static void saveSessionData();
    static void closeAllToolTips();
    static void hide();

private slots:
    static void slotUpdateVisibleToolTips();
    void slotDebuggerStateChanged(Debugger::DebuggerState);
    void slotEditorOpened(Core::IEditor *);
    void slotTooltipOverrideRequested(TextEditor::TextEditorWidget *editorWidget,
            const QPoint &point, int pos, bool *handled);

private:
    bool tryHandleToolTipOverride(TextEditor::TextEditorWidget *editorWidget,
            const QPoint &point, int pos);
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_INTERNAL_DEBUGGERTOOLTIPMANAGER_H
