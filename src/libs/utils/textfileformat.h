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

QT_BEGIN_NAMESPACE
class QTextCodec;
class QStringList;
class QString;
class QByteArray;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT TextFileFormat  {
public:
    enum LineTerminationMode
    {
        LFLineTerminator,
        CRLFLineTerminator,
        NativeLineTerminator =
#if defined (Q_OS_WIN)
        CRLFLineTerminator,
#else
        LFLineTerminator
#endif
    };

    enum ReadResult
    {
        ReadSuccess,
        ReadEncodingError,
        ReadMemoryAllocationError,
        ReadIOError
    };

    TextFileFormat();

    static TextFileFormat detect(const QByteArray &data);

    bool decode(const QByteArray &data, QString *target) const;
    bool decode(const QByteArray &data, QStringList *target) const;

    static ReadResult readFile(const QString &fileName, const QTextCodec *defaultCodec,
                               QStringList *plainText, TextFileFormat *format, QString *errorString,
                               QByteArray *decodingErrorSample = 0);
    static ReadResult readFile(const QString &fileName, const QTextCodec *defaultCodec,
                               QString *plainText, TextFileFormat *format, QString *errorString,
                               QByteArray *decodingErrorSample = 0);
    static ReadResult readFileUTF8(const QString &fileName, const QTextCodec *defaultCodec,
                                   QByteArray *plainText, QString *errorString);

    bool writeFile(const QString &fileName, QString plainText, QString *errorString) const;

    static QByteArray decodingErrorSample(const QByteArray &data);

    LineTerminationMode lineTerminationMode;
    bool hasUtf8Bom;
    const QTextCodec *codec;
};

} // namespace Utils
