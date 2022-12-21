// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <functional>

QT_BEGIN_NAMESPACE
class QLayout;
class QWidget;
QT_END_NAMESPACE

namespace ClangCodeModel {
namespace Internal {
class ClangDiagnostic;

class ClangDiagnosticWidget {
public:
    enum Destination { ToolTip, InfoBar };

    static QString createText(const QList<ClangDiagnostic> &diagnostics,
                              const Destination &destination);


    static QWidget *createWidget(const QList<ClangDiagnostic> &diagnostics,
                                 const Destination &destination,
                                 const std::function<bool()> &canApplyFixIt,
                                 const QString &source);
};

} // namespace Internal
} // namespace ClangCodeModel
