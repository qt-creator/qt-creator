/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef OUTPUTFORMATTER_H
#define OUTPUTFORMATTER_H

#include "utils_global.h"
#include "outputformat.h"

#include <QObject>
#include <QFont>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextCharFormat;
class QColor;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT OutputFormatter : public QObject
{
    Q_OBJECT

public:
    OutputFormatter();
    virtual ~OutputFormatter();

    QPlainTextEdit *plainTextEdit() const;
    void setPlainTextEdit(QPlainTextEdit *plainText);

    QFont font() const;
    void setFont(const QFont &font);

    virtual void appendMessage(const QString &text, OutputFormat format);
    virtual void handleLink(const QString &href);

protected:
    void initFormats();
    virtual void clearLastLine();
    QTextCharFormat charFormat(OutputFormat format) const;

    static QColor mixColors(const QColor &a, const QColor &b);

private:
    QPlainTextEdit *m_plainTextEdit;
    QTextCharFormat *m_formats;
    QFont m_font;
};

} // namespace Utils

#endif // OUTPUTFORMATTER_H
