// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imode.h"

namespace Core {
class IEditor;

/**
  * A global mode for Design pane - used by Bauhaus (QML Designer) and
  * Qt Designer. Other plugins can register themselves by registerDesignWidget()
  * and giving a list of mimetypes that the editor understands, as well as an instance
  * to the main editor widget itself.
  */

class CORE_EXPORT DesignMode final : public IMode
{
    Q_OBJECT

public:
    static DesignMode *instance();

    static void setDesignModeIsRequired();

    static void registerDesignWidget(QWidget *widget,
                                     const QStringList &mimeTypes,
                                     const Context &context);
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
