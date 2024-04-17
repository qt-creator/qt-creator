// Copyright (C) 2018 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"
#include "qmt/model_ui/modeltreefilterdata.h"

#include <QWidget>

namespace qmt {

class QMT_EXPORT ModelTreeFilter : public QWidget
{
    Q_OBJECT
    class Private;

public:
    explicit ModelTreeFilter(QWidget *parent = nullptr);
    ~ModelTreeFilter();

signals:
    void viewDataChanged(const ModelTreeViewData &modelTreeViewData);
    void filterDataChanged(const ModelTreeFilterData &modelTreeFilterData);

private:

    void setupFilter();
    void resetView();
    void clearFilter();
    void onViewChanged();
    void onFilterChanged();

private:
    Private *d = nullptr;
};


} // namespace qmt
