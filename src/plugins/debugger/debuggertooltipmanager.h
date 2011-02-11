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

#ifndef DEBUGGER_INTERNAL_DEBUGGERTOOLTIPMANAGER_H
#define DEBUGGER_INTERNAL_DEBUGGERTOOLTIPMANAGER_H

#include "debuggerconstants.h"

#include <QtGui/QTreeView>

#include <QtCore/QPointer>
#include <QtCore/QList>
#include <QtCore/QXmlStreamWriter>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QDate>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QToolButton;
class QSortFilterProxyModel;
class QStandardItemModel;
class QPlainTextEdit;
class QLabel;
class QToolBar;
class QMenu;
class QDebug;
QT_END_NAMESPACE

namespace Core {
class IEditor;
class IMode;
}

namespace Debugger {
class DebuggerEngine;

namespace Internal {

class PinnableToolTipWidget : public QWidget
{
    Q_OBJECT

public:
    enum PinState
    {
        Unpinned,
        Pinned
    };

    explicit PinnableToolTipWidget(QWidget *parent = 0);
    PinState pinState() const  { return m_pinState; }

    void addWidget(QWidget *w);
    void addToolBarWidget(QWidget *w);

public slots:
    void pin();

signals:
    void closeAllRequested();

protected:
    virtual void leaveEvent(QEvent *ev);

private slots:
    void toolButtonClicked();

private:
    PinState m_pinState;
    QVBoxLayout *m_mainVBoxLayout;
    QToolBar *m_toolBar;
    QToolButton *m_toolButton;
    QMenu *m_menu;
};

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

class AbstractDebuggerToolTipWidget : public PinnableToolTipWidget
{
    Q_OBJECT

public:
    explicit AbstractDebuggerToolTipWidget(QWidget *parent = 0);
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

    static AbstractDebuggerToolTipWidget *loadSessionData(QXmlStreamReader &r);

public slots:
    void saveSessionData(QXmlStreamWriter &w) const;

    void acquireEngine(Debugger::DebuggerEngine *engine);
    void releaseEngine();
    bool positionShow(const QPlainTextEdit *pe);

protected:
    virtual void doAcquireEngine(Debugger::DebuggerEngine *engine) = 0;
    virtual void doReleaseEngine() = 0;
    virtual void doSaveSessionData(QXmlStreamWriter &w) const = 0;
    virtual void doLoadSessionData(QXmlStreamReader &r) = 0;

private:
    static AbstractDebuggerToolTipWidget *loadSessionDataI(QXmlStreamReader &r);
    QLabel *m_titleLabel;
    bool m_engineAcquired;
    QString m_engineType;
    DebuggerToolTipContext m_context;
    QDate m_creationDate;
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

class DebuggerTreeViewToolTipWidget : public AbstractDebuggerToolTipWidget
{
    Q_OBJECT

public:
    explicit DebuggerTreeViewToolTipWidget(QWidget *parent = 0);

    int debuggerModel() const { return m_debuggerModel; }
    void setDebuggerModel(int m) { m_debuggerModel = m; }
    QString expression() const { return m_expression; }
    void setExpression(const QString &e) { m_expression = e; }

protected:
    virtual void doAcquireEngine(Debugger::DebuggerEngine *engine);
    virtual void doReleaseEngine();
    virtual void doSaveSessionData(QXmlStreamWriter &w) const;
    virtual void doLoadSessionData(QXmlStreamReader &r);

private:
    static void restoreTreeModel(QXmlStreamReader &r, QStandardItemModel *m);

    int m_debuggerModel;
    QString m_expression;

    DebuggerToolTipTreeView *m_treeView;
    QStandardItemModel *m_defaultModel;
};

class DebuggerToolTipManager : public QObject
{
    Q_OBJECT
public:
    explicit DebuggerToolTipManager(QObject *parent = 0);
    virtual ~DebuggerToolTipManager();

    static DebuggerToolTipManager *instance() { return m_instance; }
    void registerEngine(DebuggerEngine *engine);

    // Collect all expressions of DebuggerTreeViewToolTipWidget
    QStringList treeWidgetExpressions(const QString &fileName,
                                      const QString &engineType = QString(),
                                      const QString &function= QString()) const;

    void add(const QPoint &p, AbstractDebuggerToolTipWidget *w);

    virtual bool eventFilter(QObject *, QEvent *);

    static bool debug();

signals:

public slots:
    void debugModeEntered();
    void leavingDebugMode();
    void sessionAboutToChange();
    void loadSessionData();
    void saveSessionData();
    void closeAllToolTips();
    void closeUnpinnedToolTips();
    void hide();

private slots:
    void slotUpdateVisibleToolTips();
    void slotDebuggerStateChanged(Debugger::DebuggerState);
    void slotStackFrameCompleted();
    void slotEditorOpened(Core::IEditor *);

private:
    typedef QList<QPointer<AbstractDebuggerToolTipWidget> > DebuggerToolTipWidgetList;

    inline bool isActive() const { return !m_tooltips.isEmpty(); }
    void add(AbstractDebuggerToolTipWidget *toolTipWidget);
    void moveToolTipsBy(const QPoint &distance);
    // Purge out closed (null) tooltips and return list for convenience
    DebuggerToolTipWidgetList &purgeClosedToolTips();

    static DebuggerToolTipManager *m_instance;

    DebuggerToolTipWidgetList m_tooltips;
    bool m_debugModeActive;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_INTERNAL_DEBUGGERTOOLTIPMANAGER_H
