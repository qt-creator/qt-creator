// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CppDocument.h>

QT_BEGIN_NAMESPACE
class QObject;
class QTextDocument;
QT_END_NAMESPACE

namespace CppEditor::Internal {

enum MocIncludeMode { RespectMocIncludes, IgnoreMocIncludes };
enum IncludeStyle { LocalBeforeGlobal, GlobalBeforeLocal, AutoDetect };

int lineForNewIncludeDirective(const Utils::FilePath &filePath, const QTextDocument *textDocument,
                               const CPlusPlus::Document::Ptr cppDocument,
                               MocIncludeMode mocIncludeMode,
                               IncludeStyle includeStyle,
                               const QString &newIncludeFileName,
                               unsigned *newLinesToPrepend,
                               unsigned *newLinesToAppend);

QObject *createIncludeGroupsTest();

} // CppEditor::Internal
