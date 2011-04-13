/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QTS60STYLETHEMEIO_H
#define QTS60STYLETHEMEIO_H

#include "qstring.h"

class QS60Style;

class QtS60StyleThemeIO
{
public:
    static bool loadThemeFromBlob(const QString &themeBlob, QS60Style *s60Style = 0);
#ifndef QT_NO_WEBKIT
    static bool loadThemeFromTdf(const QString &themeTdf, QS60Style *s60Style = 0);
    static bool convertTdfToBlob(const QString &themeTdf, const QString &themeBlob);
#endif
};

#endif // QTS60STYLETHEMEIO_H
