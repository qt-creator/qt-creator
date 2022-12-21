// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itaskhandler.h"

#include "task.h"

namespace ProjectExplorer {
namespace Internal {

class ConfigTaskHandler : public ITaskHandler
{
    Q_OBJECT

public:
    ConfigTaskHandler(const Task &pattern, Utils::Id page);

    bool canHandle(const Task &task) const override;
    void handle(const Task &task) override;
    QAction *createAction(QObject *parent) const override;

private:
    const Task m_pattern;
    const Utils::Id m_targetPage;
};

} // namespace Internal
} // namespace ProjectExplorer
