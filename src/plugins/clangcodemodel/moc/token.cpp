// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "token.h"

QT_BEGIN_NAMESPACE

#if defined(DEBUG_MOC)
const char *tokenTypeName(Token t)
{
    switch (t) {
#define CREATE_CASE(Name) case Name: return #Name;
    FOR_ALL_TOKENS(CREATE_CASE)
#undef CREATE_CASE
    }
    return "";
}
#endif

QT_END_NAMESPACE
