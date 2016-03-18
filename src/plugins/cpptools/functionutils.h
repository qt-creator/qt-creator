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

#include "cpptools_global.h"

QT_BEGIN_NAMESPACE
template <class> class QList;
QT_END_NAMESPACE

namespace CPlusPlus {
class Class;
class Function;
class LookupContext;
class Snapshot;
class Symbol;
} // namespace CPlusPlus

namespace CppTools {

class CPPTOOLS_EXPORT FunctionUtils
{
public:
    static bool isVirtualFunction(const CPlusPlus::Function *function,
                                  const CPlusPlus::LookupContext &context,
                                  const CPlusPlus::Function **firstVirtual = 0);

    static bool isPureVirtualFunction(const CPlusPlus::Function *function,
                                      const CPlusPlus::LookupContext &context,
                                      const CPlusPlus::Function **firstVirtual = 0);

    static QList<CPlusPlus::Function *> overrides(CPlusPlus::Function *function,
                                                  CPlusPlus::Class *functionsClass,
                                                  CPlusPlus::Class *staticClass,
                                                  const CPlusPlus::Snapshot &snapshot);
};

} // namespace CppTools
