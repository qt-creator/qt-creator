// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "layoutpreview.h"

#include "../cppeditortr.h"
#include "../cppelementevaluator.h"
#include "../cpprefactoringchanges.h"
#include "../symbolfinder.h"
#include "cppquickfix.h"

#include <cplusplus/TypeOfExpression.h>

#include <utils/lua.h>

#ifdef WITH_TESTS
#include "../cpptoolstestcase.h"
#include "../cppeditorwidget.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/kitmanager.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <QtTest>

using namespace ProjectExplorer;
using namespace TextEditor;

#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {

class TopLevelFinder final : protected ASTVisitor
{
public:
    TopLevelFinder(const CppQuickFixInterface &interface)
        : ASTVisitor(interface.context().thisDocument()->translationUnit())
    {}

    void run(AST *ast)
    {
        accept(ast);
    }

    // The  Row { "x" }  case.
    bool visit(TypenameCallExpressionAST *ast) final
    {
        name = ast->name;
        accept(ast->expression);
        return false;
    }

    // The  Row row { "x" }  case.
    bool visit(SimpleDeclarationAST *ast) final
    {
        for (DeclaratorListAST *iter = ast->declarator_list; iter; iter = iter->next)
            accept(iter->value);
        return false;
    }

    bool visit(NamedTypeSpecifierAST *ast) final
    {
        name = ast->name; // Record the type's name
        return false;
    }

    bool visit(DeclaratorAST *ast) final
    {
        accept(ast->initializer);
        return false;
    }

    bool visit(BracedInitializerAST *ast) final
    {
        found = ast;
        return false;
    }

    bool preVisit(AST *ast) final { Q_UNUSED(ast); return !found; }
    void postVisit(AST *) final {}

    NameAST *name = nullptr;
    BracedInitializerAST *found = nullptr;
};

class LuaConverter final : protected ASTVisitor
{
public:
    LuaConverter(const CppQuickFixOperation &op, TranslationUnit *tu)
        : ASTVisitor(tu), iface(op)
    {
        typeOfExpression.init(iface.semanticInfo().doc, iface.snapshot(), iface.context().bindings());
    }

    void run(AST *ast)
    {
        accept(ast);
    }

    void terminal(int token, AST *ast)
    {
        Q_UNUSED(ast)
        out << " TOK: " << token;
    }

    void cr()
    {
        // out << '\n' << QString(4 * indent, QChar(' '));
    }

    QString source(AST *ast) const
    {
        const Token &firstToken = tokenAt(ast->firstToken());
        const Token &lastToken = tokenAt(ast->lastToken());
        const char *src = translationUnit()->firstSourceChar();
        const int first = firstToken.utf16charsBegin();
        const int last = lastToken.utf16charsEnd();
        return  QString::fromUtf8(src + first, last - first);
    }

    QStringList prolog;
    QString result;
    QTextStream out{&result};
    int indent = 0;

    TypeOfExpression typeOfExpression;
    const CppQuickFixOperation &iface;

protected:
    bool preVisit(AST *) { return true; }
    void postVisit(AST *) {}

    bool visit(CompoundStatementAST *ast)
    {
        if (ast->lbrace_token)
            terminal(ast->lbrace_token, ast);
        for (StatementListAST *iter = ast->statement_list; iter; iter = iter->next)
            accept(iter->value);
        if (ast->rbrace_token)
            terminal(ast->rbrace_token, ast);
        return false;
    }

    bool visit(QualifiedNameAST *ast)
    {
        out << "[[QUALIFIED_NAME:" << source(ast) << "]]";
        // // ping()
        // if (ast->global_scope_token)
        //     terminal(ast->global_scope_token, ast);
        // for (NestedNameSpecifierListAST *iter = ast->nested_name_specifier_list; iter; iter = iter->next)
        //     accept(iter->value);
        // accept(ast->unqualified_name);
        return false;
    }

