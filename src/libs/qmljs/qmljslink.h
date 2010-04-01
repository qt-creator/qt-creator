#ifndef QMLJSLINK_H
#define QMLJSLINK_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmljsastfwd_p.h>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QStringList>

namespace QmlJS {

class NameId;

/*
    Helper for building a context.
*/
class QMLJS_EXPORT Link
{
public:
    // Link all documents in snapshot
    Link(Interpreter::Context *context, const Document::Ptr &doc, const Snapshot &snapshot,
         const QStringList &importPaths);
    ~Link();

    // Get the scope chain for the currentObject inside doc.
    void scopeChainAt(Document::Ptr doc, const QList<AST::Node *> &astPath = QList<AST::Node *>());

    QList<DiagnosticMessage> diagnosticMessages() const;

private:
    Interpreter::Engine *engine();

    void makeComponentChain(
            Document::Ptr doc,
            Interpreter::ScopeChain::QmlComponentChain *target,
            QHash<Document *, Interpreter::ScopeChain::QmlComponentChain *> *components);

    static QList<Document::Ptr> reachableDocuments(Document::Ptr startDoc, const Snapshot &snapshot,
                                                   const QStringList &importPaths);
    static AST::UiQualifiedId *qualifiedTypeNameId(AST::Node *node);

    void linkImports();

    void populateImportedTypes(Interpreter::ObjectValue *typeEnv, Document::Ptr doc);
    void importFile(Interpreter::ObjectValue *typeEnv, Document::Ptr doc,
                    AST::UiImport *import);
    void importNonFile(Interpreter::ObjectValue *typeEnv, Document::Ptr doc,
                       AST::UiImport *import);
    void importObject(Bind *bind, const QString &name, Interpreter::ObjectValue *object, NameId *targetNamespace);

    void error(const Document::Ptr &doc, const AST::SourceLocation &loc, const QString &message);

private:
    Document::Ptr _doc;
    Snapshot _snapshot;
    Interpreter::Context *_context;
    const QStringList _importPaths;

    QList<DiagnosticMessage> _diagnosticMessages;
};

} // namespace QmlJS

#endif // QMLJSLINK_H
