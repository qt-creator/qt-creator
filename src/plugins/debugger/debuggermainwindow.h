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

#ifndef DEBUGGERMAINWINDOW_H
#define DEBUGGERMAINWINDOW_H

#include "debugger_global.h"

#include <utils/fancymainwindow.h>
#include <utils/statuslabel.h>

#include <QPointer>
#include <QSet>

#include <functional>

QT_BEGIN_NAMESPACE
class QComboBox;
class QStackedWidget;
QT_END_NAMESPACE

namespace Core { class IMode; }

namespace Utils {

class DEBUGGER_EXPORT Perspective
{
public:
    enum OperationType { SplitVertical, SplitHorizontal, AddToTab, Raise };

    class DEBUGGER_EXPORT Operation
    {
    public:
        Operation() = default;
        Operation(const QByteArray &dockId, QWidget *widget,
                  const QByteArray &anchorDockId,
                  OperationType operationType,
                  bool visibleByDefault = true,
                  Qt::DockWidgetArea area = Qt::BottomDockWidgetArea);

        QByteArray dockId;
        QPointer<QWidget> widget;
        QByteArray anchorDockId;
        OperationType operationType;
        bool visibleByDefault;
        Qt::DockWidgetArea area;
    };

    Perspective() = default;
    Perspective(const QString &name, const QVector<Operation> &operations);

    void addOperation(const Operation &operation);

    QVector<Operation> operations() const { return m_operations; }
    QVector<QByteArray> docks() const { return m_docks; }

    QString name() const;
    void setName(const QString &name);

private:
    QString m_name;
    QVector<QByteArray> m_docks;
    QVector<Operation> m_operations;
};

class DEBUGGER_EXPORT ToolbarDescription
{
public:
    ToolbarDescription() = default;
    ToolbarDescription(const QList<QWidget *> &widgets) : m_widgets(widgets) {}

    QList<QWidget *> widgets() const;

    void addAction(QAction *action);
    void addWidget(QWidget *widget);

private:
    QList<QWidget *> m_widgets;
};

class DEBUGGER_EXPORT DebuggerMainWindow : public FancyMainWindow
{
    Q_OBJECT

public:
    DebuggerMainWindow();
    ~DebuggerMainWindow() override;

    void registerPerspective(const QByteArray &perspectiveId, const Perspective &perspective);
    void registerToolbar(const QByteArray &perspectiveId, QWidget *widget);

    void saveCurrentPerspective();
    void resetCurrentPerspective();
    void restorePerspective(const QByteArray &perspectiveId);

    void finalizeSetup();

    void showStatusMessage(const QString &message, int timeoutMS);
    QDockWidget *dockWidget(const QByteArray &dockId) const;
    QByteArray currentPerspective() const { return m_currentPerspectiveId; }

private:
    QDockWidget *registerDockWidget(const QByteArray &dockId, QWidget *widget);
    void loadPerspectiveHelper(const QByteArray &perspectiveId, bool fromStoredSettings = true);

    QByteArray m_currentPerspectiveId;
    QComboBox *m_perspectiveChooser;
    QStackedWidget *m_controlsStackWidget;
    Utils::StatusLabel *m_statusLabel;
    QDockWidget *m_toolbarDock;

    QHash<QByteArray, QDockWidget *> m_dockForDockId;
    QHash<QByteArray, QWidget *> m_toolbarForPerspectiveId;
    QHash<QByteArray, Perspective> m_perspectiveForPerspectiveId;
};

QWidget *createModeWindow(Core::IMode *mode, DebuggerMainWindow *mainWindow, QWidget *central);

} // Utils

#endif // DEBUGGERMAINWINDOW_H
