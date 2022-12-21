// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmljs/parser/qmljsglobal_p.h"

#include "qmljsglobal_p.h"
#include <QtCore/qstring.h>
#include <languageutils/componentversion.h>

QT_QML_BEGIN_NAMESPACE

QML_PARSER_EXPORT QStringList qQmlResolveImportPaths(QStringView uri, const QStringList &basePaths,
                                   LanguageUtils::ComponentVersion version);

QT_QML_END_NAMESPACE
