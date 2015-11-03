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

#ifndef QMT_DOBJECT_H
#define QMT_DOBJECT_H

#include "delement.h"

#include "qmt/infrastructure/uid.h"

#include <QList>
#include <QPointF>
#include <QRectF>


namespace qmt {

class MObject;


class QMT_EXPORT DObject :
        public DElement
{
public:

    enum VisualPrimaryRole {
        PRIMARY_ROLE_NORMAL,
        DEPRECATED_PRIMARY_ROLE_LIGHTER,
        DEPRECATED_PRIMARY_ROLE_DARKER,
        DEPRECATED_PRIMARY_ROLE_SOFTEN,
        DEPRECATED_PRIMARY_ROLE_OUTLINE,
        PRIMARY_ROLE_CUSTOM1,
        PRIMARY_ROLE_CUSTOM2,
        PRIMARY_ROLE_CUSTOM3,
        PRIMARY_ROLE_CUSTOM4,
        PRIMARY_ROLE_CUSTOM5
    };

    enum VisualSecondaryRole {
        SECONDARY_ROLE_NONE,
        SECONDARY_ROLE_LIGHTER,
        SECONDARY_ROLE_DARKER,
        SECONDARY_ROLE_SOFTEN,
        SECONDARY_ROLE_OUTLINE
    };

    enum StereotypeDisplay {
        STEREOTYPE_NONE,
        STEREOTYPE_LABEL,
        STEREOTYPE_DECORATION,
        STEREOTYPE_ICON,
        STEREOTYPE_SMART
    };

public:
    DObject();

    DObject(const DObject &);

    ~DObject();

public:

    DObject &operator=(const DObject &rhs);

public:

    Uid getModelUid() const { return m_modelUid; }

    void setModelUid(const Uid &uid);

    QList<QString> getStereotypes() const { return m_stereotypes; }

    void setStereotypes(const QList<QString> &stereotypes);

    QString getContext() const { return m_context; }

    void setContext(const QString &context);

    QString getName() const { return m_name; }

    void setName(const QString &name);

    QPointF getPos() const { return m_pos; }

    void setPos(const QPointF &pos);

    QRectF getRect() const { return m_rect; }

    void setRect(const QRectF &rect);

    int getDepth() const { return m_depth; }

    void setDepth(int depth);

    VisualPrimaryRole getVisualPrimaryRole() const { return m_visualPrimaryRole; }

    void setVisualPrimaryRole(VisualPrimaryRole visual_primary_role);

    VisualSecondaryRole getVisualSecondaryRole() const { return m_visualSecondaryRole; }

    void setVisualSecondaryRole(VisualSecondaryRole visual_secondary_role);

    StereotypeDisplay getStereotypeDisplay() const { return m_stereotypeDisplay; }

    void setStereotypeDisplay(StereotypeDisplay stereotype_display);

    bool hasAutoSize() const { return m_autoSized; }

    void setAutoSize(bool auto_sized);

    bool isVisualEmphasized() const { return m_visualEmphasized; }

    void setVisualEmphasized(bool visual_emphasized);

public:

    virtual void accept(DVisitor *visitor);

    virtual void accept(DConstVisitor *visitor) const;

private:

    Uid m_modelUid;

    QList<QString> m_stereotypes;

    QString m_context;

    QString m_name;

    QPointF m_pos;

    QRectF m_rect;

    int m_depth;

    VisualPrimaryRole m_visualPrimaryRole;

    VisualSecondaryRole m_visualSecondaryRole;

    StereotypeDisplay m_stereotypeDisplay;

    bool m_autoSized;

    bool m_visualEmphasized;
};

}

#endif // QMT_DOBJECT_H