    bool visit(SimpleNameAST *ast)
    {
        //const CppRefactoringFilePtr file = interface.currentFile();
        // QString s = file->textOf(ast);
        // out << " NAME: " << s;
        if (const Identifier *id = identifier(ast->identifier_token)) {
            const QByteArray ba(id->chars(), id->size());
            const QString name = QString::fromUtf8(ba);
            Scope *scope = iface.currentFile()->scopeAt(ast->firstToken());
            const QList<LookupItem> resolvedSymbols =
                    typeOfExpression.reference(ba, scope, TypeOfExpression::Preprocess);
            if (!resolvedSymbols.isEmpty()) {
                const LookupItem resolved = resolvedSymbols.front();
                Symbol *s = resolved.declaration();
                if (s->asDeclaration()) {
                    if (name == "br" || name == "st" || name == "hr") {
                        ;
                    } else {
                        prolog.append(QString("local %1 = [[VAR:%1]];").arg(name));
                    }
                }
            }
            out << name;
        } else {
            out << " UNNAMED ";
        }
        return false;
    }

    static bool isSkippableName(QByteArrayView ba)
    {
        return ba == "size" || ba == "bindTo" || ba == "id" || ba.startsWith("on");
    }

    bool visit(CallAST *ast)
    {
        QByteArrayView calleeName;
        if (IdExpressionAST *idExpr = ast->base_expression->asIdExpression()) {
            if (const SimpleNameAST *simpleName = idExpr->name->asSimpleName()) {
                const Identifier *ident = identifier(simpleName->identifier_token);
                calleeName = {ident->chars(), ident->size()};
            }
        }

        if (!calleeName.isEmpty() && calleeName[0] >= 'A' && calleeName[0] <= 'Z') {
            // Type constructor call (e.g. QString("foo")) -- emit the arguments directly,
            // without a "Name = " prefix, so they land in the Lua table's array part.
            for (ExpressionListAST *iter = ast->expression_list; iter; iter = iter->next) {
                if (iter != ast->expression_list)
                    out << ", ";
                accept(iter->value);
            }
            return false;
        }

        accept(ast->base_expression);
        out << " = ";

        if (!calleeName.isEmpty() && isSkippableName(calleeName)) {
            out << " [[SKIP:" << source(ast) << "]]";
            return false;
        }

        for (ExpressionListAST *iter = ast->expression_list; iter; iter = iter->next) {
            if (iter != ast->expression_list)
                out << ", ";
            accept(iter->value);
        }
        return false;
    }

    bool visit(TypenameCallExpressionAST *ast)
    {
        //ping();
        //out << "TYPENAME CALL";
        if (ast->typename_token)
            terminal(ast->typename_token, ast);
        accept(ast->name);
        accept(ast->expression);
        return false;
    }

    bool visit(NumericLiteralAST *ast)
    {
        if (ast->literal_token) {
            const NumericLiteral *literal = numericLiteral(ast->literal_token);
            out << QString::fromUtf8(literal->chars(), literal->size());
            // switch (ast->literal_token) {
            // case T_NUMERIC_LITERAL: out << tokenAt(ast->literal_token).number; break;
            // case T_WIDE_CHAR_LITERAL: out << "L" << tokenAt(ast->literal_token).number; break;
            // case T_UTF16_CHAR_LITERAL: out << "u" << tokenAt(ast->literal_token).number; break;
            // case T_UTF32_CHAR_LITERAL: out << "U" << tokenAt(ast->literal_token).number; break;
            // default: out << "NUMBER";
            // }
        }
        return false;
    }

    bool visit(StringLiteralAST *ast)
    {
        //ping();
        if (ast->literal_token) {
            // const Token &tk = tokenAt(ast->literal_token);
            // int intId;
            // switch (tk.kind()) {
            // case T_WIDE_STRING_LITERAL:
            //     intId = IntegerType::WideChar;
            //     break;
            // case T_UTF16_STRING_LITERAL:
            //     intId = IntegerType::Char16;
            //     break;
            // case T_UTF32_STRING_LITERAL:
            //     intId = IntegerType::Char32;
            //     break;
            // default:
            //     intId = IntegerType::Char;
            //     break;
            // }
            const StringLiteral *lit = translationUnit()->stringLiteral(ast->literal_token);
            out << '"' <<  QString::fromUtf8(lit->chars(), lit->size()) << '"';
        }
        accept(ast->next);
        return false;
    }

