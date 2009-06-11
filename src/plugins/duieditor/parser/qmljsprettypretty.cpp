/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the QtScript module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmljsprettypretty_p.h"



#include "qmljsengine_p.h"




#include "qmljsast_p.h"

#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QtDebug>

QT_BEGIN_NAMESPACE

namespace QmlJS {
QString numberToString(double value);
}

using namespace QmlJS;

PrettyPretty::PrettyPretty(QTextStream &o):
    out(o), m_indentLevel(0)
{
}

PrettyPretty::~PrettyPretty()
{
}

void PrettyPretty::acceptAsBlock(AST::Node *node)
{
    out << "{";
    pushIndentLevel();
    newlineAndIndent();
    accept(node);
    popIndentLevel();
    newlineAndIndent();
    out << "}";
}

int PrettyPretty::operatorPrecedenceLevel(int op)
{
    switch (op) {
    case QSOperator::Div:
    case QSOperator::Mod:
    case QSOperator::Mul:
        return 5;
    case QSOperator::Add:
    case QSOperator::Sub:
        return 6;
    case QSOperator::LShift:
    case QSOperator::RShift:
    case QSOperator::URShift:
        return 7;
    case QSOperator::Ge:
    case QSOperator::Gt:
    case QSOperator::In:
    case QSOperator::InstanceOf:
    case QSOperator::Le:
    case QSOperator::Lt:
        return 8;
    case QSOperator::Equal:
    case QSOperator::NotEqual:
    case QSOperator::StrictEqual:
    case QSOperator::StrictNotEqual:
        return 9;
    case QSOperator::BitAnd:
        return 10;
    case QSOperator::BitXor:
        return 11;
    case QSOperator::BitOr:
        return 12;
    case QSOperator::And:
        return 13;
    case QSOperator::Or:
        return 14;
    case QSOperator::InplaceAnd:
    case QSOperator::InplaceSub:
    case QSOperator::InplaceDiv:
    case QSOperator::InplaceAdd:
    case QSOperator::InplaceLeftShift:
    case QSOperator::InplaceMod:
    case QSOperator::InplaceMul:
    case QSOperator::InplaceOr:
    case QSOperator::InplaceRightShift:
    case QSOperator::InplaceURightShift:
    case QSOperator::InplaceXor:
    case QSOperator::Assign:
        return 16;
    default:
        Q_ASSERT_X(false, "PrettyPretty::operatorPrecedenceLevel()", "bad operator");
    }
    return 0;
}

int PrettyPretty::compareOperatorPrecedence(int op1, int op2)
{
    int prec1 = operatorPrecedenceLevel(op1);
    int prec2 = operatorPrecedenceLevel(op2);
    if (prec1 == prec2)
        return 0;
    if (prec1 > prec2)
        return -1;
    return 1;
}

QTextStream &PrettyPretty::operator () (AST::Node *node, int level)
{
    int was = indentLevel(level);
    accept(node);
    indentLevel(was);
    return out;
}

QTextStream &PrettyPretty::newlineAndIndent()
{
    enum { IND = 4 };
    out << endl << QString().fill(QLatin1Char(' '), m_indentLevel * IND);
    return out;
}

void PrettyPretty::accept(AST::Node *node)
{
    AST::Node::acceptChild(node, this);
}

bool PrettyPretty::visit(AST::ThisExpression *node)
{
    Q_UNUSED(node);
    out << "this";
    return true;
}

void PrettyPretty::endVisit(AST::ThisExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::IdentifierExpression *node)
{
    out << Engine::toString(node->name);
    return true;
}

void PrettyPretty::endVisit(AST::IdentifierExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::NullExpression *node)
{
    Q_UNUSED(node);
    out << "null";
    return false;
}

void PrettyPretty::endVisit(AST::NullExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::TrueLiteral *node)
{
    Q_UNUSED(node);
    out << "true";
    return false;
}

void PrettyPretty::endVisit(AST::TrueLiteral *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::FalseLiteral *node)
{
    Q_UNUSED(node);
    out << "false";
    return false;
}

