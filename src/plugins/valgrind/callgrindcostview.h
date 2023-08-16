// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "callgrindcostdelegate.h"

#include <utils/basetreeview.h>

namespace Valgrind::Internal {

class CostDelegate;
class NameDelegate;

class CostView : public Utils::BaseTreeView
{
    Q_OBJECT

public:
    explicit CostView(QWidget *parent = nullptr);
    ~CostView() override;

    /**
     * Overload automatically updates the cost delegate
     * and sets it for the cost columns of DataModel and CallModel.
     */
    void setModel(QAbstractItemModel *model) override;

    /**
     * How to format cost data columns in the view.
     */
    void setCostFormat(CostDelegate::CostFormat format);
    CostDelegate::CostFormat costFormat() const;

private:
    CostDelegate *m_costDelegate;
    NameDelegate *m_nameDelegate;
};

} // namespace Valgrind::Internal
