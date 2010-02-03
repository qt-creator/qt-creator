#ifndef QMLJSLINK_H
#define QMLJSLINK_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QtCore/QList>
#include <QtCore/QHash>

namespace QmlJS {

/*
    Temporarily links a set of bound documents together to allow resolving cross-document
    dependencies. The link is undone when this object is destoyed.
*/
class QMLJS_EXPORT Link
{
public:
    typedef QList<const Interpreter::ObjectValue *> ScopeChain;

public:
    // Link all documents in snapshot reachable from doc.
    Link(Document::Ptr doc, const Snapshot &snapshot, Interpreter::Engine *interp);
    ~Link();

    Interpreter::Context *context();
    ScopeChain scopeChain() const;
    Interpreter::Engine *engine();

    // Get the scope chain for the currentObject inside doc.
    void scopeChainAt(Document::Ptr doc, AST::Node *currentObject);

    const Interpreter::Value *lookup(const QString &name) const;

private:
    static QList<Document::Ptr> reachableDocuments(Document::Ptr startDoc, const Snapshot &snapshot);
    static const Interpreter::ObjectValue *lookupType(Interpreter::ObjectValue *env, AST::UiQualifiedId *id);
    static AST::UiQualifiedId *qualifiedTypeNameId(AST::Node *node);

    void linkImports();

    void populateImportedTypes(Interpreter::ObjectValue *typeEnv, Document::Ptr doc);
    void importFile(Interpreter::ObjectValue *typeEnv, Document::Ptr doc,
                    AST::UiImport *import, const QString &startPath);
    void importNonFile(Interpreter::ObjectValue *typeEnv, Document::Ptr doc,
                       AST::UiImport *import);
    void importObject(BindPtr bind, const QString &name, Interpreter::ObjectValue *object, NameId* targetNamespace);    

private:
    Snapshot _snapshot;
    Interpreter::Engine *_interp;
    Interpreter::Context _context;
    QList<Document::Ptr> _docs;
    QHash<Document *, Interpreter::ObjectValue *> _typeEnvironments;
    ScopeChain _scopeChain;
};

} // namespace QmlJS

#endif // QMLJSLINK_H
