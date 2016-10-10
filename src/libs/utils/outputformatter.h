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
#include <QFont>

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
    Q_OBJECT

public:
    OutputFormatter();
    virtual ~OutputFormatter();

    QPlainTextEdit *plainTextEdit() const;
    virtual void setPlainTextEdit(QPlainTextEdit *plainText);

    void flush();

    virtual void appendMessage(const QString &text, OutputFormat format);
    virtual void appendMessage(const QString &text, const QTextCharFormat &format);
    virtual void handleLink(const QString &href);
    virtual QList<QWidget *> toolbarWidgets() const { return {}; }
    virtual void clear() {}

protected:
    void initFormats();
    virtual void clearLastLine();
    QTextCharFormat charFormat(OutputFormat format) const;
    QList<Utils::FormattedText> parseAnsi(const QString &text, const QTextCharFormat &format);
    void append(QTextCursor &cursor, const QString &text, const QTextCharFormat &format);

private:
    Internal::OutputFormatterPrivate *d;
};

} // namespace Utils