    bool visit(UnaryExpressionAST *ast)
    {
        if (ast->unary_op_token) {
            unsigned unaryOp = tokenKind(ast->unary_op_token);
            //out << literal->chars();
            switch (unaryOp) {
            case T_PLUS_PLUS: out << "++"; break;
            case T_MINUS_MINUS: out << "--"; break;
            case T_STAR: out << "*"; break;
            case T_AMPER: out << "&"; break;
            case T_PLUS: out << "+"; break;
            case T_MINUS: out << "-"; break;
            case T_EXCLAIM:  out << "!"; break;
            default: out << "UNARY";
            }
        }
        accept(ast->expression);
        return false;
    }

    bool visit(LambdaExpressionAST *ast)
    {
        out << " [[LAMDBA:" << source(ast) << "]]";
        return false;
    }

    bool visit(LambdaIntroducerAST *)
    {
        return false;
    }

    bool visit(LambdaCaptureAST *)
    {
        return false;
    }

    bool visit(CaptureAST *)
    {
        return false;
    }

    bool visit(LambdaDeclaratorAST *)
    {
        return false;
    }

    bool visit(BracedInitializerAST *ast)
    {
        out << " {";
        ++indent;
        cr();
        for (ExpressionListAST *iter = ast->expression_list; iter; iter = iter->next) {
            if (iter != ast->expression_list) {
                out << ",";
                cr();
            }
            accept(iter->value);
        }
        --indent;
        cr();
        out << "}";
        return false;
    }
};

class LayoutPreviewOperation final : public CppQuickFixOperation
{
public:
    LayoutPreviewOperation(const CppQuickFixInterface &interface, AST *ast, const QString &name)
        : CppQuickFixOperation(interface), m_ast(ast), m_name(name)
    {
        setDescription(Tr::tr("Preview %1").arg(name));
    }

    void perform() final
    {
        TranslationUnit *tu = context().thisDocument()->translationUnit();
        LuaConverter converter(*this, tu);
        converter.run(m_ast);

        const QString prolog = converter.prolog.join(";\n") + ";\n";
#ifdef WITH_TESTS
        m_testResult = converter.result;
#endif

        const QString script = QString(R"(
            local Gui = require('Gui')

            local function using(tbl)
                local result = _G
                for k, v in pairs(tbl) do result[k] = v end
                return result
            end

            --- "using namespace Gui"
            local _ENV = using(Gui)

            %1
            Preview { %2 %3 } :show()
        )").arg(prolog, m_name, converter.result);

        auto res = Utils::runScript(script, "RunScriptTest");
    }

#ifdef WITH_TESTS
    QString testResult() const { return m_testResult; }
#endif

private:
    AST *m_ast;
    QString m_name;

#ifdef WITH_TESTS
    QString m_testResult;
#endif
};

class LayoutPreview : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) final
    {
        const QList<AST *> &path = interface.path();
        const int n = path.size();
        if (n < 3)
            return;

        TopLevelFinder finder(interface);
        finder.run(path.at(n - 2));

        if (!finder.found) {
            finder.run(path.at(n - 3));
            if (!finder.found)
                return;
        }

        QTC_ASSERT(finder.name, return);

        QTC_ASSERT(interface.currentFile(), return);
        Scope *scope = interface.currentFile()->scopeAt(finder.found->firstToken());
        QTC_ASSERT(scope, return);

        const ClassOrNamespace *klass = interface.context().lookupType(finder.name->name, scope);
        QTC_ASSERT(klass, return);
        QTC_ASSERT(klass->rootClass(), return);
        const Identifier *klassId = klass->rootClass()->identifier();
        const QByteArrayView kname{klassId->chars(), klassId->size()};
        while (klass) {
            if (Class *root = klass->rootClass()) {
                const Identifier *id = root->identifier();
                QTC_ASSERT(id, return);
                const QByteArrayView name{id->chars(), id->size()};
                if (name == "Object") {
                    const QString className = QString::fromUtf8(klassId->chars(), klassId->size());
                    result << new LayoutPreviewOperation(interface, finder.found, className);
                    return;
                }
            }

            const QList<ClassOrNamespace *> usings = klass->usings();
            if (usings.isEmpty())
                break;
            klass = usings.first();
        }
    }
};

