// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "convertstringliteral.h"

#include "../cppeditordocument.h"
#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

#include <QTextDecoder>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#include <QtTest>
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

enum StringLiteralType { TypeString, TypeObjCString, TypeChar, TypeNone };

enum ActionFlags {
    EncloseInQLatin1CharAction = 0x1,
    EncloseInQLatin1StringAction = 0x2,
    EncloseInQStringLiteralAction = 0x4,
    EncloseInQByteArrayLiteralAction = 0x8,
    EncloseActionMask = EncloseInQLatin1CharAction | EncloseInQLatin1StringAction
                        | EncloseInQStringLiteralAction | EncloseInQByteArrayLiteralAction,
    TranslateTrAction = 0x10,
    TranslateQCoreApplicationAction = 0x20,
    TranslateNoopAction = 0x40,
    TranslationMask = TranslateTrAction | TranslateQCoreApplicationAction | TranslateNoopAction,
    RemoveObjectiveCAction = 0x100,
    ConvertEscapeSequencesToCharAction = 0x200,
    ConvertEscapeSequencesToStringAction = 0x400,
    SingleQuoteAction = 0x800,
    DoubleQuoteAction = 0x1000
};

static bool isQtStringLiteral(const QByteArray &id)
{
    return id == "QLatin1String" || id == "QLatin1Literal" || id == "QStringLiteral"
           || id == "QByteArrayLiteral";
}

static bool isQtStringTranslation(const QByteArray &id)
{
    return id == "tr" || id == "trUtf8" || id == "translate" || id == "QT_TRANSLATE_NOOP";
}

/* Convert single-character string literals into character literals with some
 * special cases "a" --> 'a', "'" --> '\'', "\n" --> '\n', "\"" --> '"'. */
static QByteArray stringToCharEscapeSequences(const QByteArray &content)
{
    if (content.size() == 1)
        return content.at(0) == '\'' ? QByteArray("\\'") : content;
    if (content.size() == 2 && content.at(0) == '\\')
        return content == "\\\"" ? QByteArray(1, '"') : content;
    return QByteArray();
}

/* Convert character literal into a string literal with some special cases
 * 'a' -> "a", '\n' -> "\n", '\'' --> "'", '"' --> "\"". */
static QByteArray charToStringEscapeSequences(const QByteArray &content)
{
    if (content.size() == 1)
        return content.at(0) == '"' ? QByteArray("\\\"") : content;
    if (content.size() == 2)
        return content == "\\'" ? QByteArray("'") : content;
    return QByteArray();
}

static QString msgQtStringLiteralDescription(const QString &replacement)
{
    return Tr::tr("Enclose in %1(...)").arg(replacement);
}

static QString stringLiteralReplacement(unsigned actions)
{
    if (actions & EncloseInQLatin1CharAction)
        return QLatin1String("QLatin1Char");
    if (actions & EncloseInQLatin1StringAction)
        return QLatin1String("QLatin1String");
    if (actions & EncloseInQStringLiteralAction)
        return QLatin1String("QStringLiteral");
    if (actions & EncloseInQByteArrayLiteralAction)
        return QLatin1String("QByteArrayLiteral");
    if (actions & TranslateTrAction)
        return QLatin1String("tr");
    if (actions & TranslateQCoreApplicationAction)
        return QLatin1String("QCoreApplication::translate");
    if (actions & TranslateNoopAction)
        return QLatin1String("QT_TRANSLATE_NOOP");
    return QString();
}

static ExpressionAST *analyzeStringLiteral(const QList<AST *> &path,
                                           const CppRefactoringFilePtr &file, StringLiteralType *type,
                                           QByteArray *enclosingFunction = nullptr,
                                           CallAST **enclosingFunctionCall = nullptr)
{
    *type = TypeNone;
    if (enclosingFunction)
        enclosingFunction->clear();
    if (enclosingFunctionCall)
        *enclosingFunctionCall = nullptr;

    if (path.isEmpty())
        return nullptr;

    ExpressionAST *literal = path.last()->asExpression();
    if (literal) {
        if (literal->asStringLiteral()) {
            // Check for Objective C string (@"bla")
            const QChar firstChar = file->charAt(file->startOf(literal));
            *type = firstChar == QLatin1Char('@') ? TypeObjCString : TypeString;
        } else if (NumericLiteralAST *numericLiteral = literal->asNumericLiteral()) {
            // character ('c') constants are numeric.
            if (file->tokenAt(numericLiteral->literal_token).is(T_CHAR_LITERAL))
                *type = TypeChar;
        }
    }

    if (*type != TypeNone && enclosingFunction && path.size() > 1) {
        if (CallAST *call = path.at(path.size() - 2)->asCall()) {
            if (call->base_expression) {
                if (IdExpressionAST *idExpr = call->base_expression->asIdExpression()) {
                    if (SimpleNameAST *functionName = idExpr->name->asSimpleName()) {
                        *enclosingFunction = file->tokenAt(functionName->identifier_token).identifier->chars();
                        if (enclosingFunctionCall)
                            *enclosingFunctionCall = call;
                    }
                }
            }
        }
    }
    return literal;
}

