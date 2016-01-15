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

// WARNING: This code is shared with the qmlplugindump tool code in Qt.
//          Modifications to this file need to be applied there.

#ifndef QMLSTREAMWRITER_H
#define QMLSTREAMWRITER_H

#include <QIODevice>
#include <QList>
#include <QString>
#include <QScopedPointer>
#include <QPair>

class QmlStreamWriter
{
public:
    QmlStreamWriter(QByteArray *array);

    void writeStartDocument();
    void writeEndDocument();
    void writeLibraryImport(const QString &uri, int majorVersion, int minorVersion, const QString &as = QString());
    //void writeFilesystemImport(const QString &file, const QString &as = QString());
    void writeStartObject(const QString &component);
    void writeEndObject();
    void writeScriptBinding(const QString &name, const QString &rhs);
    void writeScriptObjectLiteralBinding(const QString &name, const QList<QPair<QString, QString> > &keyValue);
    void writeArrayBinding(const QString &name, const QStringList &elements);
    void write(const QString &data);

private:
    void writeIndent();
    void writePotentialLine(const QByteArray &line);
    void flushPotentialLinesWithNewlines();

    int m_indentDepth;
    QList<QByteArray> m_pendingLines;
    int m_pendingLineLength;
    bool m_maybeOneline;
    QScopedPointer<QIODevice> m_stream;
};

#endif // QMLSTREAMWRITER_H
