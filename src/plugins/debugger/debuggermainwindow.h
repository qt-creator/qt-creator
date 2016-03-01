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
#include "analyzer/analyzerbase_global.h"

#include <coreplugin/id.h>

#include <utils/fancymainwindow.h>
#include <utils/statuslabel.h>

#include <QPointer>
#include <QSet>

#include <functional>
#include <initializer_list>

QT_BEGIN_NAMESPACE
class QComboBox;
class QStackedWidget;
QT_END_NAMESPACE

namespace Analyzer {

class ANALYZER_EXPORT Perspective
{
public:
    enum SplitType { SplitVertical, SplitHorizontal, AddToTab };

    class Split
    {
    public:
        Split() = default;
        Split(Core::Id dockId, Core::Id existing, SplitType splitType,
              bool visibleByDefault = true,
              Qt::DockWidgetArea area = Qt::BottomDockWidgetArea);

        Core::Id dockId;
        Core::Id existing;
        SplitType splitType;
        bool visibleByDefault;
        Qt::DockWidgetArea area;
    };

    Perspective() = default;
    Perspective(std::initializer_list<Split> splits);

    void addSplit(const Split &split);

    QVector<Split> splits() const { return m_splits; }
    QVector<Core::Id> docks() const { return m_docks; }

private:
    QVector<Core::Id> m_docks;
    QVector<Split> m_splits;
};

} // Analyzer

namespace Debugger {
namespace Internal {

// DebuggerMainWindow dock widget names
const char DOCKWIDGET_BREAK[]         = "Debugger.Docks.Break";
const char DOCKWIDGET_MODULES[]       = "Debugger.Docks.Modules";
const char DOCKWIDGET_REGISTER[]      = "Debugger.Docks.Register";
const char DOCKWIDGET_OUTPUT[]        = "Debugger.Docks.Output";
const char DOCKWIDGET_SNAPSHOTS[]     = "Debugger.Docks.Snapshots";
const char DOCKWIDGET_STACK[]         = "Debugger.Docks.Stack";
const char DOCKWIDGET_SOURCE_FILES[]  = "Debugger.Docks.SourceFiles";
const char DOCKWIDGET_THREADS[]       = "Debugger.Docks.Threads";
const char DOCKWIDGET_WATCHERS[]      = "Debugger.Docks.LocalsAndWatchers";

const char CppPerspectiveId[]         = "Debugger.Perspective.Cpp";
const char QmlPerspectiveId[]         = "Debugger.Perspective.Qml";

class MainWindowBase : public Utils::FancyMainWindow
{
    Q_OBJECT

public:
    MainWindowBase();
    ~MainWindowBase() override;

    QComboBox *toolBox() const { return m_toolBox; }
    QStackedWidget *controlsStack() const { return m_controlsStackWidget; }
    QStackedWidget *statusLabelsStack() const { return m_statusLabelsStackWidget; }

    void registerPerspective(Core::Id perspectiveId, const Analyzer::Perspective &perspective);
    void registerToolbar(Core::Id perspectiveId, QWidget *widget);
    QDockWidget *registerDockWidget(Core::Id dockId, QWidget *widget);

    void saveCurrentPerspective();
    void closeCurrentPerspective();
    void resetCurrentPerspective();
    void restorePerspective(Core::Id perspectiveId);

    void showStatusMessage(Core::Id perspective, const QString &message, int timeoutMS);

    QString lastSettingsName() const;
    void setLastSettingsName(const QString &lastSettingsName);

    Core::Id currentPerspectiveId() const;

private:
    void loadPerspectiveHelper(Core::Id perspectiveId, bool fromStoredSettings = true);

    Core::Id m_currentPerspectiveId;
    QString m_lastSettingsName;
    QComboBox *m_toolBox;
    QStackedWidget *m_controlsStackWidget;
    QStackedWidget *m_statusLabelsStackWidget;
    QHash<Core::Id, QDockWidget *> m_dockForDockId;
    QHash<Core::Id, QWidget *> m_toolbarForPerspectiveId;
    QHash<Core::Id, Analyzer::Perspective> m_perspectiveForPerspectiveId;
    QHash<Core::Id, Utils::StatusLabel *> m_statusLabelForPerspectiveId;

    // list of dock widgets to prevent memory leak
    typedef QPointer<QDockWidget> DockPtr;
    QList<DockPtr> m_dockWidgets;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERMAINWINDOW_H