void PrettyPretty::endVisit(AST::FalseLiteral *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::StringLiteral *node)
{
    QString lit = Engine::toString(node->value);
    lit.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    out << "\"" << lit << "\"";
    return false;
}

void PrettyPretty::endVisit(AST::StringLiteral *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::NumericLiteral *node)
{
    out << QmlJS::numberToString(node->value);
    return true;
}

void PrettyPretty::endVisit(AST::NumericLiteral *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::RegExpLiteral *node)
{
    out << "/" << Engine::toString(node->pattern) << "/";
    if (node->flags)
        out << QmlJS::Ecma::RegExp::flagsToString(node->flags);

    return true;
}

void PrettyPretty::endVisit(AST::RegExpLiteral *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ArrayLiteral *node)
{
    out << "[";
    accept(node->elements);
    accept(node->elision);
    out << "]";
    return false;
}

void PrettyPretty::endVisit(AST::ArrayLiteral *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ObjectLiteral *node)
{
    out << "{";
    if (node->properties) {
        pushIndentLevel();
        AST::PropertyNameAndValueList *prop;
        for (prop = node->properties; prop != 0; prop = prop->next) {
            newlineAndIndent();
            accept(prop);
            if (prop->next)
                out << ",";
        }
        popIndentLevel();
        newlineAndIndent();
    }
    out << "}";
    return false;
}

void PrettyPretty::endVisit(AST::ObjectLiteral *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ElementList *node)
{
    accept(node->elision);
    accept(node->expression);
    for (node = node->next; node != 0; node = node->next) {
        out << ", ";
        accept(node->elision);
        accept(node->expression);
    }
    return false;
}

void PrettyPretty::endVisit(AST::ElementList *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::Elision *node)
{
    out << ", ";
    for (AST::Elision *eit = node->next; eit != 0; eit = eit->next)
        out << ", ";
    return false;
}

void PrettyPretty::endVisit(AST::Elision *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::PropertyNameAndValueList *node)
{
    accept(node->name);
    out << ": ";
    accept(node->value);
    return false;
}

void PrettyPretty::endVisit(AST::PropertyNameAndValueList *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::IdentifierPropertyName *node)
{
    out << Engine::toString(node->id);
    return false;
}

void PrettyPretty::endVisit(AST::IdentifierPropertyName *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::StringLiteralPropertyName *node)
{
    QString lit = Engine::toString(node->id);
    lit.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    out << lit;
    return false;
}

void PrettyPretty::endVisit(AST::StringLiteralPropertyName *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::NumericLiteralPropertyName *node)
{
    out << node->id;
    return false;
}

void PrettyPretty::endVisit(AST::NumericLiteralPropertyName *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ArrayMemberExpression *node)
{
    accept(node->base);
    out << "[";
    accept(node->expression);
    out << "]";
    return false;
}

void PrettyPretty::endVisit(AST::ArrayMemberExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::FieldMemberExpression *node)
{
    accept(node->base);
    out << "." << Engine::toString(node->name);
    return false;
}

void PrettyPretty::endVisit(AST::FieldMemberExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::NewMemberExpression *node)
{
    out << "new ";
    accept(node->base);
    out << "(";
    accept(node->arguments);
    out << ")";
    return false;
}

void PrettyPretty::endVisit(AST::NewMemberExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::NewExpression *node)
{
    Q_UNUSED(node);
    out << "new ";
    return true;
}

void PrettyPretty::endVisit(AST::NewExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::CallExpression *node)
{
    accept(node->base);
    out << "(";
    accept(node->arguments);
    out << ")";
    return false;
}

void PrettyPretty::endVisit(AST::CallExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ArgumentList *node)
{
    accept(node->expression);
    for (node = node->next; node != 0; node = node->next) {
        out << ", ";
        accept(node->expression);
    }
    return false;
}

void PrettyPretty::endVisit(AST::ArgumentList *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::PostIncrementExpression *node)
{
    Q_UNUSED(node);
    return true;
}

