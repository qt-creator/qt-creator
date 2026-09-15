// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakesignature.h"

using namespace CMakeLang;

namespace {

// The variables a body sets, as far as their values are spelled out.
class Variables
{
public:
    // A command the body may not reach gives the variable what it holds only
    // where the body reached it, and so does one that reads such a variable.
    void set(const QString &name, const std::optional<QStringList> &values, bool certain = true);
    void append(const QString &name, const std::optional<QStringList> &values,
                bool certain = true);

    // The list the text stands for, or nothing when it names a variable whose
    // value did not come out of the source.
    std::optional<QStringList> expand(const QString &text) const;

    // Whether every variable the text names holds what the body gives it
    // wherever the body goes.
    bool isCertain(const QString &text) const;

private:
    QHash<QString, QStringList> _values;
    QSet<QString> _unresolved;
    QSet<QString> _uncertain;
};

void Variables::set(const QString &name, const std::optional<QStringList> &values, bool certain)
{
    if (certain)
        _uncertain.remove(name);
    else
        _uncertain.insert(name);

    if (!values) {
        _values.remove(name);
        _unresolved.insert(name);
        return;
    }
    _values.insert(name, *values);
    _unresolved.remove(name);
}

void Variables::append(const QString &name, const std::optional<QStringList> &values, bool certain)
{
    if (!values || _unresolved.contains(name)) {
        set(name, std::nullopt, certain);
        return;
    }
    _values[name].append(*values);
    // What the variable held before stands for as little as what is appended
    // to it.
    if (!certain)
        _uncertain.insert(name);
}

std::optional<QStringList> Variables::expand(const QString &text) const
{
    QString expanded;
    for (int position = 0; position < text.size();) {
        const int start = text.indexOf("${", position);
        if (start < 0) {
            expanded += QStringView(text).mid(position);
            break;
        }
        const int end = text.indexOf(u'}', start + 2);
        if (end < 0)
            return std::nullopt;

        expanded += QStringView(text).mid(position, start - position);
        const auto it = _values.constFind(text.mid(start + 2, end - start - 2));
        if (it == _values.constEnd())
            return std::nullopt;
        expanded += it->join(u';');
        position = end + 1;
    }
    return expanded.split(u';', Qt::SkipEmptyParts);
}

bool Variables::isCertain(const QString &text) const
{
    for (int position = 0; position < text.size();) {
        const int start = text.indexOf("${", position);
        if (start < 0)
            break;
        const int end = text.indexOf(u'}', start + 2);
        if (end < 0)
            break;
        if (_uncertain.contains(text.mid(start + 2, end - start - 2)))
            return false;
        position = end + 1;
    }
    return true;
}

// The arguments of the command in a list of their own.  The walk reads them
// over and over, and the list of the AST reaches an argument only through all
// those before it.
QList<ArgumentAST *> argumentsOf(CommandAST *command)
{
    QList<ArgumentAST *> arguments;
    for (ArgumentAST *argument : command->arguments())
        arguments.append(argument);
    return arguments;
}

std::optional<QStringList> expandArguments(const Variables &variables,
                                           const QList<ArgumentAST *> &arguments,
                                           int from,
                                           int to)
{
    QStringList values;
    for (int i = from; i < to; ++i) {
        const std::optional<QStringList> expanded = variables.expand(arguments.at(i)->value());
        if (!expanded)
            return std::nullopt;
        values += *expanded;
    }
    return values;
}

std::optional<QStringList> expandArguments(const Variables &variables,
                                           const QList<ArgumentAST *> &arguments,
                                           int from)
{
    return expandArguments(variables, arguments, from, arguments.size());
}

// Whether the values the arguments stand for are ones the body has wherever it
// goes: a variable a branch or a loop may not have set holds what it does only
// where the body reached it.
bool argumentsAreCertain(const Variables &variables,
                         const QList<ArgumentAST *> &arguments,
                         int from,
                         int to)
{
    for (int i = from; i < to; ++i) {
        if (!variables.isCertain(arguments.at(i)->value()))
            return false;
    }
    return true;
}

// set(<variable> <value>... [PARENT_SCOPE])
// set(<variable> <value>... CACHE <type> <docstring> [FORCE])
// The keywords that say where the value goes are none of its values.
class SetForm
{
public:
    // Where the values end, the keyword that says where they go left out.
    int valuesEnd = 0;

