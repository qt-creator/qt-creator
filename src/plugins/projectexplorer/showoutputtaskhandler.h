// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "taskhandlers.h"

namespace Core { class IOutputPane; }

namespace ProjectExplorer {
namespace Internal {

class ShowOutputTaskHandler : public ITaskHandler
{
public:
    explicit ShowOutputTaskHandler(Core::IOutputPane *window, const QString &text,
                                   const QString &tooltip, const QString &shortcut);

private:
    bool canHandle(const Task &) const override;
    void handle(const Task &task) override;
    static QAction *createAction(const QString &text, const QString &tooltip,
                                 const QString &shortcut);

    Core::IOutputPane * const m_window;
    const QString m_text;
    const QString m_tooltip;
    const QString m_shortcut;
};

} // namespace Internal
} // namespace ProjectExplorer
