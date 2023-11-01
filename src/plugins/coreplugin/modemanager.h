// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/id.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {

class ICore;
class IMode;

namespace Internal {
class FancyTabWidget;
class ICorePrivate;
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
    static Utils::Id currentModeId();

    static void addAction(QAction *action, int priority);
    static void addProjectSelector(QAction *action);

    static void activateMode(Utils::Id id);
    static void setFocusToCurrentMode();
    static Style modeStyle();

    static void removeMode(IMode *mode);

    static void setModeStyle(Style layout);
    static void cycleModeStyle();

signals:
    void currentModeAboutToChange(Utils::Id mode);

    // the default argument '=0' is important for connects without the oldMode argument.
    void currentModeChanged(Utils::Id mode, Utils::Id oldMode = {});

private:
    explicit ModeManager(Internal::FancyTabWidget *modeStack);
    ~ModeManager() override;

    static void extensionsInitialized();

    static void addMode(IMode *mode);
    void currentTabAboutToChange(int index);
    void currentTabChanged(int index);

    friend class ICore;
    friend class IMode;
    friend class Internal::ICorePrivate;
};

} // namespace Core
