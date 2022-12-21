// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/navigationtreeview.h>

namespace QmlJSEditor {
namespace Internal {

class QmlJSOutlineTreeView : public Utils::NavigationTreeView
{
    Q_OBJECT
public:
    explicit QmlJSOutlineTreeView(QWidget *parent = nullptr);

    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void collapseAllExceptRoot();
};

} // namespace Internal
} // namespace QmlJSEditor
