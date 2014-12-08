/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TIPCONTENTS_H
#define TIPCONTENTS_H

#include "../utils_global.h"

#include <QString>
#include <QColor>
#include <QWidget>

namespace Utils {

class QTCREATOR_UTILS_EXPORT TipContent
{
public:
    TipContent();
    explicit TipContent(const QString &text);
    explicit TipContent(const QColor &color);
    explicit TipContent(QWidget *w, bool interactive);

    int typeId() const { return m_typeId; }
    bool isValid() const;
    bool isInteractive() const { return m_interactive; }
    int showTime() const;
    bool equals(const TipContent &other) const;

    const QString &text() const { return m_text; }
    const QColor &color() const { return m_color; }
    QWidget *widget() const { return m_widget; }

    enum {
        COLOR_CONTENT_ID = 0,
        TEXT_CONTENT_ID = 1,
        WIDGET_CONTENT_ID = 42
    };

    void setInteractive(bool i) { m_interactive = i; }
    // Helper to 'pin' (show as real window) a tooltip shown
    // using WidgetContent
    static bool pinToolTip(QWidget *w, QWidget *parent);

private:
    int m_typeId;
    QString m_text;
    QColor m_color;
    QWidget *m_widget;
    bool m_interactive;
};

} // namespace Utils

#endif // TIPCONTENTS_H