class EscapeStringLiteralOperation: public CppQuickFixOperation
{
public:
    EscapeStringLiteralOperation(const CppQuickFixInterface &interface,
                                 ExpressionAST *literal, bool escape)
        : CppQuickFixOperation(interface)
        , m_literal(literal)
        , m_escape(escape)
    {
        if (m_escape) {
            setDescription(Tr::tr("Escape String Literal as UTF-8"));
        } else {
            setDescription(Tr::tr("Unescape String Literal as UTF-8"));
        }
    }

private:
    static inline bool isDigit(quint8 ch, int base)
    {
        if (base == 8)
            return ch >= '0' && ch < '8';
        if (base == 16)
            return isxdigit(ch);
        return false;
    }

    static QByteArrayList escapeString(const QByteArray &contents)
    {
        QByteArrayList newContents;
        QByteArray chunk;
        bool wasEscaped = false;
        for (const quint8 c : contents) {
            const bool needsEscape = !isascii(c) || !isprint(c);
            if (!needsEscape && wasEscaped && std::isxdigit(c) && !chunk.isEmpty()) {
                newContents << chunk;
                chunk.clear();
            }
            if (needsEscape)
                chunk += QByteArray("\\x") + QByteArray::number(c, 16).rightJustified(2, '0');
            else
                chunk += c;
            wasEscaped = needsEscape;
        }
        if (!chunk.isEmpty())
            newContents << chunk;
        return newContents;
    }

    static QByteArray unescapeString(const QByteArray &contents)
    {
        QByteArray newContents;
        const int len = contents.length();
        for (int i = 0; i < len; ++i) {
            quint8 c = contents.at(i);
            if (c == '\\' && i < len - 1) {
                int idx = i + 1;
                quint8 ch = contents.at(idx);
                int base = 0;
                int maxlen = 0;
                if (isDigit(ch, 8)) {
                    base = 8;
                    maxlen = 3;
                } else if ((ch == 'x' || ch == 'X') && idx < len - 1) {
                    base = 16;
                    maxlen = 2;
                    ch = contents.at(++idx);
                }
                if (base > 0) {
                    QByteArray buf;
                    while (isDigit(ch, base) && idx < len && buf.length() < maxlen) {
                        buf += ch;
                        ++idx;
                        if (idx == len)
                            break;
                        ch = contents.at(idx);
                    }
                    if (!buf.isEmpty()) {
                        bool ok;
                        uint value = buf.toUInt(&ok, base);
                        // Don't unescape isascii() && !isprint()
                        if (ok && (!isascii(value) || isprint(value))) {
                            newContents += value;
                            i = idx - 1;
                            continue;
                        }
                    }
                }
                newContents += c;
                c = contents.at(++i);
            }
            newContents += c;
        }
        return newContents;
    }

    // QuickFixOperation interface
public:
    void perform() override
    {
        const int startPos = currentFile()->startOf(m_literal);
        const int endPos = currentFile()->endOf(m_literal);

        StringLiteralAST *stringLiteral = m_literal->asStringLiteral();
        QTC_ASSERT(stringLiteral, return);
        const QByteArray oldContents(currentFile()->tokenAt(stringLiteral->literal_token).
                                     identifier->chars());
        QByteArrayList newContents;
        if (m_escape)
            newContents = escapeString(oldContents);
        else
            newContents = {unescapeString(oldContents)};

        if (newContents.isEmpty()
            || (newContents.size() == 1 && newContents.first() == oldContents)) {
            return;
        }

        QTextCodec *utf8codec = QTextCodec::codecForName("UTF-8");
        QScopedPointer<QTextDecoder> decoder(utf8codec->makeDecoder());
        ChangeSet changes;

        bool replace = true;
        for (const QByteArray &chunk : std::as_const(newContents)) {
            const QString str = decoder->toUnicode(chunk);
            const QByteArray utf8buf = str.toUtf8();
            if (!utf8codec->canEncode(str) || chunk != utf8buf)
                return;
            if (replace)
                changes.replace(startPos + 1, endPos - 1, str);
            else
                changes.insert(endPos, "\"" + str + "\"");
            replace = false;
        }
        currentFile()->apply(changes);
    }

private:
    ExpressionAST *m_literal;
    bool m_escape;
};

