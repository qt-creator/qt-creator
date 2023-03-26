// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "boosttesttreeitem.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/TypeOfExpression.h>

#include <QByteArray>

namespace Autotest {
namespace Internal {

class BoostCodeParser
{
public:
    BoostCodeParser(const QByteArray &source, const CPlusPlus::LanguageFeatures &features,
                    const CPlusPlus::Document::Ptr &doc, const CPlusPlus::Snapshot &snapshot);
    virtual ~BoostCodeParser() = default;
    BoostTestCodeLocationList findTests();
private:
    enum class TestCaseType {Auto, Functions, Parameter, Fixture, Data};

    void handleIdentifier();
    void handleSuiteBegin(bool isFixture);
    void handleSuiteEnd();
    void handleTestCase(TestCaseType testCaseType);
    void handleDecorator();
    void handleDecorators();

    bool skipCommentsUntil(CPlusPlus::Kind nextExpectedKind); // moves currentIndex if succeeds
    QByteArray contentUntil(CPlusPlus::Kind stopKind);        // does not move currentIndex

    bool isBoostBindCall(const QByteArray &function);
    bool aliasedOrRealNamespace(const QByteArray &symbolName, const QString &origNamespace,
                                QByteArray *simplifiedName, bool *aliasedOrReal);
    bool evalCurrentDecorator(const QByteArray &decorator, QString *symbolName,
                              QByteArray *simplifiedName, bool *aliasedOrReal);

    const QByteArray &m_source;
    const CPlusPlus::LanguageFeatures &m_features;
    const CPlusPlus::Document::Ptr &m_doc;
    const CPlusPlus::Snapshot &m_snapshot;
    CPlusPlus::LookupContext m_lookupContext;
    CPlusPlus::TypeOfExpression m_typeOfExpression;
    CPlusPlus::Tokens m_tokens;
    int m_currentIndex = 0;
    BoostTestCodeLocationList m_testCases;
    QVector<BoostTestInfo> m_suites;
    QString m_currentSuite;
    BoostTestTreeItem::TestStates m_currentState = BoostTestTreeItem::Enabled;
    int m_lineNo = 0;
};

} // Internal
} // Autotest
