/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "dobject.h"
#include "qmt/model/mclassmember.h"

#include <QSet>

namespace qmt {

class QMT_EXPORT DClass : public DObject
{
public:
    enum TemplateDisplay {
        TemplateSmart,
        TemplateBox,
        TemplateName
    };

    DClass();
    ~DClass() override;

    QString umlNamespace() const { return m_umlNamespace; }
    void setUmlNamespace(const QString &umlNamespace);
    QList<QString> templateParameters() const { return m_templateParameters; }
    void setTemplateParameters(const QList<QString> &templateParameters);
    QList<MClassMember> members() const { return m_members; }
    void setMembers(const QList<MClassMember> &members);
    QSet<Uid> visibleMembers() const { return m_visibleMembers; }
    void setVisibleMembers(const QSet<Uid> &visibleMembers);
    TemplateDisplay templateDisplay() const { return m_templateDisplay; }
    void setTemplateDisplay(TemplateDisplay templateDisplay);
    bool showAllMembers() const { return m_showAllMembers; }
    void setShowAllMembers(bool showAllMembers);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;

private:
    QString m_umlNamespace;
    QList<QString> m_templateParameters;
    QList<MClassMember> m_members;
    QSet<Uid> m_visibleMembers;
    TemplateDisplay m_templateDisplay;
    bool m_showAllMembers;
};

} // namespace qmt
