#ifndef CPLUSPLUS_FINDBREAKPOINT_H
#define CPLUSPLUS_FINDBREAKPOINT_H

#include <CPlusPlusForwardDeclarations.h>
#include <ASTVisitor.h>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT FindCdbBreakpoint: protected ASTVisitor
{
public:
    static const unsigned NO_LINE_FOUND = 0;

public:
    FindCdbBreakpoint(TranslationUnit *unit);

    unsigned operator()(unsigned line)
    { return searchFrom(line); }

    /**
     * Search for the next breakable line of code.
     *
     * \param line the starting line from where the next breakable code line
     *        should be found
     * \return the next breakable code line (1-based), or \c NO_LINE_FOUND if
     *         no line could be found.
     */
    unsigned searchFrom(unsigned line);

protected:
    void foundLine(unsigned tokenIndex);
    unsigned endLine(unsigned tokenIndex) const;
    unsigned endLine(AST *ast) const;

protected:
    using ASTVisitor::visit;

    bool preVisit(AST *ast);

    bool visit(FunctionDefinitionAST *ast);

    // Statements:
    bool visit(QtMemberDeclarationAST *ast);
    bool visit(CaseStatementAST *ast);
    bool visit(CompoundStatementAST *ast);
    bool visit(DeclarationStatementAST *ast);
    bool visit(DoStatementAST *ast);
    bool visit(ExpressionStatementAST *ast);
    bool visit(ForeachStatementAST *ast);
    bool visit(ForStatementAST *ast);
    bool visit(IfStatementAST *ast);
    bool visit(LabeledStatementAST *ast);
    bool visit(BreakStatementAST *ast);
    bool visit(ContinueStatementAST *ast);
    bool visit(GotoStatementAST *ast);
    bool visit(ReturnStatementAST *ast);
    bool visit(SwitchStatementAST *ast);
    bool visit(TryBlockStatementAST *ast);
    bool visit(CatchClauseAST *ast);
    bool visit(WhileStatementAST *ast);
    bool visit(ObjCFastEnumerationAST *ast);
    bool visit(ObjCSynchronizedStatementAST *ast);

private:
    unsigned m_initialLine;

    unsigned m_breakpointLine;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_FINDBREAKPOINT_H
