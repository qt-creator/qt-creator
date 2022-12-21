// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "parser/qmljsastfwd_p.h"
#include "parser/qmljsdiagnosticmessage_p.h"
#include "parser/qmljsengine_p.h"
#include "qmljs_global.h"
#include "qmljsconstants.h"
#include <utils/filepath.h>

QT_FORWARD_DECLARE_CLASS(QColor)

namespace QmlJS {

QMLJS_EXPORT QColor toQColor(const QString &qmlColorString);
QMLJS_EXPORT QString toString(AST::UiQualifiedId *qualifiedId,
                              const QChar delimiter = QLatin1Char('.'));

QMLJS_EXPORT SourceLocation locationFromRange(const SourceLocation &start,
                                                   const SourceLocation &end);

QMLJS_EXPORT SourceLocation fullLocationForQualifiedId(AST::UiQualifiedId *);

QMLJS_EXPORT QString idOfObject(AST::Node *object, AST::UiScriptBinding **idBinding = nullptr);

QMLJS_EXPORT AST::UiObjectInitializer *initializerOfObject(AST::Node *object);

QMLJS_EXPORT AST::UiQualifiedId *qualifiedTypeNameId(AST::Node *node);

QMLJS_EXPORT bool isValidBuiltinPropertyType(const QString &name);

QMLJS_EXPORT DiagnosticMessage errorMessage(const SourceLocation &loc,
                                            const QString &message);

QMLJS_EXPORT bool maybeModuleVersion(const QString &version);

QMLJS_EXPORT const QStringList splitVersion(const QString &version);
QMLJS_EXPORT QList<Utils::FilePath> modulePaths(const QString &moduleImportName,
                                                const QString &version,
                                                const QList<Utils::FilePath> &importPaths);

template <class T>
SourceLocation locationFromRange(const T *node)
{
    return locationFromRange(node->firstSourceLocation(), node->lastSourceLocation());
}

template <class T>
DiagnosticMessage errorMessage(const T *node, const QString &message)
{
    return DiagnosticMessage(QmlJS::Severity::Error,
                             locationFromRange(node),
                             message);
}

} // namespace QmlJS
