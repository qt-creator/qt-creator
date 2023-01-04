// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "glsl.h"

namespace GLSL {

class GLSL_EXPORT Type
{
public:
    virtual ~Type();

    virtual QString toString() const = 0;

    virtual const UndefinedType *asUndefinedType() const { return nullptr; }
    virtual const VoidType *asVoidType() const { return nullptr; }
    virtual const BoolType *asBoolType() const { return nullptr; }
    virtual const IntType *asIntType() const { return nullptr; }
    virtual const UIntType *asUIntType() const { return nullptr; }
    virtual const FloatType *asFloatType() const { return nullptr; }
    virtual const DoubleType *asDoubleType() const { return nullptr; }
    virtual const ScalarType *asScalarType() const { return nullptr; }
    virtual const IndexType *asIndexType() const { return nullptr; }
    virtual const VectorType *asVectorType() const { return nullptr; }
    virtual const MatrixType *asMatrixType() const { return nullptr; }
    virtual const ArrayType *asArrayType() const { return nullptr; }
    virtual const SamplerType *asSamplerType() const { return nullptr; }
    virtual const OverloadSet *asOverloadSetType() const { return nullptr; }

    virtual const Struct *asStructType() const { return nullptr; }
    virtual const Function *asFunctionType() const { return nullptr; }

    virtual bool isEqualTo(const Type *other) const = 0;
    virtual bool isLessThan(const Type *other) const = 0;
};

} // namespace GLSL