    // PARENT_SCOPE is what the caller sees.  A cache entry is what it reads
    // where it has no variable of that name itself, and what it does not read
    // where it has one, so it says that whatever the variable held is no
    // longer certain rather than what it holds now.
    bool parentScope = false;
    bool cache = false;

    bool reachesCaller() const { return parentScope || cache; }
};

// The types a cache entry comes in.  CACHE is the keyword only where one of
// them stands after it.
bool isCacheType(const QString &value)
{
    static const QStringList types{"BOOL", "FILEPATH", "PATH", "STRING", "INTERNAL"};
    return types.contains(value);
}

SetForm setForm(const QList<ArgumentAST *> &arguments)
{
    SetForm form;
    form.valuesEnd = arguments.size();
    if (form.valuesEnd > 1 && arguments.last()->value() == "PARENT_SCOPE") {
        --form.valuesEnd;
        form.parentScope = true;
        // The two forms are one another's opposite: a set() that names the
        // scope of the caller writes no cache entry, whatever its values are
        // spelled like.
        return form;
    }

    // CACHE <type> <docstring> [FORCE] is the tail of the command, and
    // everything before it is a value.  A word spelled like the keyword
    // anywhere else is a value of its own.
    int tail = form.valuesEnd;
    if (tail > 1 && arguments.at(tail - 1)->value() == "FORCE")
        --tail;
    const int cache = tail - 3;
    if (cache > 0 && arguments.at(cache)->value() == "CACHE"
        && isCacheType(arguments.at(cache + 1)->value())) {
        form.valuesEnd = cache;
        form.cache = true;
    }
    return form;
}

// The function or macro the command is part of.
NestedCommandAST *enclosingDefinition(const DocumentPtr &document, CommandAST *command)
{
    const QList<AST *> constructs = document->enclosingConstructs(command);
    for (auto it = constructs.crbegin(); it != constructs.crend(); ++it) {
        if ((*it)->asFunction() || (*it)->asMacro())
            return (*it)->asNestedCommand();
    }
    return nullptr;
}

QString definedName(NestedCommandAST *definition)
{
    ArgumentAST *name = definition->openCommand->arguments().first();
    return name ? name->value() : QString();
}

// cmake_parse_arguments(<prefix> <options> <one_value> <multi_value> <args>...)
// cmake_parse_arguments(PARSE_ARGV <n> <prefix> <options> <one_value> <multi_value>)
bool addKeywords(Signature *signature, const QList<ArgumentAST *> &arguments,
                 const Variables &variables)
{
    ArgumentAST *first = arguments.value(0);
    int index = first && first->value() == "PARSE_ARGV" ? 3 : 1;
    if (arguments.size() < index + 3)
        return false;

    for (const Signature::Arity arity :
         {Signature::Option, Signature::OneValue, Signature::MultiValue}) {
        const std::optional<QStringList> keywords
            = variables.expand(arguments.at(index++)->value());
        if (!keywords)
            return false;
        signature->add(*keywords, arity);
    }
    return true;
}

// Whether the command hands the arguments of its caller on.
bool forwardsArguments(const QList<ArgumentAST *> &arguments)
{
    for (ArgumentAST *argument : arguments) {
        const QString value = argument->value();
        if (value == "${ARGV}" || value == "${ARGN}")
            return true;
    }
    return false;
}

// Whether the cmake_parse_arguments() call reads the arguments of the
// command it stands in.  One that reads the values of an argument instead,
// the way a call may look through what it was handed for PROPERTIES, says
// nothing about the command.
bool parsesOwnArguments(const QList<ArgumentAST *> &arguments)
{
    ArgumentAST *first = arguments.value(0);
    if (first && first->value() == "PARSE_ARGV")
        return true;
    if (forwardsArguments(arguments))
        return true;

    // A call may work through what an earlier one left over of the same
    // arguments, the way cmake_print_properties() looks for its mode
    // keyword in what the call for PROPERTIES did not take.
    for (ArgumentAST *argument : arguments) {
        const QString value = argument->value();
        if (value.startsWith("${") && value.endsWith("_UNPARSED_ARGUMENTS}"))
            return true;
    }
    return false;
}

// The parameters of the definition, by the position the call gives them.
QHash<QString, int> parameters(NestedCommandAST *definition)
{
    const ListView<ArgumentAST *> arguments = definition->openCommand->arguments();
    QHash<QString, int> positions;
    for (int i = 1, size = arguments.size(); i < size; ++i)
        positions.insert(arguments.at(i)->value(), i - 1);
    return positions;
}

// Whether the command stands where what it sets is none of what the command
// hands back for certain: in a branch or a loop the body may not reach at
// all, or in a block(), which is a scope of its own that neither
// PARENT_SCOPE nor the scope a macro runs in reaches past.
bool isSetAside(const DocumentPtr &document, CommandAST *command,
                NestedCommandAST *definition)
{
    const QList<AST *> constructs = document->enclosingConstructs(command);
    for (auto it = constructs.crbegin(); it != constructs.crend(); ++it) {
        if (*it == definition)
            break;
        if ((*it)->asIf() || (*it)->asElseIfClause() || (*it)->asElseClause()
            || (*it)->asForEach() || (*it)->asWhile() || (*it)->asBlock()) {
            return true;
        }
    }
    return false;
}

// The variable the argument names, or nothing where it is a value of its own.
std::optional<QString> namedVariable(const QString &text)
{
    if (!text.startsWith("${") || !text.endsWith(u'}'))
        return std::nullopt;
    const QString name = text.mid(2, text.size() - 3);
    if (name.isEmpty() || name.contains("${"))
        return std::nullopt;
    return name;
}

// The position of the parameter the text names, ARGV<n> among them: that one
// stands for the argument the call gives at the position it spells out,
// whether the definition gives it a name of its own or not.
std::optional<int> parameterPosition(const QHash<QString, int> &positions, const QString &text)
{
    const std::optional<QString> named = namedVariable(text);
    if (!named)
        return std::nullopt;

    const auto it = positions.constFind(*named);
    if (it != positions.constEnd())
        return *it;

    if (named->startsWith("ARGV")) {
        bool isNumber = false;
        const int position = QStringView(*named).mid(4).toInt(&isNumber);
        if (isNumber && position >= 0)
            return position;
    }
    return std::nullopt;
}

// The variable of the caller the command writes: the one the parameter at that
// position names, the one the body spells out itself, which is a variable of
// the caller of that very name, or none it can tell, in which case any
// variable the call names may hold something else now.
void handBack(HandedBack *handedBack,
              const QHash<QString, int> &positions,
              const Variables &variables,
              const QString &target,
              const std::optional<QStringList> &values)
{
    if (const std::optional<int> position = parameterPosition(positions, target)) {
        handedBack->positions.insert(*position, values);
        return;
    }
    const std::optional<QStringList> named = variables.expand(target);
    if (named && named->size() == 1)
        handedBack->names.insert(named->first(), values);
    else
        handedBack->anyVariable = true;
}

// Whether the command is a call to another command, rather than one the walk
// reads itself.
bool isCall(CommandAST *command)
{
    return !command->isNamed("set") && !command->isNamed("list")
           && !command->isNamed("cmake_parse_arguments") && !command->isNamed("return");
}

// The commands that name a variable to read it rather than to write it, the
// constructs of the language among them: what the body knows about a variable
// one of them names stands.  What any other command does to a variable it
// names there is no telling, so whatever the body knew about it is gone.
bool readsNamedVariables(CommandAST *command)
{
    static const QStringList reading{
        "if", "elseif", "else", "endif", "foreach", "endforeach", "while", "endwhile",
        "break", "continue", "return", "function", "endfunction", "macro", "endmacro",
        "block", "endblock", "message"};
    return reading.contains(command->commandName().toLower());
}

// The variable the argument names holds whatever the command made of it.  One
// named through another variable is no telling which, and nothing is learned
// from it.
void forget(Variables *variables, const QString &name)
{
    if (!name.isEmpty() && !name.contains("${"))
        variables->set(name, std::nullopt);
}

// The commands the body calls, each of them once.  What one of them hands
// back may be a keyword list, so the body has to be read anew whenever that
// becomes known.
QStringList calledCommands(const QList<CommandAST *> &body)
{
    QStringList names;
    for (CommandAST *command : body) {
        if (!isCall(command))
            continue;
        const QString name = command->commandName().toLower();
        if (!names.contains(name))
            names.append(name);
    }
    return names;
}

// What the body of one function or macro says about the command it defines.
class Walk
{
public:
    Signature signature;
    QStringList forwardsTo;

