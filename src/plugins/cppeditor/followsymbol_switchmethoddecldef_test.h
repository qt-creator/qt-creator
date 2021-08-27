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
