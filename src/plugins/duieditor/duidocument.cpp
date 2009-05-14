#include "duidocument.h"
#include "parser/javascriptast_p.h"
#include "parser/javascriptlexer_p.h"
#include "parser/javascriptparser_p.h"
#include "parser/javascriptengine_p.h"
#include "parser/javascriptnodepool_p.h"

using namespace DuiEditor;
using namespace DuiEditor::Internal;
using namespace JavaScript;

DuiDocument::DuiDocument(const QString &fileName)
    : _engine(0), _pool(0), _program(0), _fileName(fileName)
{
}

DuiDocument::~DuiDocument()
{
    delete _engine;
    delete _pool;
}

DuiDocument::Ptr DuiDocument::create(const QString &fileName)
{
    DuiDocument::Ptr doc(new DuiDocument(fileName));
    return doc;
}

AST::UiProgram *DuiDocument::program() const
{
    return _program;
}

QList<JavaScriptParser::DiagnosticMessage> DuiDocument::diagnosticMessages() const
{
    return _diagnosticMessages;
}

void DuiDocument::setSource(const QString &source)
{
    _source = source;
}

bool DuiDocument::parse()
{
    Q_ASSERT(! _engine);
    Q_ASSERT(! _pool);
    Q_ASSERT(! _program);

    _engine = new JavaScriptEnginePrivate();
    _pool = new NodePool(_fileName, _engine);

    JavaScriptParser parser;

    NodePool nodePool(_fileName, _engine);
    _engine->setNodePool(_pool);

    Lexer lexer(_engine);
    _engine->setLexer(&lexer);

    lexer.setCode(_source, /*line = */ 1);

    bool parsed = parser.parse(_engine);
    _program = parser.ast();
    _diagnosticMessages = parser.diagnosticMessages();
    return parsed;
}

Snapshot::Snapshot()
{
}

Snapshot::~Snapshot()
{
}