void PrettyPretty::endVisit(AST::PostIncrementExpression *node)
{
    Q_UNUSED(node);
    out << "++";
}

bool PrettyPretty::visit(AST::PostDecrementExpression *node)
{
    Q_UNUSED(node);
    return true;
}

void PrettyPretty::endVisit(AST::PostDecrementExpression *node)
{
    Q_UNUSED(node);
    out << "--";
}

bool PrettyPretty::visit(AST::DeleteExpression *node)
{
    Q_UNUSED(node);
    out << "delete ";
    return true;
}

void PrettyPretty::endVisit(AST::DeleteExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::VoidExpression *node)
{
    Q_UNUSED(node);
    out << "void ";
    return true;
}

void PrettyPretty::endVisit(AST::VoidExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::TypeOfExpression *node)
{
    Q_UNUSED(node);
    out << "typeof ";
    return true;
}

void PrettyPretty::endVisit(AST::TypeOfExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::PreIncrementExpression *node)
{
    Q_UNUSED(node);
    out << "++";
    return true;
}

void PrettyPretty::endVisit(AST::PreIncrementExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::PreDecrementExpression *node)
{
    Q_UNUSED(node);
    out << "--";
    return true;
}

void PrettyPretty::endVisit(AST::PreDecrementExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::UnaryPlusExpression *node)
{
    out << "+";
    bool needParens = (node->expression->binaryExpressionCast() != 0);
    if (needParens)
        out << "(";
    accept(node->expression);
    if (needParens)
        out << ")";
    return false;
}

void PrettyPretty::endVisit(AST::UnaryPlusExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::UnaryMinusExpression *node)
{
    out << "-";
    bool needParens = (node->expression->binaryExpressionCast() != 0);
    if (needParens)
        out << "(";
    accept(node->expression);
    if (needParens)
        out << ")";
    return false;
}

void PrettyPretty::endVisit(AST::UnaryMinusExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::TildeExpression *node)
{
    out << "~";
    bool needParens = (node->expression->binaryExpressionCast() != 0);
    if (needParens)
        out << "(";
    accept(node->expression);
    if (needParens)
        out << ")";
    return false;
}

void PrettyPretty::endVisit(AST::TildeExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::NotExpression *node)
{
    out << "!";
    bool needParens = (node->expression->binaryExpressionCast() != 0);
    if (needParens)
        out << "(";
    accept(node->expression);
    if (needParens)
        out << ")";
    return false;
}

