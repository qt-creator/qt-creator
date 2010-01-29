/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef TOOLCHAIN_H
#define TOOLCHAIN_H

#include "projectexplorer_export.h"
#include "environment.h"

#include <QtCore/QString>
#include <QtCore/QPair>
#include <QtCore/QMetaType>

namespace ProjectExplorer {

class Environment;
class Project;

class PROJECTEXPLORER_EXPORT HeaderPath
{
public:
    enum Kind {
        GlobalHeaderPath,
        UserHeaderPath,
        FrameworkHeaderPath
    };

    HeaderPath()
        : _kind(GlobalHeaderPath)
    { }

    HeaderPath(const QString &path, Kind kind)
        : _path(path), _kind(kind)
    { }

    QString path() const { return _path; }
    Kind    kind() const { return _kind; }

private:
    QString _path;
    Kind _kind;
};



class PROJECTEXPLORER_EXPORT ToolChain
{
public:
    enum ToolChainType
    {
        GCC = 0,
        LinuxICC = 1,
        MinGW = 2,
        MSVC = 3,
        WINCE = 4,
        WINSCW = 5,
        GCCE = 6,
        RVCT_ARMV5 = 7,
        RVCT_ARMV6 = 8,
        LAST_VALID = 9,
        OTHER = 200,
        UNKNOWN = 201,
        INVALID = 202
    };

    virtual QByteArray predefinedMacros() = 0;
    virtual QList<HeaderPath> systemHeaderPaths() = 0;
    virtual void addToEnvironment(ProjectExplorer::Environment &env) = 0;
    virtual ToolChainType type() const = 0;
    virtual QString makeCommand() const = 0;

    ToolChain();
    virtual ~ToolChain();

    static bool equals(ToolChain *, ToolChain *);
    // Factory methods
    static ToolChain *createGccToolChain(const QString &gcc);
    static ToolChain *createMinGWToolChain(const QString &gcc, const QString &mingwPath);
    static ToolChain *createMSVCToolChain(const QString &name, bool amd64);
    static ToolChain *createWinCEToolChain(const QString &name, const QString &platform);
    static QStringList availableMSVCVersions();
    static QList<ToolChain::ToolChainType> supportedToolChains();

    static QString toolChainName(ToolChainType tc);

protected:
    virtual bool equals(ToolChain *other) const = 0;
};

class PROJECTEXPLORER_EXPORT GccToolChain : public ToolChain
{
public:
    GccToolChain(const QString &gcc);
    virtual QByteArray predefinedMacros();
    virtual QList<HeaderPath> systemHeaderPaths();
    virtual void addToEnvironment(ProjectExplorer::Environment &env);
    virtual ToolChainType type() const;
    virtual QString makeCommand() const;

protected:
    virtual bool equals(ToolChain *other) const;
    QByteArray m_predefinedMacros;
    QList<HeaderPath> m_systemHeaderPaths;
private:
    QString m_gcc;
};

// TODO this class needs to fleshed out more
class PROJECTEXPLORER_EXPORT MinGWToolChain : public GccToolChain
{
public:
    MinGWToolChain(const QString &gcc, const QString &mingwPath);
    virtual void addToEnvironment(ProjectExplorer::Environment &env);
    virtual ToolChainType type() const;
    virtual QString makeCommand() const;
protected:
    virtual bool equals(ToolChain *other) const;
private:
    QString m_mingwPath;
};

// TODO some stuff needs to be moved into this
class PROJECTEXPLORER_EXPORT MSVCToolChain : public ToolChain
{
public:
    MSVCToolChain(const QString &name, bool amd64 = false);
    virtual QByteArray predefinedMacros();
    virtual QList<HeaderPath> systemHeaderPaths();
    virtual void addToEnvironment(ProjectExplorer::Environment &env);
    virtual ToolChainType type() const;
    virtual QString makeCommand() const;
protected:
    virtual bool equals(ToolChain *other) const;
    QByteArray m_predefinedMacros;
    QString m_name;
private:
    mutable QList<QPair<QString, QString> > m_values;
    mutable bool m_valuesSet;
    mutable ProjectExplorer::Environment m_lastEnvironment;
    bool m_amd64;
};

// TODO some stuff needs to be moved into here
class PROJECTEXPLORER_EXPORT WinCEToolChain : public MSVCToolChain
{
public:
    WinCEToolChain(const QString &name, const QString &platform);
    virtual QByteArray predefinedMacros();
    virtual QList<HeaderPath> systemHeaderPaths();
    virtual void addToEnvironment(ProjectExplorer::Environment &env);
    virtual ToolChainType type() const;
protected:
    virtual bool equals(ToolChain *other) const;
private:
    QString m_platform;
};

}

Q_DECLARE_METATYPE(ProjectExplorer::ToolChain::ToolChainType);

#endif // TOOLCHAIN_H
