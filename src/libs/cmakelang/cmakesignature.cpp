// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakesignature.h"

using namespace CMakeLang;

namespace {

// The variables a body sets, as far as their values are spelled out.
class Variables
{
public:
    void set(const QString &name, const std::optional<QStringList> &values);
    void append(const QString &name, const std::optional<QStringList> &values);

    // The list the text stands for, or nothing when it names a variable whose
    // value did not come out of the source.
    std::optional<QStringList> expand(const QString &text) const;

private:
    QHash<QString, QStringList> _values;
    QSet<QString> _unresolved;
};

void Variables::set(const QString &name, const std::optional<QStringList> &values)
{
    if (!values) {
        _values.remove(name);
        _unresolved.insert(name);
        return;
    }
    _values.insert(name, *values);
    _unresolved.remove(name);
}

void Variables::append(const QString &name, const std::optional<QStringList> &values)
{
    if (!values || _unresolved.contains(name)) {
        set(name, std::nullopt);
        return;
    }
    _values[name].append(*values);
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

std::optional<QStringList> expandArguments(const Variables &variables,
                                           ListView<ArgumentAST *> arguments,
                                           int from)
{
    QStringList values;
    for (int i = from, size = arguments.size(); i < size; ++i) {
        const std::optional<QStringList> expanded = variables.expand(arguments.at(i)->value());
        if (!expanded)
            return std::nullopt;
        values += *expanded;
    }
    return values;
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
bool addKeywords(Signature *signature, CommandAST *command, const Variables &variables)
{
    const ListView<ArgumentAST *> arguments = command->arguments();
    ArgumentAST *first = arguments.first();
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
bool forwardsArguments(CommandAST *command)
{
    for (ArgumentAST *argument : command->arguments()) {
        const QString value = argument->value();
        if (value == "${ARGV}" || value == "${ARGN}")
            return true;
    }
    return false;
}

} // namespace

namespace CMakeLang {

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

    bool parsesArguments = false;
    for (CommandAST *command : document->commands()) {
        if (command->isNamed("cmake_parse_arguments")) {
            parsesArguments = true;
            break;
        }
    }
    if (!parsesArguments)
        return;

    QList<NestedCommandAST *> definitions;
    QHash<NestedCommandAST *, QList<CommandAST *>> bodies;
    for (CommandAST *command : document->commands()) {
        NestedCommandAST *definition = enclosingDefinition(document, command);
        if (!definition)
            continue;
        QList<CommandAST *> &body = bodies[definition];
        if (body.isEmpty())
            definitions.append(definition);
        body.append(command);
    }

    for (NestedCommandAST *node : definitions) {
        const QString name = definedName(node);
        if (name.isEmpty() || name.contains("${"))
            continue;

        Definition definition;
        Variables variables;
        bool spelledOut = true;
        for (CommandAST *command : bodies.value(node)) {
            const ListView<ArgumentAST *> arguments = command->arguments();
            ArgumentAST *first = arguments.first();
            if (!first)
                continue;

            if (command->isNamed("set")) {
                variables.set(first->value(), expandArguments(variables, arguments, 1));
            } else if (command->isNamed("list") && first->value() == "APPEND"
                       && arguments.size() > 1) {
                variables.append(arguments.at(1)->value(),
                                 expandArguments(variables, arguments, 2));
            } else if (command->isNamed("cmake_parse_arguments")) {
                spelledOut = addKeywords(&definition.signature, command, variables) && spelledOut;
            } else if (forwardsArguments(command)) {
                definition.forwardsTo.append(command->commandName().toLower());
            }
        }
        if (!spelledOut)
            continue;

        Definition &stored = _definitions[name.toLower()];
        stored.signature.add(definition.signature);
        stored.forwardsTo += definition.forwardsTo;
    }
}

Signature SignatureTable::signature(const QString &commandName) const
{
    QSet<QString> visited;
    return resolve(commandName.toLower(), visited);
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
