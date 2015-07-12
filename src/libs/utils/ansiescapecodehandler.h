/****************************************************************************
**
** Copyright (C) 2015 Petar Perisin <petar.perisin@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


#ifndef UTILS_ANSIESCAPECODEHANDLER_H
#define UTILS_ANSIESCAPECODEHANDLER_H

#include "utils_global.h"

#include <QTextCharFormat>

namespace Utils {

class QTCREATOR_UTILS_EXPORT FormattedText {
public:
    FormattedText() { }
    FormattedText(const FormattedText &other) : text(other.text), format(other.format) { }
    FormattedText(const QString &txt, const QTextCharFormat &fmt = QTextCharFormat()) :
        text(txt), format(fmt)
    { }

    QString text;
    QTextCharFormat format;
};

class QTCREATOR_UTILS_EXPORT AnsiEscapeCodeHandler
{
public:
    AnsiEscapeCodeHandler();
    QList<FormattedText> parseText(const FormattedText &input);
    void endFormatScope();

private:
    void setFormatScope(const QTextCharFormat &charFormat);

    bool            m_previousFormatClosed;
    QTextCharFormat m_previousFormat;
    QString         m_pendingText;
};

} // namespace Utils

#endif // UTILS_ANSIESCAPECODEHANDLER_H
