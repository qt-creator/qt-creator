// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gtesttreeitem.h"

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>

#include <QMap>

namespace Autotest {
namespace Internal {

inline bool operator<(const GTestCaseSpec &spec1, const GTestCaseSpec &spec2)
{
    if (spec1.testCaseName != spec2.testCaseName)
        return spec1.testCaseName < spec2.testCaseName;
    if (spec1.parameterized == spec2.parameterized) {
        if (spec1.typed == spec2.typed) {
            if (spec1.disabled == spec2.disabled)
                return false;
            else
                return !spec1.disabled;
        } else {
            return !spec1.typed;
        }
    } else {
        return !spec1.parameterized;
    }
}

class GTestVisitor : public CPlusPlus::ASTVisitor
{
public:
    explicit GTestVisitor(CPlusPlus::Document::Ptr doc);
    bool visit(CPlusPlus::FunctionDefinitionAST *ast) override;

    QMap<GTestCaseSpec, GTestCodeLocationList> gtestFunctions() const { return m_gtestFunctions; }

private:
    QString enclosingNamespaces(CPlusPlus::Symbol *symbol) const;

    CPlusPlus::Document::Ptr m_document;
    CPlusPlus::Overview m_overview;
    QMap<GTestCaseSpec, GTestCodeLocationList> m_gtestFunctions;
};

} // namespace Internal
} // namespace Autotest
