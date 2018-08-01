/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "debugger_global.h"

#include <utils/fancymainwindow.h>
#include <utils/statuslabel.h>

#include <QHBoxLayout>
#include <QPointer>
#include <QSet>

#include <functional>

QT_BEGIN_NAMESPACE
class QComboBox;
class QToolButton;
class QStackedWidget;
QT_END_NAMESPACE

namespace Core { class Id; }

namespace Utils {

class DEBUGGER_EXPORT Perspective
{
public:
    enum OperationType { SplitVertical, SplitHorizontal, AddToTab, Raise };

    Perspective() = default;
    explicit Perspective(const QString &name);
    ~Perspective();

    void setCentralWidget(QWidget *centralWidget);
    void addWindow(QWidget *widget,
                   OperationType op,
                   QWidget *anchorWidget,
                   bool visibleByDefault = true,
                   Qt::DockWidgetArea area = Qt::BottomDockWidgetArea);

    QToolButton *addToolbarAction(QAction *action, const QIcon &toolbarIcon = QIcon());
    void addToolbarWidget(QWidget *widget);
    void addToolbarSeparator();

    QWidget *centralWidget() const;

    QString name() const;
    void setName(const QString &name);

    using Callback = std::function<void()>;
    void setAboutToActivateCallback(const Callback &cb);
    void aboutToActivate() const;

    QByteArray parentPerspective() const;
    void setParentPerspective(const QByteArray &parentPerspective);

private:
    Perspective(const Perspective &) = delete;
    void operator=(const Perspective &) = delete;

    friend class DebuggerMainWindow;
    class PerspectivePrivate *d = nullptr;
    class Operation
    {
    public:
        QPointer<QWidget> widget;
        QByteArray anchorDockId;
        OperationType operationType = Raise;
        bool visibleByDefault = true;
        Qt::DockWidgetArea area = Qt::BottomDockWidgetArea;
    };

    class ToolbarOperation
    {
    public:
        QPointer<QWidget> widget; // Owned by plugin if present
        QPointer<QAction> action; // Owned by plugin if present
        QPointer<QToolButton> toolbutton; // Owned here in case action is used
        QPointer<QWidget> separator;
        QIcon icon;
    };

    QByteArray m_id;
    QString m_name;
    QByteArray m_parentPerspective;
    QVector<Operation> m_operations;
    QPointer<QWidget> m_centralWidget;
    Callback m_aboutToActivateCallback;
    QVector<ToolbarOperation> m_toolbarOperations;
};

class DEBUGGER_EXPORT DebuggerMainWindow : public FancyMainWindow
{
    Q_OBJECT

public:
    DebuggerMainWindow();
    ~DebuggerMainWindow() override;

    void registerPerspective(const QByteArray &perspectiveId, Perspective *perspective);
    void destroyDynamicPerspective(Perspective *perspective);

    void resetCurrentPerspective();
    void restorePerspective(Perspective *perspective);

    void finalizeSetup();

    void showStatusMessage(const QString &message, int timeoutMS);
    void raiseDock(const QByteArray &dockId);
    QByteArray currentPerspective() const;
    QStackedWidget *centralWidgetStack() const { return m_centralWidgetStack; }

    void onModeChanged(Core::Id mode);

    void setPerspectiveEnabled(const QByteArray &perspectiveId, bool enabled);

    Perspective *findPerspective(const QByteArray &perspectiveId) const;

private:
    void closeEvent(QCloseEvent *) final { savePerspectiveHelper(m_currentPerspective); }

    void loadPerspectiveHelper(Perspective *perspective, bool fromStoredSettings = true);
    void savePerspectiveHelper(const Perspective *perspective);
    void increaseChooserWidthIfNecessary(const QString &visibleName);
    int indexInChooser(Perspective *perspective) const;

    Perspective *m_currentPerspective = nullptr;
    QComboBox *m_perspectiveChooser = nullptr;
    QHBoxLayout *m_toolbuttonBoxLayout = nullptr;
    QStackedWidget *m_centralWidgetStack = nullptr;
    QWidget *m_editorPlaceHolder = nullptr;
    Utils::StatusLabel *m_statusLabel = nullptr;
    QDockWidget *m_toolbarDock = nullptr;

    QHash<QByteArray, QDockWidget *> m_dockForDockId;
    QList<Perspective *> m_perspectives;
};

DEBUGGER_EXPORT QWidget *createModeWindow(const Core::Id &mode, DebuggerMainWindow *mainWindow);

} // Utils
