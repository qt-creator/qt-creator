// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itaskhandler.h"

namespace Core { class IOutputPane; }

namespace ProjectExplorer {
namespace Internal {

class ShowOutputTaskHandler : public ITaskHandler
{
    Q_OBJECT

public:
    explicit ShowOutputTaskHandler(Core::IOutputPane *window, const QString &text,
                                   const QString &tooltip, const QString &shortcut);

    bool canHandle(const Task &) const override;
    void handle(const Task &task) override;
    QAction *createAction(QObject *parent) const override;

private:
    Core::IOutputPane * const m_window;
    const QString m_text;
    const QString m_tooltip;
    const QString m_shortcut;
};

} // namespace Internal
} // namespace ProjectExplorer