void PrettyPretty::endVisit(AST::NotExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::BinaryExpression *node)
{
    bool needParens = node->left->binaryExpressionCast()
                      && (compareOperatorPrecedence(node->left->binaryExpressionCast()->op, node->op) < 0);
    if (needParens)
        out << "(";
    accept(node->left);
    if (needParens)
        out << ")";
    QString s;
    switch (node->op) {
        case QSOperator::Add:
            s = QLatin1String("+"); break;
        case QSOperator::And:
            s = QLatin1String("&&"); break;
        case QSOperator::InplaceAnd:
            s = QLatin1String("&="); break;
        case QSOperator::Assign:
            s = QLatin1String("="); break;
        case QSOperator::BitAnd:
            s = QLatin1String("&"); break;
        case QSOperator::BitOr:
            s = QLatin1String("|"); break;
        case QSOperator::BitXor:
            s = QLatin1String("^"); break;
        case QSOperator::InplaceSub:
            s = QLatin1String("-="); break;
        case QSOperator::Div:
            s = QLatin1String("/"); break;
        case QSOperator::InplaceDiv:
            s = QLatin1String("/="); break;
        case QSOperator::Equal:
            s = QLatin1String("=="); break;
        case QSOperator::Ge:
            s = QLatin1String(">="); break;
        case QSOperator::Gt:
            s = QLatin1String(">"); break;
        case QSOperator::In:
            s = QLatin1String("in"); break;
        case QSOperator::InplaceAdd:
            s = QLatin1String("+="); break;
        case QSOperator::InstanceOf:
            s = QLatin1String("instanceof"); break;
        case QSOperator::Le:
            s = QLatin1String("<="); break;
        case QSOperator::LShift:
            s = QLatin1String("<<"); break;
        case QSOperator::InplaceLeftShift:
            s = QLatin1String("<<="); break;
        case QSOperator::Lt:
            s = QLatin1String("<"); break;
        case QSOperator::Mod:
            s = QLatin1String("%"); break;
        case QSOperator::InplaceMod:
            s = QLatin1String("%="); break;
        case QSOperator::Mul:
            s = QLatin1String("*"); break;
        case QSOperator::InplaceMul:
            s = QLatin1String("*="); break;
        case QSOperator::NotEqual:
            s = QLatin1String("!="); break;
        case QSOperator::Or:
            s = QLatin1String("||"); break;
        case QSOperator::InplaceOr:
            s = QLatin1String("|="); break;
        case QSOperator::RShift:
            s = QLatin1String(">>"); break;
        case QSOperator::InplaceRightShift:
            s = QLatin1String(">>="); break;
        case QSOperator::StrictEqual:
            s = QLatin1String("==="); break;
        case QSOperator::StrictNotEqual:
            s = QLatin1String("!=="); break;
        case QSOperator::Sub:
            s = QLatin1String("-"); break;
        case QSOperator::URShift:
            s = QLatin1String(">>>"); break;
        case QSOperator::InplaceURightShift:
            s = QLatin1String(">>>="); break;
        case QSOperator::InplaceXor:
            s = QLatin1String("^="); break;
        default:
            Q_ASSERT (0);
    }
    out << " " << s << " ";
    needParens = node->right->binaryExpressionCast()
                 && (compareOperatorPrecedence(node->right->binaryExpressionCast()->op, node->op) <= 0);
    if (needParens)
        out << "(";
    accept(node->right);
    if (needParens)
        out << ")";
    return false;
}

void PrettyPretty::endVisit(AST::BinaryExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ConditionalExpression *node)
{
    accept(node->expression);
    out << " ? ";
    accept(node->ok);
    out << " : ";
    accept(node->ko);
    return false;
}

void PrettyPretty::endVisit(AST::ConditionalExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::Expression *node)
{
    accept(node->left);
    out << ", ";
    accept(node->right);
    return false;
}

void PrettyPretty::endVisit(AST::Expression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::Block *node)
{
    Q_UNUSED(node);
    return true;
}

void PrettyPretty::endVisit(AST::Block *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::StatementList *node)
{
    accept(node->statement);
    for (node = node->next; node != 0; node = node->next) {
        newlineAndIndent();
        accept(node->statement);
    }
    return false;
}

void PrettyPretty::endVisit(AST::StatementList *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::VariableDeclarationList *node)
{
    AST::VariableDeclarationList *it = node;

    do {
        it->declaration->accept(this);
        it = it->next;
        if (it)
            out << ", ";
    } while (it);

    return false;
}

void PrettyPretty::endVisit(AST::VariableDeclarationList *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::VariableStatement *node)
{
    out << "var ";
    Q_UNUSED(node);
    return true;
}

void PrettyPretty::endVisit(AST::VariableStatement *node)
{
    Q_UNUSED(node);
    out << ";";
}

bool PrettyPretty::visit(AST::VariableDeclaration *node)
{
    out << Engine::toString(node->name);
    if (node->expression) {
        out << " = ";
        accept(node->expression);
    }
    return false;
}

void PrettyPretty::endVisit(AST::VariableDeclaration *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::EmptyStatement *node)
{
    Q_UNUSED(node);
    out << ";";
    return true;
}

void PrettyPretty::endVisit(AST::EmptyStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ExpressionStatement *node)
{
    accept(node->expression);
    out << ";";
    return false;
}