/// Operation performs the operations of type ActionFlags passed in as actions.
class WrapStringLiteralOp : public CppQuickFixOperation
{
public:
    WrapStringLiteralOp(const CppQuickFixInterface &interface, int priority,
                        unsigned actions, const QString &description, ExpressionAST *literal,
                        const QString &translationContext = QString())
        : CppQuickFixOperation(interface, priority), m_actions(actions), m_literal(literal),
        m_translationContext(translationContext)
    {
        setDescription(description);
    }

    void perform() override
    {
        ChangeSet changes;

        const int startPos = currentFile()->startOf(m_literal);
        const int endPos = currentFile()->endOf(m_literal);

        // kill leading '@'. No need to adapt endPos, that is done by ChangeSet
        if (m_actions & RemoveObjectiveCAction)
            changes.remove(startPos, startPos + 1);

        // Fix quotes
        if (m_actions & (SingleQuoteAction | DoubleQuoteAction)) {
            const QString newQuote((m_actions & SingleQuoteAction)
                                       ? QLatin1Char('\'') : QLatin1Char('"'));
            changes.replace(startPos, startPos + 1, newQuote);
            changes.replace(endPos - 1, endPos, newQuote);
        }

        // Convert single character strings into character constants
        if (m_actions & ConvertEscapeSequencesToCharAction) {
            StringLiteralAST *stringLiteral = m_literal->asStringLiteral();
            QTC_ASSERT(stringLiteral, return ;);
            const QByteArray oldContents(currentFile()->tokenAt(stringLiteral->literal_token).identifier->chars());
            const QByteArray newContents = stringToCharEscapeSequences(oldContents);
            QTC_ASSERT(!newContents.isEmpty(), return ;);
            if (oldContents != newContents)
                changes.replace(startPos + 1, endPos -1, QString::fromLatin1(newContents));
        }

        // Convert character constants into strings constants
        if (m_actions & ConvertEscapeSequencesToStringAction) {
            NumericLiteralAST *charLiteral = m_literal->asNumericLiteral(); // char 'c' constants are numerical.
            QTC_ASSERT(charLiteral, return ;);
            const QByteArray oldContents(currentFile()->tokenAt(charLiteral->literal_token).identifier->chars());
            const QByteArray newContents = charToStringEscapeSequences(oldContents);
            QTC_ASSERT(!newContents.isEmpty(), return ;);
            if (oldContents != newContents)
                changes.replace(startPos + 1, endPos -1, QString::fromLatin1(newContents));
        }

        // Enclose in literal or translation function, macro.
        if (m_actions & (EncloseActionMask | TranslationMask)) {
            changes.insert(endPos, QString(QLatin1Char(')')));
            QString leading = stringLiteralReplacement(m_actions);
            leading += QLatin1Char('(');
            if (m_actions
                & (TranslateQCoreApplicationAction | TranslateNoopAction)) {
                leading += QLatin1Char('"');
                leading += m_translationContext;
                leading += QLatin1String("\", ");
            }
            changes.insert(startPos, leading);
        }

        currentFile()->apply(changes);
    }

private:
    const unsigned m_actions;
    ExpressionAST *m_literal;
    const QString m_translationContext;
};

class ConvertCStringToNSStringOp: public CppQuickFixOperation
{
public:
    ConvertCStringToNSStringOp(const CppQuickFixInterface &interface, int priority,
                               StringLiteralAST *stringLiteral, CallAST *qlatin1Call)
        : CppQuickFixOperation(interface, priority)
        , stringLiteral(stringLiteral)
        , qlatin1Call(qlatin1Call)
    {
        setDescription(Tr::tr("Convert to Objective-C String Literal"));
    }

