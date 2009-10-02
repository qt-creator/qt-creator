#include "qmljsast_p.h"
#include "qmljsengine_p.h"

#include "qmlexpressionundercursor.h"
#include "qmllookupcontext.h"
#include "qmlresolveexpression.h"

using namespace QmlEditor;
using namespace QmlEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlLookupContext::QmlLookupContext(const QStack<QmlSymbol *> &scopes,
                                   const QmlDocument::Ptr &doc,
                                   const Snapshot &snapshot):
        _scopes(scopes),
        _doc(doc),
        _snapshot(snapshot)
{
}

static inline int findFirstQmlObjectScope(const QStack<QmlSymbol*> &scopes, int startIdx)
{
    if (startIdx < 0 || startIdx >= scopes.size())
        return -1;

    for (int i = startIdx; i >= 0; --i) {
        QmlSymbol *current = scopes.at(i);

        if (current->isSymbolFromFile()) {
            Node *node = current->asSymbolFromFile()->node();

            if (cast<UiObjectBinding*>(node) || cast<UiObjectDefinition*>(node)) {
                return i;
            }
        }
    }

    return -1;
}

static inline QmlSymbol *resolveParent(const QStack<QmlSymbol*> &scopes)
{
    int idx = findFirstQmlObjectScope(scopes, scopes.size() - 1);
    if (idx < 1)
        return 0;

    idx = findFirstQmlObjectScope(scopes, idx - 1);

    if (idx < 0)
        return 0;
    else
        return scopes.at(idx);
}

QmlSymbol *QmlLookupContext::resolve(const QString &name)
{
    // look at property definitions
    if (!_scopes.isEmpty())
        if (QmlSymbol *propertySymbol = resolveProperty(name, _scopes.top(), _doc->fileName()))
            return propertySymbol;

    if (name == "parent") {
        return resolveParent(_scopes);
    }

    // look at the ids.
    const QmlDocument::IdTable ids = _doc->ids();

    if (ids.contains(name))
        return ids[name];
    else
        return 0;
}

QmlSymbol *QmlLookupContext::resolveType(const QString &name, const QString &fileName)
{
    // TODO: handle import-as.
    QmlDocument::Ptr document = _snapshot[fileName];
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

        const QMap<QString, QmlDocument::Ptr> importedTypes = _snapshot.componentsDefinedByImportedDocuments(document, path);
        if (importedTypes.contains(name)) {
            QmlDocument::Ptr importedDoc = importedTypes.value(name);

            return importedDoc->symbols().at(0);
        }
    }

    return 0;
}

QmlSymbol *QmlLookupContext::resolveProperty(const QString &name, QmlSymbol *scope, const QString &fileName)
{
    foreach (QmlSymbol *symbol, scope->members())
        if (symbol->name() == name)
            return symbol;

    UiQualifiedId *typeName = 0;

    if (scope->isSymbolFromFile()) {
        Node *ast = scope->asSymbolFromFile()->node();

        if (UiObjectBinding *binding = cast<UiObjectBinding*>(ast)) {
            typeName = binding->qualifiedTypeNameId;
        } else if (UiObjectDefinition *definition = cast<UiObjectDefinition*>(ast)) {
            typeName = definition->qualifiedTypeNameId;
        } // TODO: extend this to handle (JavaScript) block scopes.
    }

    if (typeName == 0)
        return 0;

    QmlSymbol *typeSymbol = resolveType(toString(typeName), fileName);
    if (!typeSymbol)
        return 0;

    if (typeSymbol->isSymbolFromFile()) {
        return resolveProperty(name, typeSymbol->asSymbolFromFile(), typeSymbol->asSymbolFromFile()->fileName());
    } // TODO: internal types

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

QList<QmlSymbol*> QmlLookupContext::visibleSymbolsInScope()
{
    QList<QmlSymbol*> result;

    if (!_scopes.isEmpty()) {
        QmlSymbol *scope = _scopes.top();

        result.append(scope->members());
    }

    return result;
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

        const QMap<QString, QmlDocument::Ptr> types = _snapshot.componentsDefinedByImportedDocuments(_doc, path);
        foreach (const QmlDocument::Ptr typeDoc, types) {
            result.append(typeDoc->symbols().at(0));
        }
    }

    return result;
}