void PrettyPretty::endVisit(AST::ExpressionStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::IfStatement *node)
{
    out << "if (";
    accept(node->expression);
    out << ") ";
    acceptAsBlock(node->ok);
    if (node->ko) {
        out << " else ";
        acceptAsBlock(node->ko);
    }
    return false;
}

void PrettyPretty::endVisit(AST::IfStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::DoWhileStatement *node)
{
    out << "do ";
    acceptAsBlock(node->statement);
    out << " while (";
    accept(node->expression);
    out << ");";
    return false;
}

void PrettyPretty::endVisit(AST::DoWhileStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::WhileStatement *node)
{
    out << "while (";
    accept(node->expression);
    out << ") ";
    acceptAsBlock(node->statement);
    return false;
}

void PrettyPretty::endVisit(AST::WhileStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ForStatement *node)
{
    out << "for (";
    accept(node->initialiser);
    out << "; ";
    accept(node->condition);
    out << "; ";
    accept(node->expression);
    out << ") ";
    acceptAsBlock(node->statement);
    return false;
}

void PrettyPretty::endVisit(AST::ForStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::LocalForStatement *node)
{
    out << "for (var ";
    accept(node->declarations);
    out << "; ";
    accept(node->condition);
    out << "; ";
    accept(node->expression);
    out << ") ";
    acceptAsBlock(node->statement);
    return false;
}

void PrettyPretty::endVisit(AST::LocalForStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ForEachStatement *node)
{
    out << "for (";
    accept(node->initialiser);
    out << " in ";
    accept(node->expression);
    out << ") ";
    acceptAsBlock(node->statement);
    return false;
}

void PrettyPretty::endVisit(AST::ForEachStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::LocalForEachStatement *node)
{
    out << "for (var ";
    accept(node->declaration);
    out << " in ";
    accept(node->expression);
    out << ") ";
    acceptAsBlock(node->statement);
    return false;
}

void PrettyPretty::endVisit(AST::LocalForEachStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ContinueStatement *node)
{
    out << "continue";
    if (node->label) {
        out << " " << Engine::toString(node->label);
    }
    out << ";";
    return false;
}

void PrettyPretty::endVisit(AST::ContinueStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::BreakStatement *node)
{
    out << "break";
    if (node->label) {
        out << " " << Engine::toString(node->label);
    }
    out << ";";
    return false;
}

void PrettyPretty::endVisit(AST::BreakStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ReturnStatement *node)
{
    out << "return";
    if (node->expression) {
        out << " ";
        accept(node->expression);
    }
    out << ";";
    return false;
}

void PrettyPretty::endVisit(AST::ReturnStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::WithStatement *node)
{
    out << "with (";
    accept(node->expression);
    out << ") ";
    acceptAsBlock(node->statement);
    return false;
}

void PrettyPretty::endVisit(AST::WithStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::SwitchStatement *node)
{
    out << "switch (";
    accept(node->expression);
    out << ") ";
    acceptAsBlock(node->block);
    return false;
}

void PrettyPretty::endVisit(AST::SwitchStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::CaseBlock *node)
{
    accept(node->clauses);
    if (node->defaultClause) {
        newlineAndIndent();
        accept(node->defaultClause);
    }
    if (node->moreClauses) {
        newlineAndIndent();
        accept(node->moreClauses);
    }
    return false;
}

void PrettyPretty::endVisit(AST::CaseBlock *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::CaseClauses *node)
{
    accept(node->clause);
    for (node = node->next; node != 0; node = node->next) {
        newlineAndIndent();
        accept(node->clause);
    }
    return false;
}

void PrettyPretty::endVisit(AST::CaseClauses *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::CaseClause *node)
{
    out << "case ";
    accept(node->expression);
    out << ":";
    if (node->statements) {
        newlineAndIndent();
        accept(node->statements);
    }
    return false;
}

void PrettyPretty::endVisit(AST::CaseClause *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::DefaultClause *node)
{
    Q_UNUSED(node);
    out << "default:";
    newlineAndIndent();
    return true;
}

