/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljstypedescriptionreader.h"

#include "parser/qmljsparser_p.h"
#include "parser/qmljslexer_p.h"
#include "parser/qmljsengine_p.h"
#include "parser/qmljsast_p.h"
#include "parser/qmljsastvisitor_p.h"

#include "qmljsbind.h"
#include "qmljsinterpreter.h"
#include "qmljsutils.h"

#include <QIODevice>
#include <QBuffer>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace LanguageUtils;

TypeDescriptionReader::TypeDescriptionReader(const QString &data)
    : _source(data)
    , _objects(0)
{
}

TypeDescriptionReader::~TypeDescriptionReader()
{
}

bool TypeDescriptionReader::operator()(
        QHash<QString, FakeMetaObject::ConstPtr> *objects,
        QList<ModuleApiInfo> *moduleApis)
{
    Engine engine;

    Lexer lexer(&engine);
    Parser parser(&engine);

    lexer.setCode(_source, /*line = */ 1, /*qmlMode = */true);

    if (!parser.parse()) {
        _errorMessage = QString::fromLatin1("%1:%2: %3").arg(
                    QString::number(parser.errorLineNumber()),
                    QString::number(parser.errorColumnNumber()),
                    parser.errorMessage());
        return false;
    }

    _objects = objects;
    _moduleApis = moduleApis;
    readDocument(parser.ast());

    return _errorMessage.isEmpty();
}

QString TypeDescriptionReader::errorMessage() const
{
    return _errorMessage;
}

QString TypeDescriptionReader::warningMessage() const
{
    return _warningMessage;
}

void TypeDescriptionReader::readDocument(UiProgram *ast)
{
    if (!ast) {
        addError(SourceLocation(), tr("Could not parse document"));
        return;
    }

    if (!ast->imports || ast->imports->next) {
        addError(SourceLocation(), tr("Expected a single import"));
        return;
    }

    UiImport *import = ast->imports->import;
    if (toString(import->importUri) != QLatin1String("QtQuick.tooling")) {
        addError(import->importToken, tr("Expected import of QtQuick.tooling"));
        return;
    }

    ComponentVersion version;
    const QString versionString = _source.mid(import->versionToken.offset, import->versionToken.length);
    const int dotIdx = versionString.indexOf(QLatin1Char('.'));
    if (dotIdx != -1) {
        version = ComponentVersion(versionString.left(dotIdx).toInt(),
                                   versionString.mid(dotIdx + 1).toInt());
    }
    if (version > ComponentVersion(1, 1)) {
        addError(import->versionToken, tr("Expected version 1.1 or lower"));
        return;
    }

    if (!ast->members || !ast->members->member || ast->members->next) {
        addError(SourceLocation(), tr("Expected document to contain a single object definition"));
        return;
    }

    UiObjectDefinition *module = dynamic_cast<UiObjectDefinition *>(ast->members->member);
    if (!module) {
        addError(SourceLocation(), tr("Expected document to contain a single object definition"));
        return;
    }

    if (toString(module->qualifiedTypeNameId) != QLatin1String("Module")) {
        addError(SourceLocation(), tr("Expected document to contain a Module {} member"));
        return;
    }

    readModule(module);
}

void TypeDescriptionReader::readModule(UiObjectDefinition *ast)
{
    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        UiObjectDefinition *component = dynamic_cast<UiObjectDefinition *>(member);

        QString typeName;
        if (component)
            typeName = toString(component->qualifiedTypeNameId);

        if (!component || (typeName != QLatin1String("Component") && typeName != QLatin1String("ModuleApi"))) {
            addWarning(member->firstSourceLocation(),
                       tr("Expected only 'Component' and 'ModuleApi' object definitions"));
            continue;
        }

        if (typeName == QLatin1String("Component"))
            readComponent(component);
        else if (typeName == QLatin1String("ModuleApi"))
            readModuleApi(component);
    }
}

