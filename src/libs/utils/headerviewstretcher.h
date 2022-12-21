// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QHeaderView;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT HeaderViewStretcher : public QObject
{
    const int m_columnToStretch;
public:
    explicit HeaderViewStretcher(QHeaderView *headerView, int columnToStretch);

    void stretch();
    void softStretch();
    bool eventFilter(QObject *obj, QEvent *ev) override;
};

} // namespace Utils
