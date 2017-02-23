/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

namespace Beautifier {
namespace Internal {

class AbstractSettings;

class ConfigurationSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

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

} // namespace Internal
} // namespace Beautifier
