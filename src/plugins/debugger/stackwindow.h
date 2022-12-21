// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/basetreeview.h>

namespace Debugger::Internal {

class StackTreeView : public Utils::BaseTreeView
{
public:
    explicit StackTreeView(QWidget *parent = nullptr);

private:
    void setModel(QAbstractItemModel *model) override;

    void showAddressColumn(bool on);
    void adjustForContents(bool refreshSpan = false);

    bool m_contentsAdjusted = false;
};

} // Debugger::Internal
