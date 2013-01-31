/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QTreeView>

#include <QPointer>
#include <QPoint>
#include <QList>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QDate>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QToolButton;
class QStandardItemModel;
class QToolBar;
class QDebug;
QT_END_NAMESPACE

namespace Core {
class IEditor;
class IMode;
}

namespace TextEditor {
class ITextEditor;
}

namespace Debugger {
class DebuggerEngine;

namespace Internal {
class DraggableLabel;
class DebuggerToolTipEditor;

class DebuggerToolTipContext
{
public:
    DebuggerToolTipContext();
    static DebuggerToolTipContext fromEditor(Core::IEditor *ed, int pos);
    bool isValid() const { return !fileName.isEmpty(); }

    QString fileName;
    int position;
    int line;
    int column;
    QString function; //!< Optional function. This must be set by the engine as it is language-specific.
};

QDebug operator<<(QDebug, const DebuggerToolTipContext &);

class DebuggerToolTipTreeView;

class DebuggerToolTipWidget : public QWidget
{
    Q_OBJECT

public:
    bool isPinned() const  { return m_isPinned; }

    void addWidget(QWidget *w);
    void addToolBarWidget(QWidget *w);

    explicit DebuggerToolTipWidget(QWidget *parent = 0);
    bool engineAcquired() const { return m_engineAcquired; }

    QString fileName() const { return m_context.fileName; }
    QString function() const { return m_context.function; }
    int position() const { return m_context.position; }
    // Check for a match at position.
    bool matches(const QString &fileName,
                 const QString &engineType = QString(),
                 const QString &function= QString()) const;

    const DebuggerToolTipContext &context() const { return m_context; }
    void setContext(const DebuggerToolTipContext &c) { m_context = c; }

    QString engineType() const { return m_engineType; }
    void setEngineType(const QString &e) { m_engineType = e; }

    QDate creationDate() const { return m_creationDate; }
    void setCreationDate(const QDate &d) { m_creationDate = d; }

    QPoint offset() const { return m_offset; }
    void setOffset(const QPoint &o) { m_offset = o; }

    static DebuggerToolTipWidget *loadSessionData(QXmlStreamReader &r);

    QByteArray iname() const { return m_iname; }
    void setIname(const QByteArray &e) { m_iname = e; }

    QString expression() const { return m_expression; }
    void setExpression(const QString &e) { m_expression = e; }

    static QString treeModelClipboardContents(const QAbstractItemModel *m);

public slots:
    void saveSessionData(QXmlStreamWriter &w) const;

    void acquireEngine(Debugger::DebuggerEngine *engine);
    void releaseEngine();
    void copy();
    bool positionShow(const DebuggerToolTipEditor &pe);
    void pin();

private slots:
    void slotDragged(const QPoint &p);
    void toolButtonClicked();

private:
    bool m_isPinned;
    QVBoxLayout *m_mainVBoxLayout;
    QToolBar *m_toolBar;
    QToolButton *m_toolButton;

private:
    static DebuggerToolTipWidget *loadSessionDataI(QXmlStreamReader &r);
    void doAcquireEngine(Debugger::DebuggerEngine *engine);
    void doReleaseEngine();
    void doSaveSessionData(QXmlStreamWriter &w) const;
    void doLoadSessionData(QXmlStreamReader &r);
    QString clipboardContents() const;

    DraggableLabel *m_titleLabel;
    bool m_engineAcquired;
    QString m_engineType;
    DebuggerToolTipContext m_context;
    QDate m_creationDate;
    QPoint m_offset; //!< Offset to text cursor position (user dragging).

private:
    QAbstractItemModel *swapModel(QAbstractItemModel *newModel);
    static void restoreTreeModel(QXmlStreamReader &r, QStandardItemModel *m);

    int m_debuggerModel;
    QString m_expression;
    QByteArray m_iname;

    DebuggerToolTipTreeView *m_treeView;
    QStandardItemModel *m_defaultModel;
};

class DebuggerToolTipTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit DebuggerToolTipTreeView(QWidget *parent = 0);

    QAbstractItemModel *swapModel(QAbstractItemModel *model);
    QSize sizeHint() const { return m_size; }
    int computeHeight(const QModelIndex &index) const;

public slots:
    void computeSize();

private:
    void init(QAbstractItemModel *model);

    QSize m_size;
};

class DebuggerToolTipManager : public QObject
{
    Q_OBJECT

public:
    typedef QPair<QString, QByteArray> ExpressionInamePair;
    typedef QList<ExpressionInamePair> ExpressionInamePairs;

    explicit DebuggerToolTipManager(QObject *parent = 0);
    virtual ~DebuggerToolTipManager();

    static DebuggerToolTipManager *instance() { return m_instance; }
    void registerEngine(DebuggerEngine *engine);
    bool hasToolTips() const { return !m_tooltips.isEmpty(); }

    // Collect all expressions of DebuggerTreeViewToolTipWidget
    ExpressionInamePairs treeWidgetExpressions(const QString &fileName,
                                               const QString &engineType = QString(),
                                               const QString &function= QString()) const;

    void showToolTip(const QPoint &p, Core::IEditor *editor, DebuggerToolTipWidget *);

    virtual bool eventFilter(QObject *, QEvent *);

    static bool debug();

public slots:
    void debugModeEntered();
    void leavingDebugMode();
    void sessionAboutToChange();
    void loadSessionData();
    void saveSessionData();
    void closeAllToolTips();
    void hide();

private slots:
    void slotUpdateVisibleToolTips();
    void slotDebuggerStateChanged(Debugger::DebuggerState);
    void slotStackFrameCompleted();
    void slotEditorOpened(Core::IEditor *);
    void slotTooltipOverrideRequested(TextEditor::ITextEditor *editor,
            const QPoint &point, int pos, bool *handled);

private:
    typedef QList<QPointer<DebuggerToolTipWidget> > DebuggerToolTipWidgetList;

    void registerToolTip(DebuggerToolTipWidget *toolTipWidget);
    void moveToolTipsBy(const QPoint &distance);
    // Purge out closed (null) tooltips and return list for convenience
    void purgeClosedToolTips();

    static DebuggerToolTipManager *m_instance;

    DebuggerToolTipWidgetList m_tooltips;

    bool m_debugModeActive;
    QPoint m_lastToolTipPoint;
    Core::IEditor *m_lastToolTipEditor;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_INTERNAL_DEBUGGERTOOLTIPMANAGER_H
