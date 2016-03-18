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

#include "callgrindfunction.h"

namespace Valgrind {
namespace Callgrind {

/**
 * self cost of a function cycle: sum of self costs of functions in the cycle
 * callers of a function cycle: set of callers to functions in the cycle
 *                              excluding calls inside the cycle
 * callees of a function cycle: set of callees from functions in the cycle
 *                              excluding calees inside the cycle
 * inclusive cost of a function cycle: sum of inclusive cost of callees of the cycle (see above)
 */
class FunctionCycle : public Function
{
public:
    explicit FunctionCycle(const ParseData *data);
    virtual ~FunctionCycle();

    /// sets the list of functions that make up this cycle
    /// NOTE: ownership is *not* transferred to the cycle
    void setFunctions(const QVector<const Function *> &functions);
    /// @return the functions that make up this cycle
    QVector<const Function *> functions() const;

private:
    class Private;
};

} // namespace Callgrind
} // namespace Valgrind
