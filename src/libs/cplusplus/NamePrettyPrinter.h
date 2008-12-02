/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

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
