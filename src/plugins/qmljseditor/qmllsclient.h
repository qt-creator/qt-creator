// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljseditor_global.h"

#include <utils/filepath.h>

#include <languageclient/client.h>
#include <languageclient/languageclientinterface.h>

namespace QmlJSEditor {

class QMLJSEDITOR_EXPORT QmllsClient : public LanguageClient::Client
{
    Q_OBJECT
public:
    // Only token types that overlap with the token types registered in the language
    // server are highlighted.
    // Therefore, this should be matched with the server's token types
    enum QmlSemanticTokens {
        // Subset of the QLspSpefication::SemanticTokenTypes enum
        // We register only the token types used in the qml semantic highlighting
        Namespace,
        Type,
        Enum,
        Parameter,
        Variable,
        Property,
        EnumMember,
        Method,
        Keyword,
        Comment,
        String,
        Number,
        Regexp,
        Operator,
        Decorator,

        // Additional token types for the extended semantic highlighting
        QmlLocalId, // object id within the same file
        QmlExternalId, // object id defined in another file
        QmlRootObjectProperty, // qml property defined in the parent scopes
        QmlScopeObjectProperty, // qml property defined in the current scope
        QmlExternalObjectProperty, // qml property defined in the root object of another file
        JsScopeVar, // js variable defined in the current file
        JsImportVar, // js import name that is imported in the qml file
        JsGlobalVar, // js global variables
        QmlStateName // name of a qml state
    };
    Q_ENUM(QmlSemanticTokens);
    explicit QmllsClient(LanguageClient::StdIOClientInterface *interface);
    ~QmllsClient();

    void startImpl() override;
    static QmllsClient *clientForQmlls(const Utils::FilePath &qmlls);
private:
    static QMap<QString, int> semanticTokenTypesMap();
};

} // namespace QmlJSEditor