    void perform() override
    {
        ChangeSet changes;

        if (qlatin1Call) {
            changes.replace(currentFile()->startOf(qlatin1Call), currentFile()->startOf(stringLiteral),
                            QLatin1String("@"));
            changes.remove(currentFile()->endOf(stringLiteral), currentFile()->endOf(qlatin1Call));
        } else {
            changes.insert(currentFile()->startOf(stringLiteral), QLatin1String("@"));
        }

        currentFile()->apply(changes);
    }

private:
    StringLiteralAST *stringLiteral;
    CallAST *qlatin1Call;
};

/*!
  Replace
     "abcd"
     QLatin1String("abcd")
     QLatin1Literal("abcd")

  With
     @"abcd"

  Activates on: the string literal, if the file type is a Objective-C(++) file.
*/
class ConvertCStringToNSString: public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        CppRefactoringFilePtr file = interface.currentFile();

        if (!interface.editor()->cppEditorDocument()->isObjCEnabled())
            return;

        StringLiteralType type = TypeNone;
        QByteArray enclosingFunction;
        CallAST *qlatin1Call;
        const QList<AST *> &path = interface.path();
        ExpressionAST *literal = analyzeStringLiteral(path, file, &type, &enclosingFunction,
                                                      &qlatin1Call);
        if (!literal || type != TypeString)
            return;
        if (!isQtStringLiteral(enclosingFunction))
            qlatin1Call = nullptr;

        result << new ConvertCStringToNSStringOp(interface, path.size() - 1, literal->asStringLiteral(),
                                                 qlatin1Call);
    }
};

/*!
  Replace
    "abcd"

  With
    tr("abcd") or
    QCoreApplication::translate("CONTEXT", "abcd") or
    QT_TRANSLATE_NOOP("GLOBAL", "abcd")

  depending on what is available.

  Activates on: the string literal
*/
class TranslateStringLiteral: public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        // Initialize
        StringLiteralType type = TypeNone;
        QByteArray enclosingFunction;
        const QList<AST *> &path = interface.path();
        CppRefactoringFilePtr file = interface.currentFile();
        ExpressionAST *literal = analyzeStringLiteral(path, file, &type, &enclosingFunction);
        if (!literal || type != TypeString
            || isQtStringLiteral(enclosingFunction) || isQtStringTranslation(enclosingFunction))
            return;

        QString trContext;

        std::shared_ptr<Control> control = interface.context().bindings()->control();
        const Name *trName = control->identifier("tr");

        // Check whether we are in a function:
        const QString description = Tr::tr("Mark as Translatable");
        for (int i = path.size() - 1; i >= 0; --i) {
            if (FunctionDefinitionAST *definition = path.at(i)->asFunctionDefinition()) {
                Function *function = definition->symbol;
                ClassOrNamespace *b = interface.context().lookupType(function);
                if (b) {
                    // Do we have a tr function?
                    const QList<LookupItem> items = b->find(trName);
                    for (const LookupItem &r : items) {
                        Symbol *s = r.declaration();
                        if (s->type()->asFunctionType()) {
                            // no context required for tr
                            result << new WrapStringLiteralOp(interface, path.size() - 1,
                                                              TranslateTrAction,
                                                              description, literal);
                            return;
                        }
                    }
                }
                // We need to do a QCA::translate, so we need a context.
                // Use fully qualified class name:
                Overview oo;
                const QList<const Name *> names = LookupContext::path(function);
                for (const Name *n : names) {
                    if (!trContext.isEmpty())
                        trContext.append(QLatin1String("::"));
                    trContext.append(oo.prettyName(n));
                }
                // ... or global if none available!
                if (trContext.isEmpty())
                    trContext = QLatin1String("GLOBAL");
                result << new WrapStringLiteralOp(interface, path.size() - 1,
                                                  TranslateQCoreApplicationAction,
                                                  description, literal, trContext);
                return;
            }
        }

        // We need to use Q_TRANSLATE_NOOP
        result << new WrapStringLiteralOp(interface, path.size() - 1,
                                          TranslateNoopAction,
                                          description, literal, trContext);
    }
};

/*!
  Replace
    "abcd"  -> QLatin1String("abcd")
    @"abcd" -> QLatin1String("abcd") (Objective C)
    'a'     -> QLatin1Char('a')
    'a'     -> "a"
    "a"     -> 'a' or QLatin1Char('a') (Single character string constants)
    "\n"    -> '\n', QLatin1Char('\n')

  Except if they are already enclosed in
    QLatin1Char, QT_TRANSLATE_NOOP, tr,
    trUtf8, QLatin1Literal, QLatin1String

  Activates on: the string or character literal
*/

