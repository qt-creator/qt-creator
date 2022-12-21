// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Debugger::Internal {

// Simplify complicated STL template types, such as
// 'std::basic_string<char,std::char_traits<char>,std::allocator<char> > '->
// 'std::string'.
QString simplifyType(const QString &typeIn);

} // Debugger::Internal
