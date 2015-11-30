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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef REMOVEPROPERTYVISITOR_H
#define REMOVEPROPERTYVISITOR_H

#include <QString>

#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class RemovePropertyVisitor: public QMLRewriter
{
public:
    RemovePropertyVisitor(QmlDesigner::TextModifier &modifier,
                          quint32 parentLocation,
                          const QString &name);

protected:
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);

private:
    void removeFrom(QmlJS::AST::UiObjectInitializer *ast);
    static bool memberNameMatchesPropertyName(const QString &propertyName,
                                              QmlJS::AST::UiObjectMember *ast);
    void removeGroupedProperty(QmlJS::AST::UiObjectDefinition *ast);
    void removeMember(QmlJS::AST::UiObjectMember *ast);

private:
    quint32 parentLocation;
    QString propertyName;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // REMOVEPROPERTYVISITOR_H
