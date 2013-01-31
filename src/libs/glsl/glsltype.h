/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GLSLTYPE_H
#define GLSLTYPE_H

#include "glsl.h"

namespace GLSL {

class GLSL_EXPORT Type
{
public:
    virtual ~Type();

    virtual QString toString() const = 0;

    virtual const UndefinedType *asUndefinedType() const { return 0; }
    virtual const VoidType *asVoidType() const { return 0; }
    virtual const BoolType *asBoolType() const { return 0; }
    virtual const IntType *asIntType() const { return 0; }
    virtual const UIntType *asUIntType() const { return 0; }
    virtual const FloatType *asFloatType() const { return 0; }
    virtual const DoubleType *asDoubleType() const { return 0; }
    virtual const ScalarType *asScalarType() const { return 0; }
    virtual const IndexType *asIndexType() const { return 0; }
    virtual const VectorType *asVectorType() const { return 0; }
    virtual const MatrixType *asMatrixType() const { return 0; }
    virtual const ArrayType *asArrayType() const { return 0; }
    virtual const SamplerType *asSamplerType() const { return 0; }
    virtual const OverloadSet *asOverloadSetType() const { return 0; }

    virtual const Struct *asStructType() const { return 0; }
    virtual const Function *asFunctionType() const { return 0; }

    virtual bool isEqualTo(const Type *other) const = 0;
    virtual bool isLessThan(const Type *other) const = 0;
};

} // namespace GLSL

#endif // GLSLTYPE_H
