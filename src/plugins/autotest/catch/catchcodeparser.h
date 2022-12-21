// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "catchtreeitem.h"

#include <cplusplus/SimpleLexer.h>

#include <QByteArray>

namespace Autotest {
namespace Internal {

class CatchCodeParser
{
public:
    CatchCodeParser(const QByteArray &source, const CPlusPlus::LanguageFeatures &features);
    virtual ~CatchCodeParser() = default;
    CatchTestCodeLocationList findTests();
private:
    void handleIdentifier();
    void handleTestCase(bool isScenario);
    void handleParameterizedTestCase(bool isFixture);
    void handleFixtureOrRegisteredTestCase(bool isFixture);

    QString getStringLiteral(CPlusPlus::Kind &stoppedAtKind);
    bool skipCommentsUntil(CPlusPlus::Kind nextExpectedKind);   // moves currentIndex if succeeds
    CPlusPlus::Kind skipUntilCorrespondingRParen();             // moves currentIndex
    bool skipFixtureParameter();
    bool skipFunctionParameter();

    const QByteArray &m_source;
    const CPlusPlus::LanguageFeatures &m_features;
    CPlusPlus::Tokens m_tokens;
    int m_currentIndex = 0;
    CatchTestCodeLocationList m_testCases;
};

} // namespace Internal
} // namespace Autotest
