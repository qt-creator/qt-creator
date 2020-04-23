/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
