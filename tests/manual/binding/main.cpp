/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include <AST.h>
#include <ASTVisitor.h>
#include <Control.h>
#include <Scope.h>
#include <Semantic.h>
#include <TranslationUnit.h>
#include <PrettyPrinter.h>
#include <Symbolvisitor.h>
#include <Names.h>
#include <Symbols.h>
#include <Literals.h>

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>


////////////////////////////////////////////////////////////////////////////////
// NamespaceBinding
////////////////////////////////////////////////////////////////////////////////

class NamespaceBinding
{
public:
    /// Constructs a binding with the given parent.
    NamespaceBinding(NamespaceBinding *parent = 0);

    /// Destroys the binding.
    ~NamespaceBinding();

    /// Returns this binding's name.
    NameId *name() const;

    /// Returns this binding's identifier.
    Identifier *identifier() const;

    /// Returns the binding for the global namespace (aka ::).
    NamespaceBinding *globalNamespaceBinding();

    /// Returns the binding for the given namespace symbol.
    NamespaceBinding *findNamespaceBinding(Name *name);

    /// Returns the binding associated with the given symbol.
    NamespaceBinding *findOrCreateNamespaceBinding(Namespace *symbol);

    /// Helpers.
    std::string qualifiedId() const;
    void dump();

private:
    NamespaceBinding *findNamespaceBindingForNameId(NameId *name);

public: // attributes
    /// This binding's parent.
    NamespaceBinding *parent;

    /// Binding for anonymous namespace symbols.
    NamespaceBinding *anonymousNamespaceBinding;

    /// This binding's connections.
    Array<NamespaceBinding *> children;

    /// This binding's namespace symbols.
    Array<Namespace *> symbols;
};

NamespaceBinding::NamespaceBinding(NamespaceBinding *parent)
    : parent(parent),
      anonymousNamespaceBinding(0)
{
    if (parent)
        parent->children.push_back(this);
}

NamespaceBinding::~NamespaceBinding()
{
    for (unsigned i = 0; i < children.size(); ++i) {
        NamespaceBinding *binding = children.at(i);

        delete binding;
    }
}

NameId *NamespaceBinding::name() const
{
    if (symbols.size()) {
        if (Name *name = symbols.at(0)->name()) {
            NameId *nameId = name->asNameId();
            assert(nameId != 0);

            return nameId;
        }
    }

    return 0;
}

Identifier *NamespaceBinding::identifier() const
{
    if (NameId *nameId = name())
        return nameId->identifier();

    return 0;
}

NamespaceBinding *NamespaceBinding::globalNamespaceBinding()
{
    NamespaceBinding *it = this;

    for (; it; it = it->parent) {
        if (! it->parent)
            break;
    }

    return it;
}

NamespaceBinding *NamespaceBinding::findNamespaceBinding(Name *name)
{
    if (! name)
        return anonymousNamespaceBinding;

    else if (NameId *nameId = name->asNameId())
        return findNamespaceBindingForNameId(nameId);

    // invalid binding
    return 0;
}

NamespaceBinding *NamespaceBinding::findNamespaceBindingForNameId(NameId *name)
{
    for (unsigned i = 0; i < children.size(); ++i) {
        NamespaceBinding *binding = children.at(i);
        Name *bindingName = binding->name();

        if (! bindingName)
            continue;

        if (NameId *bindingNameId = bindingName->asNameId()) {
            if (name->isEqualTo(bindingNameId))
                return binding;
        }
    }

    return 0;
}

NamespaceBinding *NamespaceBinding::findOrCreateNamespaceBinding(Namespace *symbol)
{
    if (NamespaceBinding *binding = findNamespaceBinding(symbol->name())) {
        unsigned index = 0;

        for (; index < binding->symbols.size(); ++index) {
            Namespace *ns = binding->symbols.at(index);

            if (ns == symbol)
                break;
        }

        if (index == binding->symbols.size())
            binding->symbols.push_back(symbol);

        return binding;
    }

    NamespaceBinding *binding = new NamespaceBinding(this);
    binding->symbols.push_back(symbol);

    if (! symbol->name()) {
        assert(! anonymousNamespaceBinding);

        anonymousNamespaceBinding = binding;
    }

    return binding;
}

