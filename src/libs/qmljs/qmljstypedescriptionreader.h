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
#include <qmljs/parser/qmljsastfwd_p.h>

#include <languageutils/fakemetaobject.h>

// for Q_DECLARE_TR_FUNCTIONS
#include <QCoreApplication>

QT_BEGIN_NAMESPACE
class QIODevice;
class QBuffer;
QT_END_NAMESPACE

namespace QmlJS {

class ModuleApiInfo;
namespace AST {
class UiProgram;
class UiObjectDefinition;
class UiScriptBinding;
class SourceLocation;
}

class QMLJS_EXPORT TypeDescriptionReader
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::TypeDescriptionReader)

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

    void addError(const AST::SourceLocation &loc, const QString &message);
    void addWarning(const AST::SourceLocation &loc, const QString &message);

    QString _fileName;
    QString _source;
    QString _errorMessage;
    QString _warningMessage;
    QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> *_objects;
    QList<ModuleApiInfo> *_moduleApis;
    QStringList *_dependencies;
};

} // namespace QmlJS
