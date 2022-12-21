// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/itemviews.h>

namespace Todo {
namespace Internal {

class TodoOutputTreeView : public Utils::TreeView
{
public:
    explicit TodoOutputTreeView(QWidget *parent = nullptr);
    ~TodoOutputTreeView() override;

    void resizeEvent(QResizeEvent *event) override;

private:
    void todoColumnResized(int column, int oldSize, int newSize);

    void saveDisplaySettings();
    void loadDisplaySettings();

    qreal m_textColumnDefaultWidth = 0.0;
    qreal m_fileColumnDefaultWidth = 0.0;
};

} // namespace Internal
} // namespace Todo
