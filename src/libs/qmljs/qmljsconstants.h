// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljs_global.h"

#include <QLoggingCategory>

namespace QmlJS {

namespace ImportType {
enum Enum {
    Invalid,
    Library,
    Directory,
    ImplicitDirectory,
    File,
    UnknownFile, // refers a file/directory that wasn't found (or to an url)
    QrcDirectory,
    QrcFile
};
}

namespace ImportKind {
enum Enum {
    Invalid,
    Library,
    Path,
    QrcPath
};
}

namespace Severity {
enum Enum
{
    Hint,                   // cosmetic or convention
    MaybeWarning,           // possibly a warning, insufficient information
    Warning,                // could cause unintended behavior
    ReadingTypeInfoWarning, // currently dumping type information
    MaybeError,             // possibly an error, insufficient information
    Error                   // definitely an error
};
}

namespace Language {
enum Enum
{
    NoLanguage = 0,
    JavaScript = 1,
    Json = 2,
    Qml = 3,
    QmlQtQuick2 = 5,
    QmlQbs = 6,
    QmlProject = 7,
    QmlTypeInfo = 8,
    AnyLanguage = 9,
};
}

namespace Constants {

const char TASK_INDEX[] = "QmlJSEditor.TaskIndex";
const char TASK_IMPORT_SCAN[] = "QmlJSEditor.TaskImportScan";

} // namespace Constants

QMLJS_EXPORT Q_DECLARE_LOGGING_CATEGORY(qmljsLog)

} // namespace QmlJS
