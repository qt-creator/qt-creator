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

#ifndef FUNCTIONUTILS_H
#define FUNCTIONUTILS_H

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

#endif // FUNCTIONUTILS_H
