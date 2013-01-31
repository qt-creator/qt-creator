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

#ifndef MARKUP_H
#define MARKUP_H

#include <QColor>
#include <QString>
#include <QList>
#include <QMetaType>

namespace BINEditor {
/*!
    \class BINEditor::Markup
    \brief Markup range of the binary editor.

    Used for displaying class layouts by the debugger.

    \note Must not have linkage - used for soft dependencies.

    \sa Debugger::Internal::MemoryAgent
*/

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
} // namespace BINEditor

Q_DECLARE_METATYPE(QList<BINEditor::Markup>)

#endif // MARKUP_H