void TypeDescriptionReader::addError(const SourceLocation &loc, const QString &message)
{
    _errorMessage += QString::fromLatin1("%1:%2: %3\n").arg(
                QString::number(loc.startLine),
                QString::number(loc.startColumn),
                message);
}

void TypeDescriptionReader::addWarning(const SourceLocation &loc, const QString &message)
{
    _warningMessage += QString::fromLatin1("%1:%2: %3\n").arg(
                QString::number(loc.startLine),
                QString::number(loc.startColumn),
                message);
}

void TypeDescriptionReader::readComponent(UiObjectDefinition *ast)
{
    FakeMetaObject::Ptr fmo(new FakeMetaObject);

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        UiObjectDefinition *component = dynamic_cast<UiObjectDefinition *>(member);
        UiScriptBinding *script = dynamic_cast<UiScriptBinding *>(member);
        if (component) {
            QString name = toString(component->qualifiedTypeNameId);
            if (name == QLatin1String("Property"))
                readProperty(component, fmo);
            else if (name == QLatin1String("Method") || name == QLatin1String("Signal"))
                readSignalOrMethod(component, name == QLatin1String("Method"), fmo);
            else if (name == QLatin1String("Enum"))
                readEnum(component, fmo);
            else
                addWarning(component->firstSourceLocation(), tr("Expected only Property, Method, Signal and Enum object definitions"));
        } else if (script) {
            QString name = toString(script->qualifiedId);
            if (name == QLatin1String("name")) {
                fmo->setClassName(readStringBinding(script));
            } else if (name == QLatin1String("prototype")) {
                fmo->setSuperclassName(readStringBinding(script));
            } else if (name == QLatin1String("defaultProperty")) {
                fmo->setDefaultPropertyName(readStringBinding(script));
            } else if (name == QLatin1String("exports")) {
                readExports(script, fmo);
            } else if (name == QLatin1String("exportMetaObjectRevisions")) {
                readMetaObjectRevisions(script, fmo);
            } else if (name == QLatin1String("attachedType")) {
                fmo->setAttachedTypeName(readStringBinding(script));
            } else {
                addWarning(script->firstSourceLocation(),
                           tr("Expected only name, prototype, defaultProperty, attachedType, exports "
                              "and exportMetaObjectRevisions script bindings"));
            }
        } else {
            addWarning(member->firstSourceLocation(), tr("Expected only script bindings and object definitions"));
        }
    }

    if (fmo->className().isEmpty()) {
        addError(ast->firstSourceLocation(), tr("Component definition is missing a name binding"));
        return;
    }

    // ### add implicit export into the package of c++ types
    fmo->addExport(fmo->className(), QmlJS::CppQmlTypes::cppPackage, ComponentVersion());
    _objects->insert(fmo->className(), fmo);
}

void TypeDescriptionReader::readModuleApi(UiObjectDefinition *ast)
{
    ModuleApiInfo apiInfo;

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        UiScriptBinding *script = dynamic_cast<UiScriptBinding *>(member);

        if (script) {
            const QString name = toString(script->qualifiedId);
            if (name == QLatin1String("uri")) {
                apiInfo.uri = readStringBinding(script);
            } else if (name == QLatin1String("version")) {
                apiInfo.version = readNumericVersionBinding(script);
            } else if (name == QLatin1String("name")) {
                apiInfo.cppName = readStringBinding(script);
            } else {
                addWarning(script->firstSourceLocation(),
                           tr("Expected only uri, version and name script bindings"));
            }
        } else {
            addWarning(member->firstSourceLocation(), tr("Expected only script bindings"));
        }
    }

    if (!apiInfo.version.isValid()) {
        addError(ast->firstSourceLocation(), tr("ModuleApi definition has no or invalid 'version' binding"));
        return;
    }

    if (_moduleApis)
        _moduleApis->append(apiInfo);
}

