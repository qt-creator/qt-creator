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

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

namespace {

using ::testing::PrintToString;

MATCHER_P(IsDiagnosticContainer, diagnosticContainer, "")
{
    if (arg.text() != diagnosticContainer.text()) {
        *result_listener << "text is " + PrintToString(arg.text())
                            + " and not " + PrintToString(diagnosticContainer.text());
        return false;
    }

    if (arg.location() != diagnosticContainer.location()) {
        *result_listener << "location is " + PrintToString(arg.location())
                            + " and not " + PrintToString(diagnosticContainer.location());
        return false;
    }

    if (arg.children() != diagnosticContainer.children()) {
        *result_listener << "children are " + PrintToString(arg.children())
                            + " and not " + PrintToString(diagnosticContainer.children());
        return false;
    }

    return true;
}

} // anonymous
