#ifndef QMLCODEFORMATTER_H
#define QMLCODEFORMATTER_H

#include <QString>

#include "qmljsastfwd_p.h"
#include "qmljsastvisitor_p.h"
#include "qmljsengine_p.h"

namespace QmlEditor {
namespace Internal {

class QmlCodeFormatter: protected QmlJS::AST::Visitor
{
public:
    QmlCodeFormatter();
    ~QmlCodeFormatter();

    QString operator()(QmlJS::AST::UiProgram *ast, const QString &originalSource, const QList<QmlJS::AST::SourceLocation> &comments, int start = -1, int end = -1);

protected:
    virtual bool visit(QmlJS::AST::UiProgram *ast);
//    virtual bool visit(UiImportList *ast);
    virtual bool visit(QmlJS::AST::UiImport *ast);
//    virtual bool visit(UiPublicMember *ast);
//    virtual bool visit(UiSourceElement *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiObjectInitializer *ast);
//    virtual bool visit(UiObjectBinding *ast);
//    virtual bool visit(UiScriptBinding *ast);
//    virtual bool visit(UiArrayBinding *ast);
//    virtual bool visit(UiObjectMemberList *ast);
//    virtual bool visit(UiArrayMemberList *ast);
    virtual bool visit(QmlJS::AST::UiQualifiedId *ast);
//    virtual bool visit(UiSignature *ast);
//    virtual bool visit(UiFormalList *ast);
//    virtual bool visit(UiFormal *ast);
//
//    virtual void endVisit(UiProgram *ast);
//    virtual void endVisit(UiImport *ast);
//    virtual void endVisit(UiPublicMember *ast);
//    virtual void endVisit(UiSourceElement *ast);
//    virtual void endVisit(UiObjectDefinition *ast);
//    virtual void endVisit(UiObjectInitializer *ast);
//    virtual void endVisit(UiObjectBinding *ast);
//    virtual void endVisit(UiScriptBinding *ast);
//    virtual void endVisit(UiArrayBinding *ast);
//    virtual void endVisit(UiObjectMemberList *ast);
//    virtual void endVisit(UiArrayMemberList *ast);
//    virtual void endVisit(UiQualifiedId *ast);
//    virtual void endVisit(UiSignature *ast);
//    virtual void endVisit(UiFormalList *ast);
//    virtual void endVisit(UiFormal *ast);

private:
    void append(char c) { m_result += c; }
    void append(const char *s) { m_result += s; }
    void append(const QString &s) { m_result += s; }
    void append(const QmlJS::AST::SourceLocation &loc) { m_result += textAt(loc); }
    void append(int pos, int len) { append(textAt(pos, len)); }

    QString textAt(const QmlJS::AST::SourceLocation &loc) const
    { return textAt(loc.offset, loc.length); }

    QString textAt(int pos, int len) const
    { return m_originalSource.mid(pos, len); }

    void indent() { if (m_indentDepth) append(QString(' ', m_indentDepth)); }
    void newline() { append('\n'); }

private:
    QString m_result;
    QString m_originalSource;
    int m_start;
    int m_end;
    unsigned m_indentDepth;
};

} // namespace Internal
} // namespace QmlEditor

#endif // QMLCODEFORMATTER_H