void TypeDescriptionReader::readSignalOrMethod(UiObjectDefinition *ast, bool isMethod, FakeMetaObject::Ptr fmo)
{
    FakeMetaMethod fmm;
    // ### confusion between Method and Slot. Method should be removed.
    if (isMethod)
        fmm.setMethodType(FakeMetaMethod::Slot);
    else
        fmm.setMethodType(FakeMetaMethod::Signal);

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        UiObjectDefinition *component = dynamic_cast<UiObjectDefinition *>(member);
        UiScriptBinding *script = dynamic_cast<UiScriptBinding *>(member);
        if (component) {
            QString name = toString(component->qualifiedTypeNameId);
            if (name == QLatin1String("Parameter"))
                readParameter(component, &fmm);
            else
                addWarning(component->firstSourceLocation(), tr("Expected only Parameter object definitions"));
        } else if (script) {
            QString name = toString(script->qualifiedId);
            if (name == QLatin1String("name"))
                fmm.setMethodName(readStringBinding(script));
            else if (name == QLatin1String("type"))
                fmm.setReturnType(readStringBinding(script));
            else if (name == QLatin1String("revision"))
                fmm.setRevision(readIntBinding(script));
            else
                addWarning(script->firstSourceLocation(), tr("Expected only name and type script bindings"));

        } else {
            addWarning(member->firstSourceLocation(), tr("Expected only script bindings and object definitions"));
        }
    }

    if (fmm.methodName().isEmpty()) {
        addError(ast->firstSourceLocation(), tr("Method or Signal is missing a name script binding"));
        return;
    }

    fmo->addMethod(fmm);
}

void TypeDescriptionReader::readProperty(UiObjectDefinition *ast, FakeMetaObject::Ptr fmo)
{
    QString name;
    QString type;
    bool isPointer = false;
    bool isReadonly = false;
    bool isList = false;
    int revision = 0;

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        UiScriptBinding *script = dynamic_cast<UiScriptBinding *>(member);
        if (!script) {
            addWarning(member->firstSourceLocation(), tr("Expected script binding"));
            continue;
        }

        QString id = toString(script->qualifiedId);
        if (id == QLatin1String("name"))
            name = readStringBinding(script);
        else if (id == QLatin1String("type"))
            type = readStringBinding(script);
        else if (id == QLatin1String("isPointer"))
            isPointer = readBoolBinding(script);
        else if (id == QLatin1String("isReadonly"))
            isReadonly = readBoolBinding(script);
        else if (id == QLatin1String("isList"))
            isList = readBoolBinding(script);
        else if (id == QLatin1String("revision"))
            revision = readIntBinding(script);
        else
            addWarning(script->firstSourceLocation(), tr("Expected only type, name, revision, isPointer, isReadonly and isList script bindings"));
    }

    if (name.isEmpty() || type.isEmpty()) {
        addError(ast->firstSourceLocation(), tr("Property object is missing a name or type script binding"));
        return;
    }

    fmo->addProperty(FakeMetaProperty(name, type, isList, !isReadonly, isPointer, revision));
}

void TypeDescriptionReader::readEnum(UiObjectDefinition *ast, FakeMetaObject::Ptr fmo)
{
    FakeMetaEnum fme;

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        UiScriptBinding *script = dynamic_cast<UiScriptBinding *>(member);
        if (!script) {
            addWarning(member->firstSourceLocation(), tr("Expected script binding"));
            continue;
        }

        QString name = toString(script->qualifiedId);
        if (name == QLatin1String("name"))
            fme.setName(readStringBinding(script));
        else if (name == QLatin1String("values"))
            readEnumValues(script, &fme);
        else
            addWarning(script->firstSourceLocation(), tr("Expected only name and values script bindings"));
    }

    fmo->addEnum(fme);
}

