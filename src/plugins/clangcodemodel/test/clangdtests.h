// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ClangCodeModel::Internal {

QObject *createClangdTestCompletion();
QObject *createClangdTestExternalChanges();
QObject *createClangdTestFindReferences();
QObject *createClangdTestFollowSymbol();
QObject *createClangdTestHighlighting();
QObject *createClangdTestIndirectChanges();
QObject *createClangdTestLocalReferences();
QObject *createClangdTestTooltips();

} // ClangCodeModel::Internal

