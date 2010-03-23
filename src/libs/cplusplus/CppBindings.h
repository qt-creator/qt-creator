/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CPPBINDINGS_H
#define CPPBINDINGS_H

#include "CppDocument.h"

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QByteArray>

namespace CPlusPlus {

class Location;
class Binding;
class NamespaceBinding;
class ClassBinding;

typedef QSharedPointer<Binding> BindingPtr;
typedef QSharedPointer<ClassBinding> ClassBindingPtr;
typedef QSharedPointer<NamespaceBinding> NamespaceBindingPtr;

class CPLUSPLUS_EXPORT Location
{
public:
    Location();
    Location(Symbol *symbol);
    Location(const StringLiteral *fileId, unsigned sourceLocation);

    inline bool isValid() const
    { return _fileId != 0; }

    inline operator bool() const
    { return _fileId != 0; }

    inline const StringLiteral *fileId() const
    { return _fileId; }

    inline unsigned sourceLocation() const
    { return _sourceLocation; }

private:
    const StringLiteral *_fileId;
    unsigned _sourceLocation;
};

class CPLUSPLUS_EXPORT Binding
{
    Q_DISABLE_COPY(Binding)

public:
    Binding() {}
    virtual ~Binding() {}

    virtual QByteArray qualifiedId() const = 0;
    virtual NamespaceBinding *asNamespaceBinding() const { return 0; }
    virtual ClassBinding *asClassBinding() const { return 0; }

    virtual ClassBinding *findClassBinding(const Name *name, QSet<const Binding *> *processed) const = 0;
    virtual Binding *findClassOrNamespaceBinding(const Identifier *id, QSet<const Binding *> *processed) const = 0;
};

class CPLUSPLUS_EXPORT NamespaceBinding: public Binding
{
public:
    /// Constructs a binding with the given parent.
    NamespaceBinding(NamespaceBinding *parent = 0);

    /// Destroys the binding.
    virtual ~NamespaceBinding();

    /// Returns this binding's name.
    const NameId *name() const;

    /// Returns this binding's identifier.
    const Identifier *identifier() const;

    /// Returns the binding for the global namespace (aka ::).
    NamespaceBinding *globalNamespaceBinding() const;

    /// Returns the binding for the given namespace symbol.
    NamespaceBinding *findNamespaceBinding(const Name *name) const;

    /// Returns the binding associated with the given symbol.
    NamespaceBinding *findOrCreateNamespaceBinding(Namespace *symbol);

    NamespaceBinding *resolveNamespace(const Location &loc,
                                       const Name *name,
                                       bool lookAtParent = true) const;

    virtual ClassBinding *findClassBinding(const Name *name, QSet<const Binding *> *processed) const;
    virtual Binding *findClassOrNamespaceBinding(const Identifier *id, QSet<const Binding *> *processed) const;

    /// Helpers.
    virtual QByteArray qualifiedId() const;
    void dump();

    virtual NamespaceBinding *asNamespaceBinding() { return this; }

    static NamespaceBinding *find(Namespace *symbol, NamespaceBinding *binding);
    static ClassBinding *find(Class *symbol, NamespaceBinding *binding);

    NamespaceBinding *parent() const {return parent_;}
    NamespaceBinding *anonymousNamespaceBinding() const {return anonymousNamespaceBinding_;}
    void setAnonymousNamespaceBinding(NamespaceBinding *binding) { anonymousNamespaceBinding_ = binding; }
    const QList<NamespaceBinding *> &children() const {return children_;}
    void addChild(NamespaceBinding *);
    void removeChild(NamespaceBinding *);
    const QList<NamespaceBinding *> &usings() const {return usings_;}
    void addUsing(NamespaceBinding *);
    void removeUsing(NamespaceBinding *);
    const QList<Namespace *> &symbols() const {return symbols_;}
    void addSymbol(Namespace *);
    void removeSymbol(Namespace *);
    const QList<ClassBinding *> &classBindings() const {return classBindings_;}
    void addClassBinding(ClassBinding *);
    void removeClassBinding(ClassBinding *);
private:
    NamespaceBinding *findNamespaceBindingForNameId(const NameId *name,
                                                    bool lookAtParentNamespace) const;

    NamespaceBinding *findNamespaceBindingForNameId_helper(const NameId *name,
                                                           bool lookAtParentNamespace,
                                                           QSet<const NamespaceBinding *> *processed) const;

private: // attributes
    /// This binding's parent.
    NamespaceBinding *parent_;

    /// Binding for anonymous namespace symbols.
    NamespaceBinding *anonymousNamespaceBinding_;

    /// This binding's connections.
    QList<NamespaceBinding *> children_;

    /// This binding's list of using namespaces.
    QList<NamespaceBinding *> usings_;

    /// This binding's namespace symbols.
    QList<Namespace *> symbols_;

    QList<ClassBinding *> classBindings_;
};

class CPLUSPLUS_EXPORT ClassBinding: public Binding
{
public:
    ClassBinding(NamespaceBinding *parent);
    ClassBinding(ClassBinding *parentClass);
    virtual ~ClassBinding();

    virtual ClassBinding *asClassBinding() { return this; }

    /// Returns this binding's name.
    const Name *name() const;

    /// Returns this binding's identifier.
    const Identifier *identifier() const;
    virtual QByteArray qualifiedId() const;

    virtual ClassBinding *findClassBinding(const Name *name, QSet<const Binding *> *processed) const;
    virtual Binding *findClassOrNamespaceBinding(const Identifier *id, QSet<const Binding *> *processed) const;

    void dump();

    Binding *parent() const { return parent_; }
    const QList<ClassBinding *> &children() const { return children_; }
    void addChild(ClassBinding *);
    void removeChild(ClassBinding *);
    const QList<Class *> &symbols() const { return symbols_; }
    void addSymbol(Class *symbol);
    void removeSymbol(Class *symbol);
    const QList<ClassBinding *> &baseClassBindings() const { return baseClassBindings_; }
    void addBaseClassBinding(ClassBinding *);
    void removeBaseClassBinding(ClassBinding *);

private: // attributes
    Binding *parent_;

    QList<ClassBinding *> children_;

    /// This binding's class symbols.
    QList<Class *> symbols_;

    /// Bindings for the base classes.
    QList<ClassBinding *> baseClassBindings_;
};

CPLUSPLUS_EXPORT NamespaceBindingPtr bind(Document::Ptr doc, Snapshot snapshot);

} // end of namespace CPlusPlus

#endif // CPPBINDINGS_H
