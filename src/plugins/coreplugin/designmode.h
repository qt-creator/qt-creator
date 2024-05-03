// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imode.h"

namespace Utils {
class FancyMainWindow;
}

namespace Core {
class IEditor;

class CORE_EXPORT DesignMode final : public IMode
{
    Q_OBJECT

public:
    static DesignMode *instance();

    static void setDesignModeIsRequired();

    static void registerDesignWidget(QWidget *widget,
                                     const QStringList &mimeTypes,
                                     const Context &context,
                                     Utils::FancyMainWindow *mainWindow = nullptr);
    static void unregisterDesignWidget(QWidget *widget);

    static void createModeIfRequired();
    static void destroyModeIfRequired();

signals:
    void actionsUpdated(Core::IEditor *editor);

private:
    DesignMode();
    ~DesignMode() final;

    void updateActions();

    void currentEditorChanged(IEditor *editor);
    void updateContext(Utils::Id newMode, Utils::Id oldMode);
    void setActiveContext(const Context &context);
};

} // namespace Core
