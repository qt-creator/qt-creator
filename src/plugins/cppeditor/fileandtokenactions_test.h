/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QObject>

namespace CppEditor::Internal::Tests {

// These tests operate on a project and require special invocation:
//
// Ensure that the project is properly configured for a given settings path:
//   $ ./qtcreator -settingspath /your/settings/path /path/to/project
//
// ...and that it builds, which might prevent blocking dialogs for not
// existing files (e.g. ui_*.h).
//
// Run the tests:
//   $ export QTC_TEST_WAIT_FOR_LOADED_PROJECT=1
//   $ ./qtcreator -settingspath /your/settings/path -test CppEditor,FileAndTokenActionsTest /path/to/project
class FileAndTokenActionsTest : public QObject
{
    Q_OBJECT

private slots:
    void testOpenEachFile();
    void testSwitchHeaderSourceOnEachFile();
    void testMoveTokenWiseThroughEveryFile();
    void testMoveTokenWiseThroughEveryFileAndFollowSymbol();
    void testMoveTokenWiseThroughEveryFileAndSwitchDeclarationDefinition();
    void testMoveTokenWiseThroughEveryFileAndFindUsages();
    void testMoveTokenWiseThroughEveryFileAndRenameUsages();
    void testMoveTokenWiseThroughEveryFileAndOpenTypeHierarchy();
    void testMoveTokenWiseThroughEveryFileAndInvokeCompletion();
    void testMoveTokenWiseThroughEveryFileAndTriggerQuickFixes();
};

} // namespace CppEditor::Internal::Tests
