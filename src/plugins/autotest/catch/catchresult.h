/****************************************************************************
**
** Copyright (C) 2019 Jochen Seemann
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

#include "../testresult.h"

namespace Autotest {
namespace Internal {

class CatchResult : public TestResult
{
public:
    CatchResult(const QString &id, const QString &name);

    bool isDirectParentOf(const TestResult *other, bool *needsIntermediate) const override;

    void setSectionDepth(int depth) { m_sectionDepth = depth; }
    int sectionDepth() const { return m_sectionDepth; }

private:
    int m_sectionDepth = 0;
};

} // namespace Internal
} // namespace Autotest
