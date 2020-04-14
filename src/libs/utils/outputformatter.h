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
#include "optional.h"
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

class QTCREATOR_UTILS_EXPORT OutputFormatter : public QObject
{
public:
    OutputFormatter();
    ~OutputFormatter() override;

    QPlainTextEdit *plainTextEdit() const;
    void setPlainTextEdit(QPlainTextEdit *plainText);

    void setFormatters(const QList<OutputFormatter *> &formatters);

    void flush();

    void appendMessage(const QString &text, OutputFormat format);

    virtual bool handleLink(const QString &href);
    void clear();
    void setBoldFontEnabled(bool enabled);

    // For unit testing only
    void overrideTextCharFormat(const QTextCharFormat &fmt);

protected:
    QTextCharFormat charFormat(OutputFormat format) const;
    static QTextCharFormat linkFormat(const QTextCharFormat &inputFormat, const QString &href);

    enum class Status { Done, InProgress, NotHandled };
    class LinkSpec {
    public:
        LinkSpec() = default;
        LinkSpec(int sp, int l, const QString &t) : startPos(sp), length(l), target(t) {}
        int startPos = -1;
        int length = -1;
        QString target;
    };
    using LinkSpecs = QList<LinkSpec>;
    class Result {
    public:
        Result(Status s, const LinkSpecs &l = {}, const optional<QString> &c = {})
            : status(s), linkSpecs(l), newContent(c) {}
        Status status;
        LinkSpecs linkSpecs;
        optional<QString> newContent; // Hard content override. Only to be used in extreme cases.
    };

private:
    // text contains at most one line feed character, and if it does occur, it's the last character.
    // Either way, the input is to be considered "complete" for formatting purposes.
    void doAppendMessage(const QString &text, OutputFormat format);

    virtual Result handleMessage(const QString &text, OutputFormat format);
    virtual void reset() {}

    void append(const QString &text, const QTextCharFormat &format);
    void initFormats();
    void flushIncompleteLine();
    void dumpIncompleteLine(const QString &line, OutputFormat format);
    void clearLastLine();
    QList<FormattedText> parseAnsi(const QString &text, const QTextCharFormat &format);
    const QList<Utils::FormattedText> linkifiedText(const QList<FormattedText> &text,
                                                    const LinkSpecs &linkSpecs);

    Internal::OutputFormatterPrivate *d;
};

} // namespace Utils
