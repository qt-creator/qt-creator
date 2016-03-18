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

#include <cplusplus/TypeVisitor.h>
#include <cplusplus/FullySpecifiedType.h>

#include <QString>
#include <QList>

namespace CPlusPlus {

class Overview;
class FullySpecifiedType;

class CPLUSPLUS_EXPORT TypePrettyPrinter: protected TypeVisitor
{
public:
    TypePrettyPrinter(const Overview *overview);
    virtual ~TypePrettyPrinter();

    const Overview *overview() const;

    QString operator()(const FullySpecifiedType &type);
    QString operator()(const FullySpecifiedType &type, const QString &name);

private:
    void acceptType(const FullySpecifiedType &ty);

    virtual void visit(UndefinedType *type);
    virtual void visit(VoidType *type);
    virtual void visit(IntegerType *type);
    virtual void visit(FloatType *type);
    virtual void visit(PointerToMemberType *type);
    virtual void visit(PointerType *type);
    virtual void visit(ReferenceType *type);
    virtual void visit(ArrayType *type);
    virtual void visit(NamedType *type);
    virtual void visit(Function *type);
    virtual void visit(Namespace *type);
    virtual void visit(Template *type);
    virtual void visit(Class *type);
    virtual void visit(Enum *type);

    QString switchName(const QString &name);
    QString switchText(const QString &text = QString());
    bool switchNeedsParens(bool needsParens);
    bool switchIsIndirectionType(bool isIndirectionType);
    bool switchIsIndirectionToArrayOrFunction(bool isIndirectionToArrayOrFunction);

    void appendSpace();
    void prependSpaceUnlessBracket();
    void prependWordSeparatorSpace();
    void prependCv(const FullySpecifiedType &ty);
    void prependSpaceAfterIndirection(bool hasName);
    void prependSpaceBeforeIndirection(const FullySpecifiedType &type);

    enum IndirectionType { aPointerType, aReferenceType, aRvalueReferenceType };
    void visitIndirectionType(const IndirectionType indirectionType,
        const FullySpecifiedType &elementType, bool isIndirectionToArrayOrFunction);

    const Overview *_overview;
    QString _name;
    QString _text;
    FullySpecifiedType _fullySpecifiedType;
    bool _needsParens;
    bool _isIndirectionType;
    bool _isIndirectionToArrayOrFunction;
};

} // namespace CPlusPlus
