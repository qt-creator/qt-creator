/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "utils_global.h"
#include "outputformat.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextCharFormat;
class QTextCursor;
QT_END_NAMESPACE

namespace Utils {

class FormattedText;

namespace Internal { class OutputFormatterPrivate; }

class QTCREATOR_UTILS_EXPORT AggregatingOutputFormatter;

class QTCREATOR_UTILS_EXPORT OutputFormatter : public QObject
{
    friend class AggregatingOutputFormatter;
public:
    OutputFormatter();
    ~OutputFormatter() override;

    QPlainTextEdit *plainTextEdit() const;
    void setPlainTextEdit(QPlainTextEdit *plainText);

    void flush();

    void appendMessage(const QString &text, OutputFormat format);

    virtual bool handleLink(const QString &href);
    void clear();
    void setBoldFontEnabled(bool enabled);
    static QTextCharFormat linkFormat(const QTextCharFormat &inputFormat, const QString &href);

    // For unit testing only
    void overrideTextCharFormat(const QTextCharFormat &fmt);

protected:
    enum class Status { Done, InProgress, NotHandled };

    void appendMessageDefault(const QString &text, OutputFormat format);
    void clearLastLine();
    QTextCharFormat charFormat(OutputFormat format) const;
    QList<FormattedText> parseAnsi(const QString &text, const QTextCharFormat &format);
    QTextCursor &cursor() const;

private:
    // text contains at most one line feed character, and if it does occur, it's the last character.
    // Either way, the input is to be considered "complete" for formatting purposes.
    void doAppendMessage(const QString &text, OutputFormat format);

    virtual Status handleMessage(const QString &text, OutputFormat format);
    virtual void reset() {}

    void doAppendMessage(const QString &text, const QTextCharFormat &format);
    void append(const QString &text, const QTextCharFormat &format);
    void initFormats();
    void flushIncompleteLine();
    void dumpIncompleteLine(const QString &line, OutputFormat format);

    Internal::OutputFormatterPrivate *d;
};

class QTCREATOR_UTILS_EXPORT AggregatingOutputFormatter : public OutputFormatter
{
public:
    AggregatingOutputFormatter();
    ~AggregatingOutputFormatter();

    void setFormatters(const QList<OutputFormatter *> &formatters);
    bool handleLink(const QString &href) override;

private:
    Status handleMessage(const QString &text, OutputFormat format) override;

    class Private;
    Private * const d;
};

} // namespace Utils
