// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QRegularExpression>
#include <QSyntaxHighlighter>

namespace Core {

class CORE_EXPORT SearchResultHighlighter : public QSyntaxHighlighter
{
public:
    explicit SearchResultHighlighter(QTextDocument *parent = nullptr);

    void setSearchExpression(const QRegularExpression &expr);

protected:
    void highlightBlock(const QString &text) override;

private:
    QRegularExpression m_searchExpr;
};

} // namespace Core
