// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QMenu>

namespace QmlDesigner {

class Edit3DToolbarMenu : public QMenu
{
    Q_OBJECT

public:
    explicit Edit3DToolbarMenu(QWidget *parent = nullptr);

protected:
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
};

} // namespace QmlDesigner
