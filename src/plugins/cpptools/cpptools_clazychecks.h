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
    int level = -1; // "Manual level"
    QStringList topics;
};
using ClazyCheckInfos = std::vector<ClazyCheckInfo>;

// CLANG-UPGRADE-CHECK: Run 'scripts/generateClazyChecks.py' after Clang upgrade to
// update this header.
static const ClazyCheckInfos CLAZY_CHECKS = {
    {"qt-keywords", -1, {}},
    {"ifndef-define-typo", -1, {"bug"}},
    {"inefficient-qlist", -1, {"containers","performance"}},
    {"isempty-vs-count", -1, {"readability"}},
    {"qrequiredresult-candidates", -1, {"bug"}},
    {"qstring-varargs", -1, {"bug"}},
    {"qt4-qstring-from-array", -1, {"qt4","qstring"}},
    {"tr-non-literal", -1, {"bug"}},
    {"raw-environment-function", -1, {"bug"}},
    {"container-inside-loop", -1, {"containers","performance"}},
    {"qhash-with-char-pointer-key", -1, {"cpp","bug"}},
    {"connect-by-name", 0, {"bug","readability"}},
    {"connect-non-signal", 0, {"bug"}},
    {"wrong-qevent-cast", 0, {"bug"}},
    {"lambda-in-connect", 0, {"bug"}},
    {"lambda-unique-connection", 0, {"bug"}},
    {"qdatetime-utc", 0, {"performance"}},
    {"qgetenv", 0, {"performance"}},
    {"qstring-insensitive-allocation", 0, {"performance","qstring"}},
    {"fully-qualified-moc-types", 0, {"bug","qml"}},
    {"qvariant-template-instantiation", 0, {"performance"}},
    {"unused-non-trivial-variable", 0, {"readability"}},
    {"connect-not-normalized", 0, {"performance"}},
    {"mutable-container-key", 0, {"containers","bug"}},
    {"qenums", 0, {"deprecation"}},
    {"qmap-with-pointer-key", 0, {"containers","performance"}},
    {"qstring-ref", 0, {"performance","qstring"}},
    {"strict-iterators", 0, {"containers","performance","bug"}},
    {"writing-to-temporary", 0, {"bug"}},
    {"container-anti-pattern", 0, {"containers","performance"}},
    {"qcolor-from-literal", 0, {"performance"}},
    {"qfileinfo-exists", 0, {"performance"}},
    {"qstring-arg", 0, {"performance","qstring"}},
    {"empty-qstringliteral", 0, {"performance"}},
    {"qt-macros", 0, {"bug"}},
    {"temporary-iterator", 0, {"containers","bug"}},
    {"wrong-qglobalstatic", 0, {"performance"}},
    {"lowercase-qml-type-name", 0, {"qml","bug"}},
    {"auto-unexpected-qstringbuilder", 1, {"bug","qstring"}},
    {"connect-3arg-lambda", 1, {"bug"}},
    {"const-signal-or-slot", 1, {"readability","bug"}},
    {"detaching-temporary", 1, {"containers","performance"}},
    {"foreach", 1, {"containers","performance"}},
    {"incorrect-emit", 1, {"readability"}},
    {"inefficient-qlist-soft", 1, {"containers","performance"}},
    {"install-event-filter", 1, {"bug"}},
    {"non-pod-global-static", 1, {"performance"}},
    {"post-event", 1, {"bug"}},
    {"qdeleteall", 1, {"containers","performance"}},
    {"qlatin1string-non-ascii", 1, {"bug","qstring"}},
    {"qproperty-without-notify", 1, {"bug"}},
    {"qstring-left", 1, {"bug","performance","qstring"}},
    {"range-loop", 1, {"containers","performance"}},
    {"returning-data-from-temporary", 1, {"bug"}},
    {"rule-of-two-soft", 1, {"cpp","bug"}},
    {"child-event-qobject-cast", 1, {"bug"}},
    {"virtual-signal", 1, {"bug","readability"}},
    {"overridden-signal", 1, {"bug","readability"}},
    {"qhash-namespace", 1, {"bug"}},
    {"skipped-base-method", 1, {"bug","cpp"}},
    {"unneeded-cast", 3, {"cpp","readability"}},
    {"ctor-missing-parent-argument", 2, {"bug"}},
    {"base-class-event", 2, {"bug"}},
    {"copyable-polymorphic", 2, {"cpp","bug"}},
    {"function-args-by-ref", 2, {"cpp","performance"}},
    {"function-args-by-value", 2, {"cpp","performance"}},
    {"global-const-char-pointer", 2, {"cpp","performance"}},
    {"implicit-casts", 2, {"cpp","bug"}},
    {"missing-qobject-macro", 2, {"bug"}},
    {"missing-typeinfo", 2, {"containers","performance"}},
    {"old-style-connect", 2, {"performance"}},
    {"qstring-allocations", 2, {"performance","qstring"}},
    {"returning-void-expression", 2, {"readability","cpp"}},
    {"rule-of-three", 2, {"cpp","bug"}},
    {"virtual-call-ctor", 2, {"cpp","bug"}},
    {"static-pmf", 2, {"bug"}},
    {"assert-with-side-effects", 3, {"bug"}},
    {"detaching-member", 3, {"containers","performance"}},
    {"thread-with-slots", 3, {"bug"}},
    {"reserve-candidates", 3, {"containers"}}
};

} // namespace Constants
} // namespace CppTools
