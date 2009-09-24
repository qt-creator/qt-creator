#include "qmljsast_p.h"
#include "qmljsengine_p.h"

#include "qmlexpressionundercursor.h"
#include "qmllookupcontext.h"
#include "qmlresolveexpression.h"

using namespace DuiEditor;
using namespace DuiEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlLookupContext::QmlLookupContext(const QStack<QmlJS::AST::Node *> &scopes,
                                   const DuiDocument::Ptr &doc,
                                   const Snapshot &snapshot):
        _scopes(scopes),
        _doc(doc),
        _snapshot(snapshot)
{
}

QmlLookupContext::~QmlLookupContext()
{
    qDeleteAll(_temporarySymbols);
}

QmlSymbol *QmlLookupContext::resolve(const QString &name)
{
    // look at property definitions
    if (QmlSymbol *propertySymbol = resolveProperty(name, _scopes.top(), _doc->fileName()))
        return propertySymbol;

    if (name == "parent") {
        for (int i = _scopes.size() - 2; i >= 0; --i) {
            Node *scope = _scopes.at(i);

            if (UiObjectDefinition *definition = cast<UiObjectDefinition*>(scope))
                return createSymbol(_doc->fileName(), definition);
            else if (UiObjectBinding *binding = cast<UiObjectBinding*>(scope))
                return createSymbol(_doc->fileName(), binding);
        }

        return 0;
    }

    // look at the ids.
    const DuiDocument::IdTable ids = _doc->ids();

    if (ids.contains(name))
        return ids[name];
    else
        return 0;
}

QmlSymbol *QmlLookupContext::createSymbol(const QString &fileName, QmlJS::AST::UiObjectMember *node)
{
    QmlSymbol *symbol = new QmlSymbolFromFile(fileName, node);
    _temporarySymbols.append(symbol);
    return symbol;
}

QmlSymbol *QmlLookupContext::resolveType(const QString &name, const QString &fileName)
{
    // TODO: handle import-as.
    DuiDocument::Ptr document = _snapshot[fileName];
    if (document.isNull())
        return 0;

    UiProgram *prog = document->program();
    if (!prog)
        return 0;

    UiImportList *imports = prog->imports;
    if (!imports)
        return 0;

    for (UiImportList *iter = imports; iter; iter = iter->next) {
        UiImport *import = iter->import;
        if (!import)
            continue;

        if (!(import->fileName))
            continue;

        const QString path = import->fileName->asString();

        const QMap<QString, DuiDocument::Ptr> importedTypes = _snapshot.componentsDefinedByImportedDocuments(document, path);
        if (importedTypes.contains(name)) {
            DuiDocument::Ptr importedDoc = importedTypes.value(name);

            UiProgram *importedProgram = importedDoc->program();
            if (importedProgram && importedProgram->members && importedProgram->members->member)
                return createSymbol(importedDoc->fileName(), importedProgram->members->member);
        }
    }

    return 0;
}

QmlSymbol *QmlLookupContext::resolveProperty(const QString &name, Node *scope, const QString &fileName)
{
    UiQualifiedId *typeName = 0;

    if (UiObjectBinding *binding = cast<UiObjectBinding*>(scope)) {
        if (QmlSymbol *symbol = resolveProperty(name, binding->initializer, fileName))
            return symbol;
        else
            typeName = binding->qualifiedTypeNameId;
    } else if (UiObjectDefinition *definition = cast<UiObjectDefinition*>(scope)) {
        if (QmlSymbol *symbol = resolveProperty(name, definition->initializer, fileName))
            return symbol;
        else
            typeName = definition->qualifiedTypeNameId;
    } // TODO: extend this to handle (JavaScript) block scopes.

    if (typeName == 0)
        return 0;

    QmlSymbol *typeSymbol = resolveType(toString(typeName), fileName);
    if (typeSymbol && typeSymbol->isSymbolFromFile()) {
        return resolveProperty(name, typeSymbol->asSymbolFromFile()->node(), typeSymbol->asSymbolFromFile()->fileName());
    }

    return 0;
}

QmlSymbol *QmlLookupContext::resolveProperty(const QString &name, QmlJS::AST::UiObjectInitializer *initializer, const QString &fileName)
{
    if (!initializer)
        return 0;

    for (UiObjectMemberList *iter = initializer->members; iter; iter = iter->next) {
        UiObjectMember *member = iter->member;
        if (!member)
            continue;

        if (UiPublicMember *publicMember = cast<UiPublicMember*>(member)) {
            if (name == publicMember->name->asString())
                return createSymbol(fileName, publicMember);
        } else if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(member)) {
            if (name == toString(objectBinding->qualifiedId))
                return createSymbol(fileName, objectBinding);
        } else if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(member)) {
            if (name == toString(arrayBinding->qualifiedId))
                return createSymbol(fileName, arrayBinding);
        } else if (UiScriptBinding *scriptBinding = cast<UiScriptBinding*>(member)) {
            if (name == toString(scriptBinding->qualifiedId))
                return createSymbol(fileName, scriptBinding);
        }
    }

    return 0;
}

QString QmlLookupContext::toString(UiQualifiedId *id)
{
    QString str;

    for (UiQualifiedId *iter = id; iter; iter = iter->next) {
        if (!(iter->name))
            continue;

        str.append(iter->name->asString());

        if (iter->next)
            str.append('.');
    }

    return str;
}

QList<QmlSymbol*> QmlLookupContext::visibleSymbols(QmlJS::AST::Node *scope)
{
    // FIXME
}

QList<QmlSymbol*> QmlLookupContext::visibleTypes()
{
    QList<QmlSymbol*> result;

    UiProgram *program = _doc->program();
    if (!program)
        return result;

    for (UiImportList *iter = program->imports; iter; iter = iter->next) {
        UiImport *import = iter->import;
        if (!import)
            continue;

        if (!(import->fileName))
            continue;
        const QString path = import->fileName->asString();

        // TODO: handle "import as".

        const QMap<QString, DuiDocument::Ptr> types = _snapshot.componentsDefinedByImportedDocuments(_doc, path);
        foreach (const DuiDocument::Ptr typeDoc, types) {
            UiProgram *typeProgram = typeDoc->program();

            if (typeProgram && typeProgram->members && typeProgram->members->member) {
                result.append(createSymbol(typeDoc->fileName(), typeProgram->members->member));
            }
        }
    }

    return result;
}
