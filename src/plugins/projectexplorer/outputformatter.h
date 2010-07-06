/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef OUTPUTFORMATTER_H
#define OUTPUTFORMATTER_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>
#include <QtCore/QString>

QT_FORWARD_DECLARE_CLASS(QMouseEvent);
QT_FORWARD_DECLARE_CLASS(QPlainTextEdit);
QT_FORWARD_DECLARE_CLASS(QTextCharFormat);

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT OutputFormatter: public QObject
{
    Q_OBJECT

public:
    OutputFormatter(QObject *parent = 0);
    virtual ~OutputFormatter();

    QPlainTextEdit *plainTextEdit() const;
    void setPlainTextEdit(QPlainTextEdit *plainText);

    virtual void appendApplicationOutput(const QString &text, bool onStdErr);
    virtual void appendMessage(const QString &text, bool isError);

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);

protected:
    enum Format {
        NormalMessageFormat = 0,
        ErrorMessageFormat = 1,
        StdOutFormat = 2,
        StdErrFormat = 3,

        NumberOfFormats = 4
    };

    void initFormats();
    void append(const QString &text, Format format);
    void append(const QString &text, const QTextCharFormat &format);

private:
    QPlainTextEdit *m_plainTextEdit;
    QTextCharFormat *m_formats;
};

} // namespace ProjectExplorer

#endif // OUTPUTFORMATTER_H
