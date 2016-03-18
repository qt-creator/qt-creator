/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <cplusplus/NameVisitor.h>

#include <QString>

namespace CPlusPlus {

class Overview;

class CPLUSPLUS_EXPORT NamePrettyPrinter: protected NameVisitor
{
public:
    NamePrettyPrinter(const Overview *overview);
    virtual ~NamePrettyPrinter();

    const Overview *overview() const;
    QString operator()(const Name *name);

protected:
    QString switchName(const QString &name = QString());

    virtual void visit(const Identifier *name);
    virtual void visit(const TemplateNameId *name);
    virtual void visit(const DestructorNameId *name);
    virtual void visit(const OperatorNameId *name);
    virtual void visit(const ConversionNameId *name);
    virtual void visit(const QualifiedNameId *name);
    virtual void visit(const SelectorNameId *name);
    virtual void visit(const AnonymousNameId *name);

private:
    const Overview *_overview;
    QString _name;
};

} // namespace CPlusPlus
