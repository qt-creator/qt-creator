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

#include <coreplugin/id.h>
#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {

class IMode;

namespace Internal {
    class MainWindow;
    class FancyTabWidget;
}

class CORE_EXPORT ModeManager : public QObject
{
    Q_OBJECT

public:
    enum class Style {
        IconsAndText,
        IconsOnly,
        Hidden
    };

    static ModeManager *instance();

    static IMode *currentMode();
    static Id currentModeId();

    static void addAction(QAction *action, int priority);
    static void addProjectSelector(QAction *action);

    static void activateMode(Id id);
    static void setFocusToCurrentMode();
    static Style modeStyle();

public slots:
    static void setModeStyle(Style layout);
    static void cycleModeStyle();

signals:
    void currentModeAboutToChange(Core::Id mode);

    // the default argument '=0' is important for connects without the oldMode argument.
    void currentModeChanged(Core::Id mode, Core::Id oldMode = Core::Id());

private:
    explicit ModeManager(Internal::MainWindow *mainWindow, Internal::FancyTabWidget *modeStack);
    ~ModeManager() override;

    static void extensionsInitialized();

    static void addMode(IMode *mode);
    static void removeMode(IMode *mode);
    void currentTabAboutToChange(int index);
    void currentTabChanged(int index);

    friend class IMode;
    friend class Core::Internal::MainWindow;
};

} // namespace Core