void PrettyPretty::endVisit(AST::DefaultClause *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::LabelledStatement *node)
{
    out << Engine::toString(node->label) << ": ";
    return true;
}

void PrettyPretty::endVisit(AST::LabelledStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::ThrowStatement *node)
{
    Q_UNUSED(node);
    out << "throw ";
    accept(node->expression);
    out << ";";
    return false;
}

void PrettyPretty::endVisit(AST::ThrowStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::TryStatement *node)
{
    out << "try ";
    acceptAsBlock(node->statement);
    if (node->catchExpression) {
        out << " catch (" << Engine::toString(node->catchExpression->name) << ") ";
        acceptAsBlock(node->catchExpression->statement);
    }
    if (node->finallyExpression) {
        out << " finally ";
        acceptAsBlock(node->finallyExpression->statement);
    }
    return false;
}

void PrettyPretty::endVisit(AST::TryStatement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::Catch *node)
{
    Q_UNUSED(node);
    return true;
}

void PrettyPretty::endVisit(AST::Catch *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::Finally *node)
{
    Q_UNUSED(node);
    out << "finally ";
    return true;
}

void PrettyPretty::endVisit(AST::Finally *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::FunctionDeclaration *node)
{
    out << "function";

    if (node->name)
        out << " " << Engine::toString(node->name);

    // the arguments
    out << "(";
    for (AST::FormalParameterList *it = node->formals; it; it = it->next) {
        if (it->name)
            out << Engine::toString(it->name);

        if (it->next)
            out << ", ";
    }
    out << ")";

    // the function body
    out << " {";

    if (node->body) {
        pushIndentLevel();
        newlineAndIndent();
        accept(node->body);
        popIndentLevel();
        newlineAndIndent();
    }

    out << "}";

    return false;
}

void PrettyPretty::endVisit(AST::FunctionDeclaration *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::FunctionExpression *node)
{
    out << "function";

    if (node->name)
        out << " " << Engine::toString(node->name);

    // the arguments
    out << "(";
    for (AST::FormalParameterList *it = node->formals; it; it = it->next) {
        if (it->name)
            out << Engine::toString(it->name);

        if (it->next)
            out << ", ";
    }
    out << ")";

    // the function body
    out << " {";

    if (node->body) {
        pushIndentLevel();
        newlineAndIndent();
        accept(node->body);
        popIndentLevel();
        newlineAndIndent();
    }

    out << "}";

    return false;
}

void PrettyPretty::endVisit(AST::FunctionExpression *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::FormalParameterList *node)
{
    Q_UNUSED(node);
    return true;
}

void PrettyPretty::endVisit(AST::FormalParameterList *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::FunctionBody *node)
{
    Q_UNUSED(node);
    return true;
}

void PrettyPretty::endVisit(AST::FunctionBody *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::Program *node)
{
    Q_UNUSED(node);
    return true;
}

void PrettyPretty::endVisit(AST::Program *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::SourceElements *node)
{
    Q_UNUSED(node);
    accept(node->element);
    for (node = node->next; node != 0; node = node->next) {
        newlineAndIndent();
        accept(node->element);
    }
    return false;
}

void PrettyPretty::endVisit(AST::SourceElements *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::FunctionSourceElement *node)
{
    Q_UNUSED(node);
    return true;
}

void PrettyPretty::endVisit(AST::FunctionSourceElement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::StatementSourceElement *node)
{
    Q_UNUSED(node);
    return true;
}

void PrettyPretty::endVisit(AST::StatementSourceElement *node)
{
    Q_UNUSED(node);
}

bool PrettyPretty::visit(AST::DebuggerStatement *node)
{
    Q_UNUSED(node);
    out << "debugger";
    return true;
}

void PrettyPretty::endVisit(AST::DebuggerStatement *node)
{
    Q_UNUSED(node);
    out << ";";
}

bool PrettyPretty::preVisit(AST::Node *node)
{
    Q_UNUSED(node);
    return true;
}

QT_END_NAMESPACE


