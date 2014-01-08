/****************************************************************************
**
** Copyright (C) 2014 Petar Perisin <petar.perisin@gmail.com>
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


#ifndef UTILS_ANSIESCAPECODEHANDLER_H
#define UTILS_ANSIESCAPECODEHANDLER_H

#include "utils_global.h"

#include <QTextCharFormat>

namespace Utils {

typedef QPair<QString, QTextCharFormat> StringFormatPair;

class QTCREATOR_UTILS_EXPORT AnsiEscapeCodeHandler
{

enum AnsiEscapeCodes {
    ResetFormat            =  0,
    BoldText               =  1,
    TextColorStart         = 30,
    TextColorEnd           = 37,
    RgbTextColor           = 38,
    DefaultTextColor       = 39,
    BackgroundColorStart   = 40,
    BackgroundColorEnd     = 47,
    RgbBackgroundColor     = 48,
    DefaultBackgroundColor = 49
};

public:
    AnsiEscapeCodeHandler();
    QList<StringFormatPair> parseText(const QString &text, const QTextCharFormat &defaultFormat);
    void endFormatScope();

private:
    void setFormatScope(const QTextCharFormat &charFormat);

    bool            m_previousFormatClosed;
    QTextCharFormat m_previousFormat;
};

} // namespace Utils

#endif // UTILS_ANSIESCAPECODEHANDLER_H
