#ifndef DUIDOCUMENT_H
#define DUIDOCUMENT_H

#include <QtCore/QSharedPointer>
#include <QtCore/QMap>
#include <QtCore/QString>

#include "parser/javascriptparser_p.h"

class JavaScriptEnginePrivate;

namespace JavaScript {

class NodePool;

namespace AST {

class UiProgram;

} // end of namespace AST
} // end of namespace JavaScript



namespace DuiEditor {
namespace Internal {

class DuiDocument
{
public:
    typedef QSharedPointer<DuiDocument> Ptr;

protected:
    DuiDocument(const QString &fileName);

public:
    ~DuiDocument();

    static DuiDocument::Ptr create(const QString &fileName);

    JavaScript::AST::UiProgram *program() const;
    QList<JavaScriptParser::DiagnosticMessage> diagnosticMessages() const;

    void setSource(const QString &source);
    bool parse();

private:
    JavaScriptEnginePrivate *_engine;
    JavaScript::NodePool *_pool;
    JavaScript::AST::UiProgram *_program;
    QList<JavaScriptParser::DiagnosticMessage> _diagnosticMessages;
    QString _fileName;
    QString _source;
};

class Snapshot: public QMap<QString, DuiDocument>
{
public:
    Snapshot();
    ~Snapshot();
};

} // end of namespace Internal
} // emd of namespace DuiEditor

#endif // DUIDOCUMENT_H