#ifdef WITH_TESTS

const char layoutBuilder[] = R"(
#include <functional>
#include <initializer_list>

class QBoxLayout;
class QFormLayout;
class QGridLayout;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QLayout;
class QObject;
class QPushButton;
class QSpinBox;
class QSplitter;
class QStackedWidget;
class QString;
class QTabWidget;
class QTextEdit;
class QToolBar;
class QVBoxLayout;
class QWidget;

namespace Building {

class NestId {};

template <typename Id, typename Arg>
class IdAndArg
{
public:
    IdAndArg(Id, const Arg &arg) : arg(arg) {}
    const Arg arg; // FIXME: Could be const &, but this would currently break bindTo().
};

// The main dispatcher

void doit(auto x, auto id, auto p);

template <typename X> class BuilderItem
{
public:
    // Property setter
    template <typename Id, typename Arg>
    BuilderItem(IdAndArg<Id, Arg> && idarg)
        : apply([&idarg](X *x) { doit(x, Id{}, idarg.arg); })
    {}

    // Nested child object
    template <typename Inner>
    BuilderItem(Inner && p)
        : apply([&p](X *x) { doit(x, NestId{}, std::forward<Inner>(p)); })
    {}

    const std::function<void(X *)> apply;
};

// See Utils::Building::setter in utils/builderutils.h.
template <typename SetterFunction>
class Setter
{
public:
    template <typename ...Args>
    auto operator()(Args &&...args) const
    {
        return IdAndArg{*this, std::tuple<Args...>{std::forward<Args>(args)...}};
    }
};

template <typename SetterFunction, typename L, typename ...Args, std::size_t ...I>
inline void applySetter(L *x, const std::tuple<Args...> &arg, std::index_sequence<I...>)
{
    SetterFunction{}(*x, std::get<I>(arg)...);
}

template <typename L, typename SetterFunction, typename ...Args>
inline void doit(L *x, Setter<SetterFunction>, const std::tuple<Args...> &arg)
{
    applySetter<SetterFunction>(x, arg, std::make_index_sequence<sizeof...(Args)>{});
}

template <typename SetterFunction>
constexpr Setter<SetterFunction> setter(SetterFunction)
{
    return {};
}

} // Building


namespace Layouting {

class Thing
{
public:
    void *ptr; // The product.
};

class Object : public Thing
{
public:
    using Implementation = QObject;
    using I = Building::BuilderItem<Object>;

    Object() = default;
    Object(std::initializer_list<I> ps);
};

class FlowLayout;
class Layout;
using LayoutModifier = std::function<void(Layout *)>;

class Layout : public Object
{
public:
    using Implementation = QLayout;
    using I = Building::BuilderItem<Layout>;

    Layout() = default;
    Layout(Implementation *w) { ptr = w; }

    void flush();
    void flush_() const;

    void show() const;

    // Grid-only
    int currentGridColumn = 0;
    int currentGridRow = 0;
    //Qt::Alignment align = {};
    bool useFormAlignment = false;
};

class Column : public Layout
{
public:
    using Implementation = QVBoxLayout;
    using I = Building::BuilderItem<Column>;

    Column(std::initializer_list<I> ps);
};

class Row : public Layout
{
public:
    using Implementation = QHBoxLayout;
    using I = Building::BuilderItem<Row>;

    Row(std::initializer_list<I> ps);
};

class Form : public Layout
{
public:
    using Implementation = QFormLayout;
    using I = Building::BuilderItem<Form>;

    Form();
    Form(std::initializer_list<I> ps);
};

class Grid : public Layout
{
public:
    using Implementation = QGridLayout;
    using I = Building::BuilderItem<Grid>;

