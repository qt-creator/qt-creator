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
    // ### TODO: look at property definitions
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

QmlSymbol *QmlLookupContext::resolveType(const QString &name)
{
    UiProgram *prog = _doc->program();
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

        const QMap<QString, DuiDocument::Ptr> importedTypes = _snapshot.componentsDefinedByImportedDocuments(_doc, path);
        if (importedTypes.contains(name)) {
            DuiDocument::Ptr importedDoc = importedTypes.value(name);

            UiProgram *importedProgram = importedDoc->program();
            if (importedProgram && importedProgram->members && importedProgram->members->member)
                return createSymbol(importedDoc->fileName(), importedProgram->members->member);
        }
    }

    return 0;
}
