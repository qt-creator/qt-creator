/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <vector>

namespace CppTools {
namespace Constants {

class ClazyCheckInfo
{
public:
    bool isValid() const { return !name.isEmpty() && level >= -1; }

    QString name;
    int level; // "Manual level"
    QStringList topics;
};
using ClazyCheckInfos = std::vector<ClazyCheckInfo>;

// CLANG-UPGRADE-CHECK: Run 'scripts/generateClazyChecks.py' after Clang upgrade to
// update this header.
static const ClazyCheckInfos CLAZY_CHECKS = {
    {QString("qt-keywords"), -1, {}},
    {QString("ifndef-define-typo"), -1, {"bug"}},
    {QString("inefficient-qlist"), -1, {"containers","performance"}},
    {QString("isempty-vs-count"), -1, {"readability"}},
    {QString("qrequiredresult-candidates"), -1, {"bug"}},
    {QString("qstring-varargs"), -1, {"bug"}},
    {QString("qt4-qstring-from-array"), -1, {"qt4","qstring"}},
    {QString("tr-non-literal"), -1, {"bug"}},
    {QString("raw-environment-function"), -1, {"bug"}},
    {QString("container-inside-loop"), -1, {"containers","performance"}},
    {QString("qhash-with-char-pointer-key"), -1, {"cpp","bug"}},
    {QString("connect-by-name"), 0, {"bug","readability"}},
    {QString("connect-non-signal"), 0, {"bug"}},
    {QString("wrong-qevent-cast"), 0, {"bug"}},
    {QString("lambda-in-connect"), 0, {"bug"}},
    {QString("lambda-unique-connection"), 0, {"bug"}},
    {QString("qdatetime-utc"), 0, {"performance"}},
    {QString("qgetenv"), 0, {"performance"}},
    {QString("qstring-insensitive-allocation"), 0, {"performance","qstring"}},
    {QString("fully-qualified-moc-types"), 0, {"bug","qml"}},
    {QString("qvariant-template-instantiation"), 0, {"performance"}},
    {QString("unused-non-trivial-variable"), 0, {"readability"}},
    {QString("connect-not-normalized"), 0, {"performance"}},
    {QString("mutable-container-key"), 0, {"containers","bug"}},
    {QString("qenums"), 0, {"deprecation"}},
    {QString("qmap-with-pointer-key"), 0, {"containers","performance"}},
    {QString("qstring-ref"), 0, {"performance","qstring"}},
    {QString("strict-iterators"), 0, {"containers","performance","bug"}},
    {QString("writing-to-temporary"), 0, {"bug"}},
    {QString("container-anti-pattern"), 0, {"containers","performance"}},
    {QString("qcolor-from-literal"), 0, {"performance"}},
    {QString("qfileinfo-exists"), 0, {"performance"}},
    {QString("qstring-arg"), 0, {"performance","qstring"}},
    {QString("empty-qstringliteral"), 0, {"performance"}},
    {QString("qt-macros"), 0, {"bug"}},
    {QString("temporary-iterator"), 0, {"containers","bug"}},
    {QString("wrong-qglobalstatic"), 0, {"performance"}},
    {QString("lowercase-qml-type-name"), 0, {"qml","bug"}},
    {QString("auto-unexpected-qstringbuilder"), 1, {"bug","qstring"}},
    {QString("connect-3arg-lambda"), 1, {"bug"}},
    {QString("const-signal-or-slot"), 1, {"readability","bug"}},
    {QString("detaching-temporary"), 1, {"containers","performance"}},
    {QString("foreach"), 1, {"containers","performance"}},
    {QString("incorrect-emit"), 1, {"readability"}},
    {QString("inefficient-qlist-soft"), 1, {"containers","performance"}},
    {QString("install-event-filter"), 1, {"bug"}},
    {QString("non-pod-global-static"), 1, {"performance"}},
    {QString("post-event"), 1, {"bug"}},
    {QString("qdeleteall"), 1, {"containers","performance"}},
    {QString("qlatin1string-non-ascii"), 1, {"bug","qstring"}},
    {QString("qproperty-without-notify"), 1, {"bug"}},
    {QString("qstring-left"), 1, {"bug","performance","qstring"}},
    {QString("range-loop"), 1, {"containers","performance"}},
    {QString("returning-data-from-temporary"), 1, {"bug"}},
    {QString("rule-of-two-soft"), 1, {"cpp","bug"}},
    {QString("child-event-qobject-cast"), 1, {"bug"}},
    {QString("virtual-signal"), 1, {"bug","readability"}},
    {QString("overridden-signal"), 1, {"bug","readability"}},
    {QString("qhash-namespace"), 1, {"bug"}},
    {QString("skipped-base-method"), 1, {"bug","cpp"}},
    {QString("unneeded-cast"), 3, {"cpp","readability"}},
    {QString("ctor-missing-parent-argument"), 2, {"bug"}},
    {QString("base-class-event"), 2, {"bug"}},
    {QString("copyable-polymorphic"), 2, {"cpp","bug"}},
    {QString("function-args-by-ref"), 2, {"cpp","performance"}},
    {QString("function-args-by-value"), 2, {"cpp","performance"}},
    {QString("global-const-char-pointer"), 2, {"cpp","performance"}},
    {QString("implicit-casts"), 2, {"cpp","bug"}},
    {QString("missing-qobject-macro"), 2, {"bug"}},
    {QString("missing-typeinfo"), 2, {"containers","performance"}},
    {QString("old-style-connect"), 2, {"performance"}},
    {QString("qstring-allocations"), 2, {"performance","qstring"}},
    {QString("returning-void-expression"), 2, {"readability","cpp"}},
    {QString("rule-of-three"), 2, {"cpp","bug"}},
    {QString("virtual-call-ctor"), 2, {"cpp","bug"}},
    {QString("static-pmf"), 2, {"bug"}},
    {QString("assert-with-side-effects"), 3, {"bug"}},
    {QString("detaching-member"), 3, {"containers","performance"}},
    {QString("thread-with-slots"), 3, {"bug"}},
    {QString("reserve-candidates"), 3, {"containers"}}
};

} // namespace Constants
} // namespace CppTools
