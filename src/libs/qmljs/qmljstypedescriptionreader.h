// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljs_global.h"
#include <qmljs/parser/qmljsastfwd_p.h>

#include <languageutils/fakemetaobject.h>

QT_BEGIN_NAMESPACE
class QIODevice;
class QBuffer;
QT_END_NAMESPACE

namespace QmlJS {
class SourceLocation;

class ModuleApiInfo;
namespace AST {
class UiProgram;
class UiObjectDefinition;
class UiScriptBinding;
}

class QMLJS_EXPORT TypeDescriptionReader
{
public:
    explicit TypeDescriptionReader(const QString &fileName, const QString &data);
    ~TypeDescriptionReader();

    bool operator()(
            QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> *objects,
            QList<ModuleApiInfo> *moduleApis,
            QStringList *dependencies);
    QString errorMessage() const;
    QString warningMessage() const;

private:
    void readDocument(AST::UiProgram *ast);
    void readModule(AST::UiObjectDefinition *ast);
    void readDependencies(AST::UiScriptBinding *ast);
    void readComponent(AST::UiObjectDefinition *ast);
    void readModuleApi(AST::UiObjectDefinition *ast);
    void readSignalOrMethod(AST::UiObjectDefinition *ast, bool isMethod, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readProperty(AST::UiObjectDefinition *ast, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readEnum(AST::UiObjectDefinition *ast, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readParameter(AST::UiObjectDefinition *ast, LanguageUtils::FakeMetaMethod *fmm);

    QString readStringBinding(AST::UiScriptBinding *ast);
    bool readBoolBinding(AST::UiScriptBinding *ast);
    double readNumericBinding(AST::UiScriptBinding *ast);
    LanguageUtils::ComponentVersion readNumericVersionBinding(AST::UiScriptBinding *ast);
    int readIntBinding(AST::UiScriptBinding *ast);
    void readExports(AST::UiScriptBinding *ast, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readMetaObjectRevisions(AST::UiScriptBinding *ast, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readEnumValues(AST::UiScriptBinding *ast, LanguageUtils::FakeMetaEnum *fme);

    void addError(const SourceLocation &loc, const QString &message);
    void addWarning(const SourceLocation &loc, const QString &message);

    QString _fileName;
    QString _source;
    QString _errorMessage;
    QString _warningMessage;
    QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> *_objects;
    QList<ModuleApiInfo> *_moduleApis = nullptr;
    QStringList *_dependencies = nullptr;
};

} // namespace QmlJS
