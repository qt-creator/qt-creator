// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "CPlusPlusForwardDeclarations.h"


namespace CPlusPlus {

class CPLUSPLUS_EXPORT FullySpecifiedType final
{
public:
    FullySpecifiedType();
    FullySpecifiedType(Type *type);
    ~FullySpecifiedType() = default;

    bool isValid() const;
    explicit operator bool() const;

    Type *type() const { return _type; }
    void setType(Type *type) { _type = type; }

    FullySpecifiedType qualifiedType() const;

    bool isConst() const { return f._isConst; }
    void setConst(bool isConst) { f._isConst = isConst; }

    bool isVolatile() const { return f._isVolatile; }
    void setVolatile(bool isVolatile) { f._isVolatile = isVolatile; }

    bool isSigned() const { return f._isSigned; }
    void setSigned(bool isSigned) { f._isSigned = isSigned; }

    bool isUnsigned() const { return f._isUnsigned; }
    void setUnsigned(bool isUnsigned) { f._isUnsigned = isUnsigned; }

    bool isFriend() const { return f._isFriend; }
    void setFriend(bool isFriend) { f._isFriend = isFriend; }

    bool isAuto() const { return f._isAuto; }
    void setAuto(bool isAuto) { f._isAuto = isAuto; }

    bool isRegister() const { return f._isRegister; }
    void setRegister(bool isRegister) { f._isRegister = isRegister; }

    bool isStatic() const { return f._isStatic; }
    void setStatic(bool isStatic) { f._isStatic = isStatic; }

    bool isExtern() const { return f._isExtern; }
    void setExtern(bool isExtern) { f._isExtern = isExtern; }

    bool isMutable() const { return f._isMutable; }
    void setMutable(bool isMutable) { f._isMutable = isMutable; }

    bool isTypedef() const { return f._isTypedef; }
    void setTypedef(bool isTypedef) { f._isTypedef = isTypedef; }

    bool isInline() const { return f._isInline; }
    void setInline(bool isInline) { f._isInline = isInline; }

    bool isVirtual() const { return f._isVirtual; }
    void setVirtual(bool isVirtual) { f._isVirtual = isVirtual; }

    bool isOverride() const { return f._isOverride; }
    void setOverride(bool isOverride) { f._isOverride = isOverride; }

    bool isFinal() const { return f._isFinal; }
    void setFinal(bool isFinal) { f._isFinal = isFinal; }

    bool isExplicit() const { return f._isExplicit; }
    void setExplicit(bool isExplicit) { f._isExplicit = isExplicit; }

    bool isDeprecated() const { return f._isDeprecated; }
    void setDeprecated(bool isDeprecated) { f._isDeprecated = isDeprecated; }

    bool isUnavailable() const { return f._isUnavailable; }
    void setUnavailable(bool isUnavailable) { f._isUnavailable = isUnavailable; }

    Type &operator*() { return *_type; }
    const Type &operator*() const { return *_type; }

    Type *operator->() { return _type; }
    const Type *operator->() const { return _type; }

    bool operator == (const FullySpecifiedType &other) const;
    bool operator != (const FullySpecifiedType &other) const { return ! operator ==(other); }
    bool operator < (const FullySpecifiedType &other) const;

    size_t hash() const;

    bool match(const FullySpecifiedType &otherTy, Matcher *matcher = nullptr) const;

    FullySpecifiedType simplified() const;

    unsigned flags() const { return _flags; }
    void setFlags(unsigned flags) { _flags = flags; }

private:
    Type *_type;
    struct Flags {
        // cv qualifiers
        unsigned _isConst: 1;
        unsigned _isVolatile: 1;

        // sign
        unsigned _isSigned: 1;
        unsigned _isUnsigned: 1;

        // storage class specifiers
        unsigned _isFriend: 1;
        unsigned _isAuto: 1;
        unsigned _isRegister: 1;
        unsigned _isStatic: 1;
        unsigned _isExtern: 1;
        unsigned _isMutable: 1;
        unsigned _isTypedef: 1;

        // function specifiers
        unsigned _isInline: 1;
        unsigned _isVirtual: 1;
        unsigned _isOverride: 1;
        unsigned _isFinal: 1;
        unsigned _isExplicit: 1;

        // speficiers from attributes
        unsigned _isDeprecated: 1;
        unsigned _isUnavailable: 1;
    };
    union {
        unsigned _flags;
        Flags f;
    };
};

} // namespace CPlusPlus
