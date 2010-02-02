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
    // Link all documents in snapshot reachable from doc.
    Link(Document::Ptr doc, const Snapshot &snapshot, Interpreter::Engine *interp);
    ~Link();

    // Get the scope chain for the currentObject inside doc.
    Interpreter::ObjectValue *scopeChainAt(Document::Ptr doc, AST::UiObjectMember *currentObject);

private:
    static QList<Document::Ptr> reachableDocuments(Document::Ptr startDoc, const Snapshot &snapshot);
    static const Interpreter::ObjectValue *lookupType(Interpreter::ObjectValue *env, AST::UiQualifiedId *id);

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
    QList<Document::Ptr> _docs;
    QHash<Document *, Interpreter::ObjectValue *> _typeEnvironments;
};

} // namespace QmlJS

#endif // QMLJSLINK_H
