#ifndef QMLJSLINK_H
#define QMLJSLINK_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmljsastfwd_p.h>

#include <QtCore/QList>
#include <QtCore/QHash>

namespace QmlJS {

class NameId;

/*
    Helper for building a context.
*/
class Link
{
public:
    // Link all documents in snapshot reachable from doc.
    Link(Interpreter::Context *context, Document::Ptr doc, const Snapshot &snapshot);
    ~Link();

    // Get the scope chain for the currentObject inside doc.
    void scopeChainAt(Document::Ptr doc, AST::Node *currentObject);

private:
    Interpreter::Engine *engine();

    static QList<Document::Ptr> reachableDocuments(Document::Ptr startDoc, const Snapshot &snapshot);
    static AST::UiQualifiedId *qualifiedTypeNameId(AST::Node *node);

    void linkImports();

    void populateImportedTypes(Interpreter::ObjectValue *typeEnv, Document::Ptr doc);
    void importFile(Interpreter::ObjectValue *typeEnv, Document::Ptr doc,
                    AST::UiImport *import, const QString &startPath);
    void importNonFile(Interpreter::ObjectValue *typeEnv, Document::Ptr doc,
                       AST::UiImport *import);
    void importObject(Bind *bind, const QString &name, Interpreter::ObjectValue *object, NameId *targetNamespace);

private:
    Snapshot _snapshot;
    Interpreter::Context *_context;
    QList<Document::Ptr> _docs;
};

} // namespace QmlJS

#endif // QMLJSLINK_H
