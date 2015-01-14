/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef MARKUP_H
#define MARKUP_H

#include <QColor>
#include <QString>
#include <QList>
#include <QMetaType>

namespace BinEditor {

class Markup
{
public:
    Markup(quint64 a = 0, quint64 l = 0, QColor c = Qt::yellow, const QString &tt = QString()) :
        address(a), length(l), color(c), toolTip(tt) {}
    bool covers(quint64 a) const { return a >= address && a < (address + length); }

    quint64 address;
    quint64 length;
    QColor color;
    QString toolTip;
};

} // namespace BinEditor

Q_DECLARE_METATYPE(QList<BinEditor::Markup>)

#endif // MARKUP_H
