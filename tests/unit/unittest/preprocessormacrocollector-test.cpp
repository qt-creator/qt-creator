/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "googletest.h"

#include <preprocessormacrocollector.h>

namespace {

using ProjectExplorer::Macros;
using ProjectExplorer::MacroType;
using PM = ClangPchManager::PreprocessorMacro;

class PreprocessorMacrosManager : public testing::Test
{
protected:
    Macros macros1{{"yi", "1"}, {"er", "2"}};
    Macros macros2{{"san", "3"}, {"se", "4"}};
    Macros macrosWithDuplicates{{"yi", "1"}, {"san", "3"}, {"se", "4"}, {"er", "0"}};
    ClangPchManager::PreprocessorMacroCollector manager;
};

TEST_F(PreprocessorMacrosManager, Add)
{
    manager.add(macros1);

    ASSERT_THAT(manager.macros(), ElementsAre(PM{"er", "2"}, PM{"yi", "1"}));
}

TEST_F(PreprocessorMacrosManager, AddMore)
{
    manager.add(macros1);

    manager.add(macros2);

    ASSERT_THAT(manager.macros(),
                ElementsAre(PM{"er", "2"}, PM{"san", "3"}, PM{"se", "4"}, PM{"yi", "1"}));
}

TEST_F(PreprocessorMacrosManager, FilterDuplicates)
{
    manager.add(macros1);

    manager.add(macrosWithDuplicates);

    ASSERT_THAT(manager.macros(),
                ElementsAre(PM{"er", "0"}, PM{"er", "2"}, PM{"san", "3"}, PM{"se", "4"}, PM{"yi", "1"}));
}
} // namespace
