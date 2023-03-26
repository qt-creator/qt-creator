// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor::Internal::Tests {

class FollowSymbolTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testSwitchMethodDeclDef_data();
    void testSwitchMethodDeclDef();

    void testFollowSymbolMultipleDocuments_data();
    void testFollowSymbolMultipleDocuments();

    void testFollowSymbol_data();
    void testFollowSymbol();

    void testFollowSymbolQTCREATORBUG7903_data();
    void testFollowSymbolQTCREATORBUG7903();

    void testFollowCall_data();
    void testFollowCall();

    void testFollowSymbolQObjectConnect_data();
    void testFollowSymbolQObjectConnect();
    void testFollowSymbolQObjectOldStyleConnect();

    void testFollowClassOperatorOnOperatorToken_data();
    void testFollowClassOperatorOnOperatorToken();

    void testFollowClassOperator_data();
    void testFollowClassOperator();

    void testFollowClassOperatorInOp_data();
    void testFollowClassOperatorInOp();

    void testFollowVirtualFunctionCall_data();
    void testFollowVirtualFunctionCall();
    void testFollowVirtualFunctionCallMultipleDocuments();
};

} // namespace CppEditor::Internal::Tests
