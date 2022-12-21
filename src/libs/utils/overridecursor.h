// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QCursor>

namespace Utils {

class QTCREATOR_UTILS_EXPORT OverrideCursor
{
public:
    OverrideCursor(const QCursor &cursor);
    ~OverrideCursor();
    void set();
    void reset();
private:
    bool m_set = true;
    QCursor m_cursor;
};

}
