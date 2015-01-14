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
