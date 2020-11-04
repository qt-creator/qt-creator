/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_DEFINITIONREF_P_H
#define KSYNTAXHIGHLIGHTING_DEFINITIONREF_P_H

#include <memory>

namespace KSyntaxHighlighting
{
class Definition;
class DefinitionData;
class DefinitionPrivate;

/** Weak reference for Definition instances.
 *
 * This must be used when holding Definition instances
 * in objects hold directly or indirectly by Definition
 * to avoid reference count loops and thus memory leaks.
 *
 * @internal
 */
class DefinitionRef
{
public:
    DefinitionRef();
    explicit DefinitionRef(const Definition &def);
    ~DefinitionRef();
    DefinitionRef &operator=(const Definition &def);

    Definition definition() const;

    /**
     * Checks two definition references for equality.
     */
    bool operator==(const DefinitionRef &other) const;

    /**
     * Checks two definition references for inequality.
     */
    bool operator!=(const DefinitionRef &other) const
    {
        return !(*this == other);
    }

private:
    friend class DefinitionData;
    std::weak_ptr<DefinitionData> d;
};

}

#endif