void TypeDescriptionReader::readParameter(UiObjectDefinition *ast, FakeMetaMethod *fmm)
{
    QString name;
    QString type;

    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        UiObjectMember *member = it->member;
        UiScriptBinding *script = dynamic_cast<UiScriptBinding *>(member);
        if (!script) {
            addWarning(member->firstSourceLocation(), tr("Expected script binding"));
            continue;
        }

        const QString id = toString(script->qualifiedId);
        if (id == QLatin1String("name")) {
            name = readStringBinding(script);
        } else if (id == QLatin1String("type")) {
            type = readStringBinding(script);
        } else if (id == QLatin1String("isPointer")) {
            // ### unhandled
        } else if (id == QLatin1String("isReadonly")) {
            // ### unhandled
        } else if (id == QLatin1String("isList")) {
            // ### unhandled
        } else {
            addWarning(script->firstSourceLocation(), tr("Expected only name and type script bindings"));
        }
    }

    fmm->addParameter(name, type);
}

QString TypeDescriptionReader::readStringBinding(UiScriptBinding *ast)
{
    if (!ast || !ast->statement) {
        addError(ast->colonToken, tr("Expected string after colon"));
        return QString();
    }

    ExpressionStatement *expStmt = dynamic_cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(), tr("Expected string after colon"));
        return QString();
    }

    StringLiteral *stringLit = dynamic_cast<StringLiteral *>(expStmt->expression);
    if (!stringLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected string after colon"));
        return QString();
    }

    return stringLit->value.toString();
}

bool TypeDescriptionReader::readBoolBinding(AST::UiScriptBinding *ast)
{
    if (!ast || !ast->statement) {
        addError(ast->colonToken, tr("Expected boolean after colon"));
        return false;
    }

    ExpressionStatement *expStmt = dynamic_cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(), tr("Expected boolean after colon"));
        return false;
    }

    TrueLiteral *trueLit = dynamic_cast<TrueLiteral *>(expStmt->expression);
    FalseLiteral *falseLit = dynamic_cast<FalseLiteral *>(expStmt->expression);
    if (!trueLit && !falseLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected true or false after colon"));
        return false;
    }

    return trueLit;
}

double TypeDescriptionReader::readNumericBinding(AST::UiScriptBinding *ast)
{
    if (!ast || !ast->statement) {
        addError(ast->colonToken, tr("Expected numeric literal after colon"));
        return 0;
    }

    ExpressionStatement *expStmt = AST::cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(), tr("Expected numeric literal after colon"));
        return 0;
    }

    NumericLiteral *numericLit = AST::cast<NumericLiteral *>(expStmt->expression);
    if (!numericLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected numeric literal after colon"));
        return 0;
    }

    return numericLit->value;
}

ComponentVersion TypeDescriptionReader::readNumericVersionBinding(UiScriptBinding *ast)
{
    ComponentVersion invalidVersion;

    if (!ast || !ast->statement) {
        addError(ast->colonToken, tr("Expected numeric literal after colon"));
        return invalidVersion;
    }

    ExpressionStatement *expStmt = AST::cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(), tr("Expected numeric literal after colon"));
        return invalidVersion;
    }

    NumericLiteral *numericLit = AST::cast<NumericLiteral *>(expStmt->expression);
    if (!numericLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected numeric literal after colon"));
        return invalidVersion;
    }

    return ComponentVersion(_source.mid(numericLit->literalToken.begin(), numericLit->literalToken.length));
}

int TypeDescriptionReader::readIntBinding(AST::UiScriptBinding *ast)
{
    double v = readNumericBinding(ast);
    int i = static_cast<int>(v);

    if (i != v) {
        addError(ast->firstSourceLocation(), tr("Expected integer after colon"));
        return 0;
    }

    return i;
}

