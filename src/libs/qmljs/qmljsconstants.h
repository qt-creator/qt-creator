/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    Hint,         // cosmetic or convention
    MaybeWarning, // possibly a warning, insufficient information
    Warning,      // could cause unintended behavior
    MaybeError,   // possibly an error, insufficient information
    Error         // definitely an error
};
}

namespace Language {
enum Enum
{
    NoLanguage = 0,
    JavaScript = 1,
    Json = 2,
    Qml = 3,
    QmlQtQuick1 = 4,
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
