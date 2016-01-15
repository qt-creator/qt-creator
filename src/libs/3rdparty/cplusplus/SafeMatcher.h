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

#ifndef CPLUSPLUS_SAFETYPEMATCHER_H
#define CPLUSPLUS_SAFETYPEMATCHER_H

#include "Matcher.h"

#include <vector>

namespace CPlusPlus {

class SafeMatcher : public Matcher
{
public:
    SafeMatcher();
    ~SafeMatcher();

    bool match(const PointerToMemberType *type, const PointerToMemberType *otherType);
    bool match(const PointerType *type, const PointerType *otherType);
    bool match(const ReferenceType *type, const ReferenceType *otherType);
    bool match(const ArrayType *type, const ArrayType *otherType);
    bool match(const NamedType *type, const NamedType *otherType);

    bool match(const TemplateNameId *name, const TemplateNameId *otherName);
    bool match(const DestructorNameId *name, const DestructorNameId *otherName);
    bool match(const ConversionNameId *name, const ConversionNameId *otherName);
    bool match(const QualifiedNameId *name, const QualifiedNameId *otherName);
    bool match(const SelectorNameId *name, const SelectorNameId *otherName);

private:
    std::vector<const Type *> _blockedTypes;
    std::vector<const Name *> _blockedNames;
};

} // CPlusPlus namespace

#endif // CPLUSPLUS_SAFETYPEMATCHER_H