    // set(${out} <values> PARENT_SCOPE) hands the values back to the caller,
    // which named the variable: they belong to that argument of the call.  One
    // the body names itself belongs to the variable of the caller of that very
    // name.
    HandedBack handedBack;

    // Whether every keyword list the body hands cmake_parse_arguments() came
    // out of the source.  Half of them would group the arguments of a call
    // wrongly, so they are of no use.
    bool spelledOut = true;
};

// The variable the OUTPUT_VARIABLE keyword of the list() operation names.
std::optional<QString> outputVariable(const QList<ArgumentAST *> &arguments)
{
    for (int i = 2, size = arguments.size(); i < size - 1; ++i) {
        if (arguments.at(i)->value() == "OUTPUT_VARIABLE")
            return arguments.at(i + 1)->value();
    }
    return std::nullopt;
}

// Reads the body, taking what the commands it calls write in the scope of
// their caller from handedBack.
Walk walkBody(const DocumentPtr &document,
              NestedCommandAST *node,
              const QList<CommandAST *> &body,
              const QHash<QString, HandedBack> &handedBack,
              const QSet<QString> &defined)
{
    const QHash<QString, int> positions = parameters(node);
    const bool isMacro = node->asMacro() != nullptr;

    Walk walk;
    Variables variables;

    // A return() may have left the body before the commands that follow it:
    // what they set is then none of what the caller gets for certain.  What
    // return(PROPAGATE) hands back is not read, which says as much.
    bool mayHaveReturned = false;

    for (CommandAST *command : body) {
        const QList<ArgumentAST *> arguments = argumentsOf(command);
        ArgumentAST *first = arguments.value(0);
        if (command->isNamed("return")) {
            mayHaveReturned = true;
            // return(PROPAGATE <variable>...) hands what the body made of the
            // variables it names to the caller.  What that is the walk does
            // not follow, so whoever reads them learns that whatever they held
            // before is gone.
            if (first && first->value() == "PROPAGATE") {
                for (int i = 1, size = arguments.size(); i < size; ++i) {
                    handBack(&walk.handedBack, positions, variables,
                             arguments.at(i)->value(), std::nullopt);
                }
            }
            continue;
        }
        // set() and list() have nothing to say without an argument to work on.
        // A call may take none at all and write a variable of its caller all
        // the same.
        if (!first && (command->isNamed("set") || command->isNamed("list")))
            continue;

        if (command->isNamed("set")) {
            const SetForm form = setForm(arguments);
            const std::optional<QStringList> values
                = expandArguments(variables, arguments, 1, form.valuesEnd);
            // A set() the body may not reach gives the variable its values
            // only where the body reached it, and one that reads such a
            // variable knows them no better.
            const bool valuesAreCertain
                = !isSetAside(document, command, node)
                  && argumentsAreCertain(variables, arguments, 1, form.valuesEnd);

            // What a function sets reaches its caller with PARENT_SCOPE and
            // through the cache; a macro runs in the scope of the caller
            // anyway, and PARENT_SCOPE reaches past that one from there.
            const bool handsBack = isMacro ? !form.parentScope : form.reachesCaller();
            if (handsBack) {
                // Nothing is handed back for values that did not come out of
                // the source, for a set() the body may not reach, for one a
                // return() before it may have left out, and for a cache entry
                // the caller may read a variable of its own instead of:
                // whoever reads them then learns that whatever the variable
                // held before is gone.  Half a keyword list would group the
                // arguments of a call wrongly.
                const bool certain = (isMacro ? !form.cache : form.parentScope)
                                     && !mayHaveReturned && valuesAreCertain;
                handBack(&walk.handedBack, positions, variables, first->value(),
                         certain ? values : std::nullopt);
            }
            variables.set(first->value(), values, valuesAreCertain);
        } else if (command->isNamed("list")) {
            const QString operation = first->value();
            if (operation == "APPEND" && arguments.size() > 1) {
                variables.append(arguments.at(1)->value(),
                                 expandArguments(variables, arguments, 2),
                                 !isSetAside(document, command, node)
                                     && argumentsAreCertain(variables, arguments, 2,
                                                            arguments.size()));
            } else if (operation == "LENGTH" || operation == "GET" || operation == "JOIN"
                       || operation == "SUBLIST" || operation == "FIND") {
                // The operations that only read the list hand what they found
                // to the variable the call names last.
                forget(&variables, arguments.last()->value());
            } else if (operation == "TRANSFORM" && arguments.size() > 1) {
                // TRANSFORM hands what it made of the list to the variable
                // named after OUTPUT_VARIABLE, and leaves the list itself
                // alone where one is named.
                const std::optional<QString> output = outputVariable(arguments);
                forget(&variables, output ? *output : arguments.at(1)->value());
            } else if (arguments.size() > 1) {
                // Whatever else the operation makes of the list the walk does
                // not follow.  POP_FRONT and POP_BACK hand what they took off
                // it to the variables the call names after it.
                const int end = operation == "POP_FRONT" || operation == "POP_BACK"
                                    ? arguments.size()
                                    : 2;
                for (int i = 1; i < end; ++i)
                    forget(&variables, arguments.at(i)->value());
            }
        } else if (command->isNamed("cmake_parse_arguments")) {
            if (parsesOwnArguments(arguments)) {
                walk.spelledOut = addKeywords(&walk.signature, arguments, variables)
                                  && walk.spelledOut;
            }
        } else {
            if (forwardsArguments(arguments))
                walk.forwardsTo.append(command->commandName().toLower());

            const QString called = command->commandName().toLower();
            const HandedBack writes = handedBack.value(called);

            // The body of a command the documents define says which variables
            // of its caller it writes, and the ones it names anywhere else are
            // none of its doing.  One they do not define may write any variable
            // the call names, the way a helper of another package hands the
            // keyword lists back, and so may one that writes a variable of its
            // caller it cannot tell the name of.
            if (writes.anyVariable
                || (!defined.contains(called) && !readsNamedVariables(command))) {
                for (ArgumentAST *argument : arguments)
                    forget(&variables, argument->value());
                continue;
            }
            if (writes.isEmpty())
                continue;

            // A call the body may not reach writes what it writes only where
            // the body reached it.
            const bool certain = !isSetAside(document, command, node);
            for (auto it = writes.positions.cbegin(), end = writes.positions.cend(); it != end;
                 ++it) {
                if (it.key() >= arguments.size())
                    continue;
                // Where the call names the variable through another one,
                // there is no telling which it is.
                const QString name = arguments.at(it.key())->value();
                if (!name.contains("${"))
                    variables.set(name, it.value(), certain);
            }
            for (auto it = writes.names.cbegin(), end = writes.names.cend(); it != end; ++it)
                variables.set(it.key(), it.value(), certain);
        }
    }
    return walk;
}

// Takes in what another body of the same command hands back.  Where the two
// disagree, and where only one of them writes the variable at all, there is no
// telling which values the caller gets.
template <typename Key>
void joinValues(QHash<Key, std::optional<QStringList>> *values,
                const QHash<Key, std::optional<QStringList>> &other)
{
    for (auto it = values->begin(), end = values->end(); it != end; ++it) {
        if (!other.contains(it.key()))
            *it = std::nullopt;
    }
    for (auto it = other.cbegin(), end = other.cend(); it != end; ++it) {
        const auto at = values->constFind(it.key());
        if (at == values->constEnd() || *at != it.value())
            values->insert(it.key(), std::nullopt);
    }
}

void join(HandedBack *handedBack, const HandedBack &other)
{
    joinValues(&handedBack->positions, other.positions);
    joinValues(&handedBack->names, other.names);
    // Whichever definition ran may be the one that writes a variable it cannot
    // tell the name of.
    handedBack->anyVariable = handedBack->anyVariable || other.anyVariable;
}

QStringList sorted(const QSet<QString> &names)
{
    QStringList result(names.cbegin(), names.cend());
    result.sort();
    return result;
}

// The function and macro definitions the elements hold, the ones nested in
// another among them.  A definition whose body has no commands has none to
// reach it through, so the elements are walked rather than the commands.
void collectDefinitions(ListView<ElementAST *> elements, QList<NestedCommandAST *> *definitions)
{
    for (ElementAST *element : elements) {
        if (IfAST *ifAst = element->asIf()) {
            collectDefinitions(ifAst->elements(), definitions);
            for (ElseIfClauseAST *clause : ifAst->elseIfClauses())
                collectDefinitions(clause->elements(), definitions);
            if (ElseClauseAST *clause = ifAst->elseClause)
                collectDefinitions(clause->elements(), definitions);
        } else if (NestedCommandAST *nested = element->asNestedCommand()) {
            if (nested->asFunction() || nested->asMacro())
                definitions->append(nested);
            collectDefinitions(nested->elements(), definitions);
        }
    }
}

// The commands the document defines, and whether any of them declares
// keywords.  Looking at a document this way costs a fraction of reading it.
QStringList definedCommands(const DocumentPtr &document, bool *declaresKeywords)
{
    QStringList names;
    for (CommandAST *command : document->commands()) {
        if (command->isNamed("cmake_parse_arguments")) {
            *declaresKeywords = true;
            return {};
        }
        if (command->isNamed("function") || command->isNamed("macro")) {
            ArgumentAST *name = command->arguments().first();
            if (name && !name->value().isEmpty() && !name->value().contains("${"))
                names.append(name->value().toLower());
        }
    }
    return names;
}

} // namespace

