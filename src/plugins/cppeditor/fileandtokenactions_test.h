// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
