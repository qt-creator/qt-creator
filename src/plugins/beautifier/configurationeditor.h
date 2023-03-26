// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE
class QCompleter;
class QStringListModel;
class QTextDocument;
QT_END_NAMESPACE

namespace Beautifier::Internal {

class AbstractSettings;

class ConfigurationSyntaxHighlighter : public QSyntaxHighlighter
{
public:
    explicit ConfigurationSyntaxHighlighter(QTextDocument *parent);
    void setKeywords(const QStringList &keywords);
    void setCommentExpression(const QRegularExpression &rx);

protected:
    void highlightBlock(const QString &text) override;

private:
    QRegularExpression m_expressionKeyword;
    QRegularExpression m_expressionComment;
    QTextCharFormat m_formatKeyword;
    QTextCharFormat m_formatComment;
};

class ConfigurationEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit ConfigurationEditor(QWidget *parent = nullptr);
    void setSettings(AbstractSettings *settings);
    void setCommentExpression(const QRegularExpression &rx);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

signals:
    void documentationChanged(const QString &word, const QString &documentation);

private:
    void insertCompleterText(const QString &text);
    void updateDocumentation();
    QTextCursor cursorForTextUnderCursor(QTextCursor tc = QTextCursor()) const;

    AbstractSettings *m_settings = nullptr;
    QCompleter *m_completer;
    QStringListModel *m_model;
    ConfigurationSyntaxHighlighter *m_highlighter;
    QString m_lastDocumentation;
};

} // Beautifier::Internal
