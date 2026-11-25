// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/aiassistance.h>
#include <utils/id.h>

#include <QtTaskTree/QTaskTree>

#include <QToolButton>

#include <functional>

namespace Core {

class AiTaskButtonPrivate;

class CORE_EXPORT AiTaskButton : public QToolButton
{
public:
    using RecipeFactory = std::function<QtTaskTree::Group()>;

    explicit AiTaskButton(QWidget *parent = nullptr);
    ~AiTaskButton() override;

    void setRecipe(const RecipeFactory &recipe);
    void setRequestMode(Utils::AiRequest::Mode mode);
    void setProgress(const QString &title, Utils::Id id);
    void setIdleToolTip(const QString &toolTip);

    bool isRunning() const;
    void start();
    void cancel();

protected:
    bool event(QEvent *event) override;

private:
    void showRunning(bool running);
    void updateVisibility();

    AiTaskButtonPrivate *d = nullptr;
};

} // namespace Core
