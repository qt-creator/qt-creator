/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef KSYNTAXHIGHLIGHTING_DEFINITIONREF_P_H
#define KSYNTAXHIGHLIGHTING_DEFINITIONREF_P_H

#include <memory>

namespace KSyntaxHighlighting {

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
    DefinitionRef& operator=(const Definition &def);

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

