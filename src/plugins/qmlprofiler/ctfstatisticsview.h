// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/itemviews.h>

namespace CtfVisualizer::Internal {

class CtfStatisticsModel;

class CtfStatisticsView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit CtfStatisticsView(CtfStatisticsModel *model, QWidget *parent = nullptr);
    ~CtfStatisticsView() override = default;

    void selectByTitle(const QString &title);

signals:
    void eventTypeSelected(const QString &title);
};

}  // CtfVisualizer::Internal
