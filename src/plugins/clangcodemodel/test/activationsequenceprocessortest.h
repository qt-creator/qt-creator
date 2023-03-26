// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ClangCodeModel::Internal::Tests {

class ActivationSequenceProcessorTest : public QObject
{
    Q_OBJECT

private slots:
    void testCouldNotProcesseRandomCharacters();
    void testCouldNotProcesseEmptyString();
    void testDot();
    void testComma();
    void testLeftParenAsFunctionCall();
    void testLeftParenNotAsFunctionCall();
    void testColonColon();
    void testArrow();
    void testDotStar();
    void testArrowStar();
    void testDoxyGenCommentBackSlash();
    void testDoxyGenCommentAt();
    void testAngleStringLiteral();
    void testStringLiteral();
    void testSlash();
    void testPound();
    void testPositionIsOne();
    void testPositionIsTwo();
    void testPositionIsTwoWithASingleSign();
    void testPositionIsThree();
};

} // namespace ClangCodeModel::Internal::Tests
