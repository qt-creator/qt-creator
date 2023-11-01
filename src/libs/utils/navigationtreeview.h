// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "itemviews.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT NavigationTreeView : public TreeView
{
public:
    explicit NavigationTreeView(QWidget *parent = nullptr);
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

} // Utils
