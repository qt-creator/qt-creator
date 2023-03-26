// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor::Internal {

class CodegenTest : public QObject
{
    Q_OBJECT
public:

private slots:
    void testPublicInEmptyClass();
    void testPublicInNonemptyClass();
    void testPublicBeforeProtected();
    void testPrivateAfterProtected();
    void testProtectedInNonemptyClass();
    void testProtectedBetweenPublicAndPrivate();
    void testQtdesignerIntegration();
    void testDefinitionEmptyClass();
    void testDefinitionFirstMember();
    void testDefinitionLastMember();
    void testDefinitionMiddleMember();
    void testDefinitionMiddleMemberSurroundedByUndefined();
    void testDefinitionMemberSpecificFile();
};

} // namespace CppEditor::Internal