void TypeDescriptionReader::readExports(UiScriptBinding *ast, FakeMetaObject::Ptr fmo)
{
    if (!ast || !ast->statement) {
        addError(ast->colonToken, tr("Expected array of strings after colon"));
        return;
    }

    ExpressionStatement *expStmt = dynamic_cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(), tr("Expected array of strings after colon"));
        return;
    }

    ArrayLiteral *arrayLit = dynamic_cast<ArrayLiteral *>(expStmt->expression);
    if (!arrayLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected array of strings after colon"));
        return;
    }

    for (ElementList *it = arrayLit->elements; it; it = it->next) {
        StringLiteral *stringLit = dynamic_cast<StringLiteral *>(it->expression);
        if (!stringLit) {
            addError(arrayLit->firstSourceLocation(), tr("Expected array literal with only string literal members"));
            return;
        }
        QString exp = stringLit->value.toString();
        int slashIdx = exp.indexOf(QLatin1Char('/'));
        int spaceIdx = exp.indexOf(QLatin1Char(' '));
        ComponentVersion version(exp.mid(spaceIdx + 1));

        if (spaceIdx == -1 || !version.isValid()) {
            addError(stringLit->firstSourceLocation(), tr("Expected string literal to contain 'Package/Name major.minor' or 'Name major.minor'"));
            continue;
        }
        QString package;
        if (slashIdx != -1)
            package = exp.left(slashIdx);
        QString name = exp.mid(slashIdx + 1, spaceIdx - (slashIdx+1));

        // ### relocatable exports where package is empty?
        fmo->addExport(name, package, version);
    }
}

void TypeDescriptionReader::readMetaObjectRevisions(UiScriptBinding *ast, FakeMetaObject::Ptr fmo)
{
    if (!ast || !ast->statement) {
        addError(ast->colonToken, tr("Expected array of numbers after colon"));
        return;
    }

    ExpressionStatement *expStmt = dynamic_cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(), tr("Expected array of numbers after colon"));
        return;
    }

    ArrayLiteral *arrayLit = dynamic_cast<ArrayLiteral *>(expStmt->expression);
    if (!arrayLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected array of numbers after colon"));
        return;
    }

    int exportIndex = 0;
    const int exportCount = fmo->exports().size();
    for (ElementList *it = arrayLit->elements; it; it = it->next, ++exportIndex) {
        NumericLiteral *numberLit = cast<NumericLiteral *>(it->expression);
        if (!numberLit) {
            addError(arrayLit->firstSourceLocation(), tr("Expected array literal with only number literal members"));
            return;
        }

        if (exportIndex >= exportCount) {
            addError(numberLit->firstSourceLocation(), tr("Meta object revision without matching export"));
            return;
        }

        const double v = numberLit->value;
        const int metaObjectRevision = static_cast<int>(v);
        if (metaObjectRevision != v) {
            addError(numberLit->firstSourceLocation(), tr("Expected integer"));
            return;
        }

        fmo->setExportMetaObjectRevision(exportIndex, metaObjectRevision);
    }
}

void TypeDescriptionReader::readEnumValues(AST::UiScriptBinding *ast, LanguageUtils::FakeMetaEnum *fme)
{
    if (!ast || !ast->statement) {
        addError(ast->colonToken, tr("Expected object literal after colon"));
        return;
    }

    ExpressionStatement *expStmt = dynamic_cast<ExpressionStatement *>(ast->statement);
    if (!expStmt) {
        addError(ast->statement->firstSourceLocation(), tr("Expected object literal after colon"));
        return;
    }

    ObjectLiteral *objectLit = dynamic_cast<ObjectLiteral *>(expStmt->expression);
    if (!objectLit) {
        addError(expStmt->firstSourceLocation(), tr("Expected object literal after colon"));
        return;
    }

    for (PropertyNameAndValueList *it = objectLit->properties; it; it = it->next) {
        StringLiteralPropertyName *propName = dynamic_cast<StringLiteralPropertyName *>(it->name);
        NumericLiteral *value = dynamic_cast<NumericLiteral *>(it->value);
        UnaryMinusExpression *minus = dynamic_cast<UnaryMinusExpression *>(it->value);
        if (minus)
            value = dynamic_cast<NumericLiteral *>(minus->expression);
        if (!propName || !value) {
            addError(objectLit->firstSourceLocation(), tr("Expected object literal to contain only 'string: number' elements"));
            continue;
        }

        double v = value->value;
        if (minus)
            v = -v;
        fme->addKey(propName->id.toString(), v);
    }
}