    Grid();
    Grid(std::initializer_list<I> ps);
};

class Flow : public Layout
{
public:
    Flow(std::initializer_list<I> ps);
};

class Stretch
{
public:
    explicit Stretch(int stretch) : stretch(stretch) {}

    int stretch;
};

class Space
{
public:
    explicit Space(int space) : space(space) {}

    int space;
};

class Span
{
public:
    Span(int cols, const Layout::I &item);
    Span(int cols, int rows, const Layout::I &item);

    Layout::I item;
    int spanCols = 1;
    int spanRows = 1;
};

//
// Widgets
//

class Widget : public Object
{
public:
    using Implementation = QWidget;
    using I = Building::BuilderItem<Widget>;

    Widget() = default;
    Widget(std::initializer_list<I> ps);
    Widget(Implementation *w) { ptr = w; }

    void *emerge() const;
    void show();

    void setLayout(const Layout &layout);
    void setSize(int, int);
    void setWindowTitle(const QString &);
    void setToolTip(const QString &);
    void setNoMargins(int = 0);
    void setNormalMargins(int = 0);
    void setContentsMargins(int left, int top, int right, int bottom);
};

class Label : public Widget
{
public:
    using Implementation = QLabel;
    using I = Building::BuilderItem<Label>;

    Label(std::initializer_list<I> ps);
    Label(const QString &text);

    void setText(const QString &);
    void setTextFormat(Qt::TextFormat);
    void setWordWrap(bool);
    void setTextInteractionFlags(Qt::TextInteractionFlags);
    void setOpenExternalLinks(bool);
    void onLinkHovered(const std::function<void(const QString &)> &, QObject *guard);
};

class Group : public Widget
{
public:
    using Implementation = QGroupBox;
    using I = Building::BuilderItem<Group>;

    Group(std::initializer_list<I> ps);

    void setTitle(const QString &);
    void setGroupChecker(const std::function<void(QObject *)> &);
};

class SpinBox : public Widget
{
public:
    using Implementation = QSpinBox;
    using I = Building::BuilderItem<SpinBox>;

    SpinBox(std::initializer_list<I> ps);

    void setValue(int);
    void onTextChanged(const std::function<void(QString)> &);
};

class PushButton : public Widget
{
public:
    using Implementation = QPushButton;
    using I = Building::BuilderItem<PushButton>;

    PushButton(std::initializer_list<I> ps);

    void setText(const QString &);
    void onClicked(const std::function<void()> &, QObject *guard);
};

class TextEdit : public Widget
{
public:
    using Implementation = QTextEdit;
    using I = Building::BuilderItem<TextEdit>;
    using Id = Implementation *;

    TextEdit(std::initializer_list<I> ps);

    void setText(const QString &);
};

class Splitter : public Widget
{
public:
    using Implementation = QSplitter;
    using I = Building::BuilderItem<Splitter>;

    Splitter(std::initializer_list<I> items);
    void setOrientation(Qt::Orientation);
    void setStretchFactor(int index, int stretch);
};

class Stack : public Widget
{
public:
    using Implementation = QStackedWidget;
    using I = Building::BuilderItem<Stack>;

    Stack() : Stack({}) {}
    Stack(std::initializer_list<I> items);
};

class Tab : public Widget
{
public:
    using Implementation = QWidget;

    Tab(const QString &tabName, const Layout &inner);

    const QString tabName;
    const Layout inner;
};

class TabWidget : public Widget
{
public:
    using Implementation = QTabWidget;
    using I = Building::BuilderItem<TabWidget>;

    TabWidget(std::initializer_list<I> items);
};

class ToolBar : public Widget
{
public:
    using Implementation = QToolBar;
    using I = Building::BuilderItem<ToolBar>;

    ToolBar(std::initializer_list<I> items);
};

// Special

class Preview : public Column
{
public:
    using Implementation = QVBoxLayout;
    using I = Building::BuilderItem<Column>;

    Preview(std::initializer_list<I> ps);

    void show() const;
};

class If
{
public:
    If(bool condition,
       const std::initializer_list<Layout::I> ifcase,
       const std::initializer_list<Layout::I> thencase = {});

