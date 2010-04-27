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

#ifndef LANGUAGESPECIFICFACTORIES_H
#define LANGUAGESPECIFICFACTORIES_H

#include "editorfactory.h"

namespace GenericEditor {
namespace Internal {

class CFactory : public EditorFactory
{
    Q_OBJECT
public:
    CFactory(QObject *parent = 0);
    virtual ~CFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class CppFactory : public EditorFactory
{
    Q_OBJECT
public:
    CppFactory(QObject *parent = 0);
    virtual ~CppFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class CssFactory : public EditorFactory
{
    Q_OBJECT
public:
    CssFactory(QObject *parent = 0);
    virtual ~CssFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class FortranFactory : public EditorFactory
{
    Q_OBJECT
public:
    FortranFactory(QObject *parent = 0);
    virtual ~FortranFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class HtmlFactory : public EditorFactory
{
    Q_OBJECT
public:
    HtmlFactory(QObject *parent = 0);
    virtual ~HtmlFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class JavaFactory : public EditorFactory
{
    Q_OBJECT
public:
    JavaFactory(QObject *parent = 0);
    virtual ~JavaFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class JavascriptFactory : public EditorFactory
{
    Q_OBJECT
public:
    JavascriptFactory(QObject *parent = 0);
    virtual ~JavascriptFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class ObjectiveCFactory : public EditorFactory
{
    Q_OBJECT
public:
    ObjectiveCFactory(QObject *parent = 0);
    virtual ~ObjectiveCFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class PerlFactory : public EditorFactory
{
    Q_OBJECT
public:
    PerlFactory(QObject *parent = 0);
    virtual ~PerlFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class PhpFactory : public EditorFactory
{
    Q_OBJECT
public:
    PhpFactory(QObject *parent = 0);
    virtual ~PhpFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class PythonFactory : public EditorFactory
{
    Q_OBJECT
public:
    PythonFactory(QObject *parent = 0);
    virtual ~PythonFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class RubyFactory : public EditorFactory
{
    Q_OBJECT
public:
    RubyFactory(QObject *parent = 0);
    virtual ~RubyFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class SqlFactory : public EditorFactory
{
    Q_OBJECT
public:
    SqlFactory(QObject *parent = 0);
    virtual ~SqlFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

class TclFactory : public EditorFactory
{
    Q_OBJECT
public:
    TclFactory(QObject *parent = 0);
    virtual ~TclFactory() {}

private:
    virtual Editor *createGenericEditor(QWidget *parent);
};

} // namespace Internal
} // namespace GenericEditor

#endif // LANGUAGESPECIFICFACTORIES_H
