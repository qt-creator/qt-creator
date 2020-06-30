/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

/*
    See std(::experimental)::variant.
*/

// std::variant from Apple's Clang supports methods that throw std::bad_optional_access only
// with deployment target >= macOS 10.14
// TODO: Use std::variant everywhere when we can require macOS 10.14
#if !defined(__apple_build_version__)
#include <variant>

namespace Utils {
using std::get;
using std::get_if;
using std::holds_alternative;
using std::variant;
using std::visit;
} // namespace Utils

#else
#include <3rdparty/variant/variant.hpp>

namespace Utils {
using mpark::get;
using mpark::get_if;
using mpark::holds_alternative;
using mpark::variant;
using mpark::visit;
} // namespace Utils

#endif
