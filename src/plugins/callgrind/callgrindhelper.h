/**************************************************************************
**
** This file is part of Qt Creator Analyzer Tools
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

#ifndef CALLGRINDHELPER_H
#define CALLGRINDHELPER_H

#include <QtCore/QLocale>

QT_BEGIN_NAMESPACE
class QColor;
class QString;
QT_END_NAMESPACE

namespace Callgrind {
namespace Internal {

namespace CallgrindHelper
{
    /**
     * Returns color for a specific string, the string<->color mapping is cached
     */
    QColor colorForString(const QString &text);

    /**
     * Returns color for a specific cost ratio
     * \param ratio The cost ratio, ratio should be of [0,1]
     */
    QColor colorForCostRatio(qreal ratio);

    /**
     * Returns a proper percent representation of a float limited to 5 chars
     */
    QString toPercent(float costs, const QLocale &locale = QLocale());
}

}
}

#endif // CALLGRINDHELPER_H
