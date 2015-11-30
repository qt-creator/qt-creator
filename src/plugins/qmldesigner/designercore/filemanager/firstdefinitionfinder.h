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

#ifndef FIRSTDEFINITIONFINDER_H
#define FIRSTDEFINITIONFINDER_H

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>

namespace QmlDesigner {

class FirstDefinitionFinder: protected QmlJS::AST::Visitor
{
public:
    FirstDefinitionFinder(const QString &text);

    qint32 operator()(quint32 offset);

protected:
    using QmlJS::AST::Visitor::visit;

    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);

    void extractFirstObjectDefinition(QmlJS::AST::UiObjectInitializer* ast);

private:
    QmlJS::Document::MutablePtr m_doc;
    quint32 m_offset;
    QmlJS::AST::UiObjectDefinition *m_firstObjectDefinition;

};

} // namespace QmlDesigner

#endif // FIRSTDEFINITIONFINDER_H
