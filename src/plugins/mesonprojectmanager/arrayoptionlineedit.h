// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QColor>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

namespace MesonProjectManager {
namespace Internal {

class RegexHighlighter : public QSyntaxHighlighter
{
    const QRegularExpression m_regex{R"('([^']+)'+|([^', ]+)[, ]*)"};
    QTextCharFormat m_format;

public:
    RegexHighlighter(QWidget *parent);
    void highlightBlock(const QString &text) override;
    QStringList options(const QString &text);
};

class ArrayOptionLineEdit : public QPlainTextEdit
{
    Q_OBJECT
    RegexHighlighter *m_highLighter;

public:
    ArrayOptionLineEdit(QWidget *parent = nullptr);
    QStringList options();

protected:
    void keyPressEvent(QKeyEvent *e) override;
};

} // namespace Internal
} // namespace MesonProjectManager