// ### rewrite me
std::string NamespaceBinding::qualifiedId() const
{
    if (! parent)
        return "<root>";

    std::string s;

    s.append(parent->qualifiedId());
    s.append("::");

    if (Identifier *id = identifier())
        s.append(id->chars(), id->size());

    else
        s.append("<anonymous>");

    return s;
}

void NamespaceBinding::dump()
{
    static int depth;

    std::cout << std::string(depth, ' ') << qualifiedId()
              << " # " << symbols.size() << std::endl;
    ++depth;

    for (unsigned i = 0; i < children.size(); ++i)
        children.at(i)->dump();

    --depth;
}

////////////////////////////////////////////////////////////////////////////////
// Binder
////////////////////////////////////////////////////////////////////////////////

class Binder: protected SymbolVisitor
{
public:
    Binder();
    virtual ~Binder();

    NamespaceBinding *operator()(Symbol *symbol)
    { return bind(symbol, 0); }

protected:
    NamespaceBinding *bind(Symbol *symbol, NamespaceBinding *binding);
    NamespaceBinding *findOrCreateNamespaceBinding(Namespace *symbol);

    NamespaceBinding *switchNamespaceBinding(NamespaceBinding *binding);

    using SymbolVisitor::visit;

    virtual bool visit(Namespace *);
    virtual bool visit(Class *);
    virtual bool visit(Function *);
    virtual bool visit(Block *);

private:
    NamespaceBinding *namespaceBinding;
};

Binder::Binder()
    : namespaceBinding(0)
{ }

Binder::~Binder()
{ }

NamespaceBinding *Binder::bind(Symbol *symbol, NamespaceBinding *binding)
{
    NamespaceBinding *previousBinding = switchNamespaceBinding(binding);
    accept(symbol);
    return switchNamespaceBinding(previousBinding);
}

NamespaceBinding *Binder::findOrCreateNamespaceBinding(Namespace *symbol)
{
    if (namespaceBinding)
        return namespaceBinding->findOrCreateNamespaceBinding(symbol);

    namespaceBinding = new NamespaceBinding;
    namespaceBinding->symbols.push_back(symbol);
    return namespaceBinding;
}

NamespaceBinding *Binder::switchNamespaceBinding(NamespaceBinding *binding)
{
    NamespaceBinding *previousBinding = namespaceBinding;
    namespaceBinding = binding;
    return previousBinding;
}

bool Binder::visit(Namespace *symbol)
{
    NamespaceBinding *binding = findOrCreateNamespaceBinding(symbol);

    for (unsigned i = 0; i < symbol->memberCount(); ++i) {
        Symbol *member = symbol->memberAt(i);

        bind(member, binding);
    }

    return false;
}

bool Binder::visit(Class *)
{ return false; }

bool Binder::visit(Function *)
{ return false; }

bool Binder::visit(Block *)
{ return false; }

////////////////////////////////////////////////////////////////////////////////
// Entry point
////////////////////////////////////////////////////////////////////////////////

static int usage()
{
    std::cerr << "cplusplus0: no input files" << std::endl;
    return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
        return usage();

    std::fstream in(argv[1]);

    if (! in)
        return usage();

    in.seekg(0, std::ios::end);
    const size_t size = in.tellg();
    in.seekp(0, std::ios::beg);

    std::vector<char> source(size + 1);
    in.read(&source[0], size);
    source[size] = '\0';

    Control control;
    StringLiteral *fileId = control.findOrInsertFileName(argv[1]);
    TranslationUnit unit(&control, fileId);
    unit.setObjCEnabled(true);
    unit.setSource(&source[0], source.size());
    unit.parse();
    if (! unit.ast())
        return EXIT_FAILURE;

    TranslationUnitAST *ast = unit.ast()->asTranslationUnit();
    assert(ast != 0);

    Namespace *globalNamespace = control.newNamespace(0, 0); // namespace symbol for `::'
    Semantic sem(&control);
    for (DeclarationAST *decl = ast->declarations; decl; decl = decl->next) {
        sem.check(decl, globalNamespace->members());
    }

    // bind
    Binder bind;
    NamespaceBinding *binding = bind(globalNamespace);
    binding->dump();
    delete binding;

    return EXIT_SUCCESS;
}