class WrapStringLiteral: public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        StringLiteralType type = TypeNone;
        QByteArray enclosingFunction;
        const QList<AST *> &path = interface.path();
        CppRefactoringFilePtr file = interface.currentFile();
        ExpressionAST *literal = analyzeStringLiteral(path, file, &type, &enclosingFunction);
        if (!literal || type == TypeNone)
            return;
        if ((type == TypeChar && enclosingFunction == "QLatin1Char")
            || isQtStringLiteral(enclosingFunction)
            || isQtStringTranslation(enclosingFunction))
            return;

        const int priority = path.size() - 1; // very high priority
        if (type == TypeChar) {
            unsigned actions = EncloseInQLatin1CharAction;
            QString description = msgQtStringLiteralDescription(stringLiteralReplacement(actions));
            result << new WrapStringLiteralOp(interface, priority, actions, description, literal);
            if (NumericLiteralAST *charLiteral = literal->asNumericLiteral()) {
                const QByteArray contents(file->tokenAt(charLiteral->literal_token).identifier->chars());
                if (!charToStringEscapeSequences(contents).isEmpty()) {
                    actions = DoubleQuoteAction | ConvertEscapeSequencesToStringAction;
                    description = Tr::tr("Convert to String Literal");
                    result << new WrapStringLiteralOp(interface, priority, actions,
                                                      description, literal);
                }
            }
        } else {
            const unsigned objectiveCActions = type == TypeObjCString ?
                                                   unsigned(RemoveObjectiveCAction) : 0u;
            unsigned actions = 0;
            if (StringLiteralAST *stringLiteral = literal->asStringLiteral()) {
                const QByteArray contents(file->tokenAt(stringLiteral->literal_token).identifier->chars());
                if (!stringToCharEscapeSequences(contents).isEmpty()) {
                    actions = EncloseInQLatin1CharAction | SingleQuoteAction
                              | ConvertEscapeSequencesToCharAction | objectiveCActions;
                    QString description =
                        Tr::tr("Convert to Character Literal and Enclose in QLatin1Char(...)");
                    result << new WrapStringLiteralOp(interface, priority, actions,
                                                      description, literal);
                    actions &= ~EncloseInQLatin1CharAction;
                    description = Tr::tr("Convert to Character Literal");
                    result << new WrapStringLiteralOp(interface, priority, actions,
                                                      description, literal);
                }
            }
            actions = EncloseInQLatin1StringAction | objectiveCActions;
            result << new WrapStringLiteralOp(interface, priority, actions,
                                              msgQtStringLiteralDescription(stringLiteralReplacement(actions)), literal);
            actions = EncloseInQStringLiteralAction | objectiveCActions;
            result << new WrapStringLiteralOp(interface, priority, actions,
                                              msgQtStringLiteralDescription(stringLiteralReplacement(actions)), literal);
            actions = EncloseInQByteArrayLiteralAction | objectiveCActions;
            result << new WrapStringLiteralOp(interface, priority, actions,
                                              msgQtStringLiteralDescription(stringLiteralReplacement(actions)), literal);
        }
    }
};

/*!
  Escapes or unescapes a string literal as UTF-8.

  Escapes non-ASCII characters in a string literal to hexadecimal escape sequences.
  Unescapes octal or hexadecimal escape sequences in a string literal.
  String literals are handled as UTF-8 even if file's encoding is not UTF-8.
 */
class EscapeStringLiteral : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        if (path.isEmpty())
            return;

        AST * const lastAst = path.last();
        ExpressionAST *literal = lastAst->asStringLiteral();
        if (!literal)
            return;

        StringLiteralAST *stringLiteral = literal->asStringLiteral();
        CppRefactoringFilePtr file = interface.currentFile();
        const QByteArray contents(file->tokenAt(stringLiteral->literal_token).identifier->chars());

        bool canEscape = false;
        bool canUnescape = false;
        for (int i = 0; i < contents.length(); ++i) {
            quint8 c = contents.at(i);
            if (!isascii(c) || !isprint(c)) {
                canEscape = true;
            } else if (c == '\\' && i < contents.length() - 1) {
                c = contents.at(++i);
                if ((c >= '0' && c < '8') || c == 'x' || c == 'X')
                    canUnescape = true;
            }
        }

        if (canEscape)
            result << new EscapeStringLiteralOperation(interface, literal, true);

        if (canUnescape)
            result << new EscapeStringLiteralOperation(interface, literal, false);
    }
};