namespace CMakeLang {

bool HandedBack::isEmpty() const
{
    return positions.isEmpty() && names.isEmpty() && !anyVariable;
}

bool HandedBack::operator==(const HandedBack &other) const
{
    return positions == other.positions && names == other.names
           && anyVariable == other.anyVariable;
}

std::optional<Signature::Arity> Signature::arity(const QString &keyword) const
{
    const auto it = _arities.constFind(keyword);
    if (it == _arities.constEnd())
        return std::nullopt;
    return *it;
}

QStringList Signature::keywords() const
{
    QStringList keywords = _arities.keys();
    keywords.sort();
    return keywords;
}

void Signature::add(const QStringList &keywords, Arity arity)
{
    for (const QString &keyword : keywords) {
        if (!_arities.contains(keyword))
            _arities.insert(keyword, arity);
    }
}

void Signature::add(const Signature &other)
{
    for (auto it = other._arities.cbegin(), end = other._arities.cend(); it != end; ++it) {
        if (!_arities.contains(it.key()))
            _arities.insert(it.key(), it.value());
    }
}

QList<KeywordArguments> groupArguments(CommandAST *command, const Signature &signature)
{
    QList<KeywordArguments> groups{KeywordArguments()};
    std::optional<Signature::Arity> current;

    for (ArgumentAST *argument : command->arguments()) {
        const std::optional<Signature::Arity> arity = signature.arity(argument->value());
        if (arity) {
            groups.append(KeywordArguments{argument, {}});
            current = arity;
            continue;
        }
        if (current == Signature::Option
            || (current == Signature::OneValue && !groups.last().values.isEmpty())) {
            groups.append(KeywordArguments());
            current.reset();
        }
        groups.last().values.append(argument);
    }

    if (!groups.first().keyword && groups.first().values.isEmpty())
        groups.removeFirst();
    return groups;
}

void SignatureTable::addDocument(const DocumentPtr &document)
{
    if (!document || !document->isValid())
        return;

    // A document that declares no keywords says nothing about any command of
    // its own.  What it defines may still be where the keyword lists of
    // another document come from, so it is looked at now and read once a
    // command turns out to call one of them.
    bool declaresKeywords = false;
    const QStringList defined = definedCommands(document, &declaresKeywords);
    if (declaresKeywords) {
        readDocument(document);
        return;
    }

    for (const QString &name : defined) {
        if (_callers.contains(name)) {
            readDocument(document);
            return;
        }
    }

    for (const QString &name : defined)
        _unread[name].append(document);
}

// The document is no longer one that was merely looked at, and the names it
// is left under would hold it for as long as the table.  Every command it
// defines is one of them, a definition with a body of no commands included.
void SignatureTable::forgetUnread(const DocumentPtr &document)
{
    bool declaresKeywords = false;
    for (const QString &name : definedCommands(document, &declaresKeywords)) {
        const auto unread = _unread.find(name);
        if (unread == _unread.end())
            continue;
        unread->removeAll(document);
        if (unread->isEmpty())
            _unread.erase(unread);
    }
}

void SignatureTable::readDocument(const DocumentPtr &document)
{
    if (_documentsRead.contains(document.get()))
        return;
    _documentsRead.insert(document.get(), document);
    forgetUnread(document);

    QList<NestedCommandAST *> definitions;
    collectDefinitions(document->ast()->elements(), &definitions);

    QHash<NestedCommandAST *, QList<CommandAST *>> bodies;
    for (CommandAST *command : document->commands()) {
        if (NestedCommandAST *definition = enclosingDefinition(document, command))
            bodies[definition].append(command);
    }

    QSet<QString> added;
    QSet<QString> newlyDefined;
    QSet<QString> called;
    for (NestedCommandAST *node : definitions) {
        const QString name = definedName(node);
        if (name.isEmpty() || name.contains("${"))
            continue;

        const QString lowered = name.toLower();
        const QList<CommandAST *> body = bodies.value(node);
        if (!_defined.contains(lowered))
            newlyDefined.insert(lowered);
        _bodies[lowered].append(Body{document, node, body});
        _defined.insert(lowered);
        for (const QString &call : calledCommands(body)) {
            _calls[lowered].insert(call);
            _callers[call].insert(lowered);
            called.insert(call);
        }
        added.insert(lowered);
    }

    // A command one of the bodies calls may stand in a document that was only
    // looked at so far.  That one is read first: what it hands back is a
    // keyword list of whoever calls it.
    for (const QString &call : sorted(called)) {
        const QList<DocumentPtr> unread = _unread.take(call);
        for (const DocumentPtr &other : unread)
            readDocument(other);
    }

    // Reading a command that calls another one says something new only where
    // what that one hands back came out different.  A command that turns out
    // to be one of the documents at all is news as well, whether it hands
    // anything back or not: whoever calls it no longer gives up the variables
    // the call names.  The callers of a command that says what it said before
    // are left alone, however many of them there are.
    QSet<QString> changed = newlyDefined;
    for (const QString &name : readingOrder(added)) {
        if (!added.contains(name) && !_calls.value(name).intersects(changed))
            continue;
        if (read(name))
            changed.insert(name);
    }
}

// The names to read for the definitions the document brought: a command
// before the ones that call it, so that what it hands back is known by the
// time they are read.  The ones that call it are read anew however far away
// they stand and whichever document they came in, since what it hands back
// may be the keyword list they were missing.
QStringList SignatureTable::readingOrder(const QSet<QString> &added) const
{
    QSet<QString> names = added;
    QStringList callers = sorted(added);
    for (int i = 0; i < callers.size(); ++i) {
        for (const QString &caller : sorted(_callers.value(callers.at(i)))) {
            if (!names.contains(caller)) {
                names.insert(caller);
                callers.append(caller);
            }
        }
    }

    QStringList order;
    QSet<QString> visited;
    for (const QString &name : sorted(names))
        addToOrder(name, names, visited, &order);
    return order;
}

void SignatureTable::addToOrder(const QString &name, const QSet<QString> &names,
                                QSet<QString> &visited, QStringList *order) const
{
    if (visited.contains(name))
        return;
    visited.insert(name);

    // Commands that hand each other values around are read in the order
    // their names sort in: there is no first one.
    for (const QString &called : sorted(_calls.value(name))) {
        if (names.contains(called))
            addToOrder(called, names, visited, order);
    }
    order->append(name);
}

// Reads the bodies that define the command anew, and says whether what it
// hands back to its caller came out different from what it did before.
bool SignatureTable::read(const QString &name)
{
    const auto bodies = _bodies.constFind(name);
    if (bodies == _bodies.constEnd())
        return false;

    Definition definition;
    std::optional<HandedBack> handedBack;
    for (const Body &body : *bodies) {
        const Walk walk
            = walkBody(body.document, body.node, body.commands, _handedBack, _defined);
        if (walk.spelledOut) {
            definition.signature.add(walk.signature);
            definition.forwardsTo += walk.forwardsTo;
        }
        if (handedBack)
            join(&*handedBack, walk.handedBack);
        else
            handedBack = walk.handedBack;
    }

    if (definition.signature.isEmpty() && definition.forwardsTo.isEmpty())
        _definitions.remove(name);
    else
        _definitions[name] = definition;

    const HandedBack values = handedBack.value_or(HandedBack());
    if (values == _handedBack.value(name))
        return false;

    if (values.isEmpty())
        _handedBack.remove(name);
    else
        _handedBack[name] = values;
    return true;
}

Signature SignatureTable::signature(const QString &commandName) const
{
    QSet<QString> visited;
    return resolve(commandName.toLower(), visited);
}

QStringList SignatureTable::forwardsTo(const QString &commandName) const
{
    QSet<QString> visited;
    QStringList result;
    collectForwarded(commandName.toLower(), visited, &result);
    return result;
}

void SignatureTable::collectForwarded(const QString &name, QSet<QString> &visited,
                                      QStringList *result) const
{
    if (visited.contains(name))
        return;
    visited.insert(name);

    const auto it = _definitions.constFind(name);
    if (it == _definitions.constEnd())
        return;

    for (const QString &forwarded : it->forwardsTo) {
        if (visited.contains(forwarded))
            continue;
        result->append(forwarded);
        collectForwarded(forwarded, visited, result);
    }
}

Signature SignatureTable::resolve(const QString &name, QSet<QString> &visited) const
{
    if (visited.contains(name))
        return {};
    visited.insert(name);

    const auto it = _definitions.constFind(name);
    if (it == _definitions.constEnd())
        return {};

    Signature signature = it->signature;
    for (const QString &forwarded : it->forwardsTo)
        signature.add(resolve(forwarded, visited));
    return signature;
}

} // namespace CMakeLang
