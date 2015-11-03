/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#ifndef QMT_DANNOTATION_H
#define QMT_DANNOTATION_H

#include "delement.h"

#include <QPointF>
#include <QRectF>

namespace qmt {

class QMT_EXPORT DAnnotation :
        public DElement
{
public:

    enum VisualRole {
        ROLE_NORMAL,
        ROLE_TITLE,
        ROLE_SUBTITLE,
        ROLE_EMPHASIZED,
        ROLE_SOFTEN,
        ROLE_FOOTNOTE
    };

public:

    DAnnotation();

    DAnnotation(const DAnnotation &);

    ~DAnnotation();

public:

    DAnnotation &operator=(const DAnnotation &);

public:

    Uid getModelUid() const { return Uid::getInvalidUid(); }

    QString getText() const { return m_text; }

    void setText(const QString &text);

    QPointF getPos() const { return m_pos; }

    void setPos(const QPointF &pos);

    QRectF getRect() const { return m_rect; }

    void setRect(const QRectF &rect);

    VisualRole getVisualRole() const { return m_visualRole; }

    void setVisualRole(VisualRole visual_role);

    bool hasAutoSize() const { return m_autoSized; }

    void setAutoSize(bool auto_sized);

public:

    virtual void accept(DVisitor *visitor);

    virtual void accept(DConstVisitor *visitor) const;

private:

    QString m_text;

    QPointF m_pos;

    QRectF m_rect;

    VisualRole m_visualRole;

    bool m_autoSized;

};

}

#endif // QMT_DANNOTATION_H
