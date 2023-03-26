// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    const QList<QString> templateParameters() const { return m_templateParameters; }
    void setTemplateParameters(const QList<QString> &templateParameters);
    const QList<MClassMember> members() const { return m_members; }
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
    TemplateDisplay m_templateDisplay = TemplateSmart;
    bool m_showAllMembers = false;
};

} // namespace qmt
