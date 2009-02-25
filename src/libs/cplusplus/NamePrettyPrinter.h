/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CPLUSPLUS_NAMEPRETTYPRINTER_H
#define CPLUSPLUS_NAMEPRETTYPRINTER_H

#include <NameVisitor.h>
#include <QString>

namespace CPlusPlus {

class Overview;

class CPLUSPLUS_EXPORT NamePrettyPrinter: protected NameVisitor
{
public:
    NamePrettyPrinter(const Overview *overview);
    virtual ~NamePrettyPrinter();

    const Overview *overview() const;
    QString operator()(Name *name);

protected:
    QString switchName(const QString &name = QString());

    virtual void visit(NameId *name);
    virtual void visit(TemplateNameId *name);
    virtual void visit(DestructorNameId *name);
    virtual void visit(OperatorNameId *name);
    virtual void visit(ConversionNameId *name);
    virtual void visit(QualifiedNameId *name);

private:
    const Overview *_overview;
    QString _name;
};

} // end of namespace CPlusPlus

#endif // CPLUSPLUS_NAMEPRETTYPRINTER_H
