/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef OUTPUTFORMATTER_H
#define OUTPUTFORMATTER_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>

QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)
QT_FORWARD_DECLARE_CLASS(QTextCharFormat)
QT_FORWARD_DECLARE_CLASS(QColor)

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT OutputFormatter: public QObject
{
    Q_OBJECT

public:
    OutputFormatter();
    virtual ~OutputFormatter();

    QPlainTextEdit *plainTextEdit() const;
    void setPlainTextEdit(QPlainTextEdit *plainText);

    virtual void appendApplicationOutput(const QString &text, bool onStdErr);
    virtual void appendMessage(const QString &text, bool isError);

    virtual void handleLink(const QString &href);

protected:
    enum Format {
        NormalMessageFormat = 0,
        ErrorMessageFormat = 1,
        StdOutFormat = 2,
        StdErrFormat = 3,

        NumberOfFormats = 4
    };

    void initFormats();
    void clearLastLine();
    QTextCharFormat format(Format format);

    static QColor mixColors(const QColor &a, const QColor &b);

private:
    QPlainTextEdit *m_plainTextEdit;
    QTextCharFormat *m_formats;
};

} // namespace ProjectExplorer

#endif // OUTPUTFORMATTER_H