    const std::initializer_list<Layout::I> used;
};

//
// Dispatchers
//

// We need one 'Id' (and a corresponding function wrapping arguments into a
// tuple marked by this id) per 'name' of "backend" setter member function,
// i.e. one 'text' is sufficient for QLabel::setText, QLineEdit::setText.
// The name of the Id does not have to match the backend names as it
// is mapped per-backend-type in the respective setter implementation
// but we assume that it generally makes sense to stay close to the
// wrapped API name-wise.

// These are free functions overloaded on the type of builder object
// and setter id. The function implementations are independent, but
// the base expectation is that they will forwards to the backend
// type's setter.

// Special dispatchers


class BindToId {};

template <typename T>
auto bindTo(T **p)
{
    return Building::IdAndArg{BindToId{}, p};
}

template <typename Interface>
void doit(Interface *x, BindToId, auto p)
{
    *p = static_cast<typename Interface::Implementation *>(x->ptr);
}

class IdId {};
auto id(auto p) { return Building::IdAndArg{IdId{}, p}; }

template <typename Interface>
void doit(Interface *x, IdId, auto p)
{
    **p = static_cast<typename Interface::Implementation *>(x->ptr);
}

// Setter dispatchers

using Building::setter;

inline constexpr auto fieldGrowthPolicy
    = setter([](auto &x, auto &&...a) { x.setFieldGrowthPolicy(a...); });
inline constexpr auto groupChecker = setter([](auto &x, auto &&...a) { x.setGroupChecker(a...); });
inline constexpr auto openExternalLinks
    = setter([](auto &x, auto &&...a) { x.setOpenExternalLinks(a...); });
inline constexpr auto size = setter([](auto &x, auto &&...a) { x.setSize(a...); });
inline constexpr auto text = setter([](auto &x, auto &&...a) { x.setText(a...); });
inline constexpr auto textFormat = setter([](auto &x, auto &&...a) { x.setTextFormat(a...); });
inline constexpr auto textInteractionFlags
    = setter([](auto &x, auto &&...a) { x.setTextInteractionFlags(a...); });
inline constexpr auto title = setter([](auto &x, auto &&...a) { x.setTitle(a...); });
inline constexpr auto toolTip = setter([](auto &x, auto &&...a) { x.setToolTip(a...); });
inline constexpr auto windowTitle = setter([](auto &x, auto &&...a) { x.setWindowTitle(a...); });
inline constexpr auto wordWrap = setter([](auto &x, auto &&...a) { x.setWordWrap(a...); });
inline constexpr auto orientation = setter([](auto &x, auto &&...a) { x.setOrientation(a...); });
inline constexpr auto columnStretch
    = setter([](auto &x, auto &&...a) { x.setColumnStretch(a...); });
inline constexpr auto onClicked = setter([](auto &x, auto &&...a) { x.onClicked(a...); });
inline constexpr auto onLinkHovered = setter([](auto &x, auto &&...a) { x.onLinkHovered(a...); });
inline constexpr auto onTextChanged = setter([](auto &x, auto &&...a) { x.onTextChanged(a...); });
inline constexpr auto customMargins
    = setter([](auto &x, auto &&...a) { x.setContentsMargins(a...); });

// Nesting dispatchers

void addToLayout(Layout *layout, const Layout &inner);
void addToLayout(Layout *layout, const Widget &inner);
void addToLayout(Layout *layout, QWidget *inner);
void addToLayout(Layout *layout, QLayout *inner);
void addToLayout(Layout *layout, const LayoutModifier &inner);
void addToLayout(Layout *layout, const QString &inner);
void addToLayout(Layout *layout, const Space &inner);
void addToLayout(Layout *layout, const Stretch &inner);
void addToLayout(Layout *layout, const If &inner);
void addToLayout(Layout *layout, const Span &inner);
// ... can be added to anywhere later to support "user types"

void addToWidget(Widget *widget, const Layout &layout);

void addToTabWidget(TabWidget *tabWidget, const Tab &inner);

void addToSplitter(Splitter *splitter, QWidget *inner);
void addToSplitter(Splitter *splitter, const Widget &inner);
void addToSplitter(Splitter *splitter, const Layout &inner);

void addToStack(Stack *stack, QWidget *inner);
void addToStack(Stack *stack, const Widget &inner);
void addToStack(Stack *stack, const Layout &inner);

template <class Inner>
void doit_nested(Layout *outer, Inner && inner)
{
    addToLayout(outer, std::forward<Inner>(inner));
}

void doit_nested(Widget *outer, auto inner)
{
    addToWidget(outer, inner);
}

void doit_nested(TabWidget *outer, auto inner)
{
    addToTabWidget(outer, inner);
}

void doit_nested(Stack *outer, auto inner)
{
    addToStack(outer, inner);
}

void doit_nested(Splitter *outer, auto inner)
{
    addToSplitter(outer, inner);
}

template <class Inner>
void doit(auto outer, Building::NestId, Inner && inner)
{
    doit_nested(outer, std::forward<Inner>(inner));
}

// Special layout items

void empty(Layout *);
void br(Layout *);
void st(Layout *);
void noMargin(Layout *);
void normalMargin(Layout *);
void withFormAlignment(Layout *);
void hr(Layout *);

LayoutModifier spacing(int space);

} // Layouting
)";

class LayoutPreviewTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");
        QTest::addColumn<bool>("applicable");

        // Check: Add local variable for a free function.
        QTest::newRow("Text")
            << layoutBuilder + QByteArray(
                   "void foo()\n"
                   "{\n"
                   "    using namespace Layouting;\n"
                   "    Te|xtEdit {\n"
                   "         id(&textId),\n"
                   "         text(\"World\")\n"
                   "    };\n"
                   "}")
            << QByteArray(
                   " {id =  [[SKIP:id(&textId),]],text = \"World\"}")
            << true;
    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);
        QFETCH(bool, applicable);

        // Tests::TestDocumentPtr tdoc = CppTestDocument::create("file.cpp", original, '|');

        // Write files to disk
        CppEditor::Tests::TemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        CppEditor::Internal::Tests::CppTestDocument doc("file.cpp", original, '|');
        QVERIFY(doc.hasCursorMarker());
        doc.m_source.remove(doc.m_cursorPosition, 1);
        doc.setBaseDirectory(temporaryDir.path());
        QVERIFY(doc.writeToDisk());

        // Update Code Model
        QVERIFY(CppEditor::Tests::TestCase::parseFiles({doc.filePath()}));

        // Open Editor
        CppEditorWidget *editorWidget = nullptr;
        TextEditor::BaseTextEditor *editor = nullptr;

        QVERIFY(CppEditor::Tests::TestCase::openCppEditor(doc.filePath(), &editor,
                                        &editorWidget));
        QVERIFY(editorWidget);
        editor->setCursorPosition(doc.m_cursorPosition);
        QVERIFY(CppEditor::Tests::TestCase::waitForRehighlightedSemanticDocument(editorWidget));

        // Check syntax.
        CppQuickFixInterface quickFixInterface(editorWidget, ExplicitlyInvoked);
        const auto diagnostics = quickFixInterface.semanticInfo().doc->diagnosticMessages();
        QVERIFY(diagnostics.isEmpty());

        // Query factory.
        LayoutPreview factory;
        QuickFixOperations operations;
        factory.match(quickFixInterface, operations);

        if (!applicable) {
            QVERIFY(operations.isEmpty());
            return;
        }

        QVERIFY(!operations.isEmpty());

        auto op = operations.first().dynamicCast<LayoutPreviewOperation>();
        op->perform();

        QCOMPARE(op->testResult().toUtf8(), expected);
    }

    void cleanup()
    {
        QVERIFY(Core::EditorManager::closeAllEditors(false));
        QVERIFY(::CppEditor::Tests::TestCase::garbageCollectGlobalSnapshot());
    }
};

QObject *LayoutPreview::createTest() { return new LayoutPreviewTest; }

#endif // WITH_TESTS

void registerLayoutPreviewQuickfix()
{
    CppQuickFixFactory::registerFactory<LayoutPreview>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <layoutpreview.moc>
#endif
