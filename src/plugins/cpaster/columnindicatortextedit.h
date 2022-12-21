// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QTextEdit>

namespace CodePaster {

// Indicate text column 100 by a vertical line.
class ColumnIndicatorTextEdit : public QTextEdit
{
public:
    ColumnIndicatorTextEdit();

    int columnIndicator() const { return m_columnIndicator; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_columnIndicator = 0;
    QFont m_columnIndicatorFont;
};

} // namespace CodePaster