#ifdef WITH_TESTS
using namespace Tests;

class EscapeStringLiteralTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        // Escape String Literal as UTF-8 (no-trigger)
        QTest::newRow("EscapeStringLiteral_notrigger")
            << QByteArray("const char *notrigger = \"@abcdef \\a\\n\\\\\";\n")
            << QByteArray();

        // Escape String Literal as UTF-8
        QTest::newRow("EscapeStringLiteral")
            << QByteArray("const char *utf8 = \"@\xe3\x81\x82\xe3\x81\x84\";\n")
            << QByteArray("const char *utf8 = \"\\xe3\\x81\\x82\\xe3\\x81\\x84\";\n");

        // Unescape String Literal as UTF-8 (from hexdecimal escape sequences)
        QTest::newRow("UnescapeStringLiteral_hex")
            << QByteArray("const char *hex_escaped = \"@\\xe3\\x81\\x82\\xe3\\x81\\x84\";\n")
            << QByteArray("const char *hex_escaped = \"\xe3\x81\x82\xe3\x81\x84\";\n");

        // Unescape String Literal as UTF-8 (from octal escape sequences)
        QTest::newRow("UnescapeStringLiteral_oct")
            << QByteArray("const char *oct_escaped = \"@\\343\\201\\202\\343\\201\\204\";\n")
            << QByteArray("const char *oct_escaped = \"\xe3\x81\x82\xe3\x81\x84\";\n");

        // Unescape String Literal as UTF-8 (triggered but no change)
        QTest::newRow("UnescapeStringLiteral_noconv")
            << QByteArray("const char *escaped_ascii = \"@\\x1b\";\n")
            << QByteArray("const char *escaped_ascii = \"\\x1b\";\n");

        // Unescape String Literal as UTF-8 (no conversion because of invalid utf-8)
        QTest::newRow("UnescapeStringLiteral_invalid")
            << QByteArray("const char *escaped = \"@\\xe3\\x81\";\n")
            << QByteArray("const char *escaped = \"\\xe3\\x81\";\n");

        QTest::newRow("escape string literal: simple case")
            << QByteArray(R"(const char *str = @"àxyz";)")
            << QByteArray(R"(const char *str = "\xc3\xa0xyz";)");
        QTest::newRow("escape string literal: simple case reverse")
            << QByteArray(R"(const char *str = @"\xc3\xa0xyz";)")
            << QByteArray(R"(const char *str = "àxyz";)");
        QTest::newRow("escape string literal: raw string literal")
            << QByteArray(R"x(const char *str = @R"(àxyz)";)x")
            << QByteArray(R"x(const char *str = R"(\xc3\xa0xyz)";)x");
        QTest::newRow("escape string literal: splitting required")
            << QByteArray(R"(const char *str = @"àf23бgб1";)")
            << QByteArray(R"(const char *str = "\xc3\xa0""f23\xd0\xb1g\xd0\xb1""1";)");
        QTest::newRow("escape string literal: unescape adjacent literals")
            << QByteArray(R"(const char *str = @"\xc3\xa0""f23\xd0\xb1g\xd0\xb1""1";)")
            << QByteArray(R"(const char *str = "àf23бgб1";)");
    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);

        EscapeStringLiteral factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }
};

QObject *EscapeStringLiteral::createTest() { return new EscapeStringLiteralTest; }
QObject *ConvertCStringToNSString::createTest() { return new QObject; }
QObject *WrapStringLiteral::createTest() { return new QObject; }
QObject *TranslateStringLiteral::createTest() { return new QObject; }

#endif // WITH_TESTS
} // namespace

void registerConvertStringLiteralQuickfixes()
{
    CppQuickFixFactory::registerFactory<ConvertCStringToNSString>();
    CppQuickFixFactory::registerFactory<EscapeStringLiteral>();
    CppQuickFixFactory::registerFactory<TranslateStringLiteral>();
    CppQuickFixFactory::registerFactory<WrapStringLiteral>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <convertstringliteral.moc>
#endif
