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

#ifndef PROFILEEVALUATOR_H
#define PROFILEEVALUATOR_H

#include "proitems.h"
#include "abstractproitemvisitor.h"

#include <QtCore/QIODevice>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QStack>

QT_BEGIN_NAMESPACE

struct ProFileOption;

class ProFileCache
{
public:
    ProFileCache() {}
    ~ProFileCache();

    void addFile(ProFile *pro);
    ProFile *getFile(const QString &fileName);

    void discardFile(const QString &fileName);
    void discardFiles(const QString &prefix);

private:
    QHash<QString, ProFile *> parsed_files;
};

class ProFileEvaluator
{
    class Private;

public:
    struct FunctionDefs {
        QHash<QString, ProBlock *> testFunctions;
        QHash<QString, ProBlock *> replaceFunctions;
    };

    enum TemplateType {
        TT_Unknown = 0,
        TT_Application,
        TT_Library,
        TT_Script,
        TT_Subdirs
    };

    ProFileEvaluator(ProFileOption *option);
    virtual ~ProFileEvaluator();

    ProFileEvaluator::TemplateType templateType();
    virtual bool contains(const QString &variableName) const;
    void setVerbose(bool on); // Default is false
    void setCumulative(bool on); // Default is true!
    void setOutputDir(const QString &dir); // Default is empty

    // -nocache, -cache, -spec, QMAKESPEC
    // -set persistent value
    void setConfigCommandLineArguments(const QStringList &addUserConfigCmdArgs, const QStringList &removeUserConfigCmdArgs);
    void setParsePreAndPostFiles(bool on); // Default is true

    // If contents is non-null, it will be used instead of the file's actual content
    ProFile *parsedProFile(const QString &fileName, const QString &contents = QString());
    bool accept(ProFile *pro);

    QStringList values(const QString &variableName) const;
    QStringList values(const QString &variableName, const ProFile *pro) const;
    QStringList absolutePathValues(const QString &variable, const QString &baseDirectory) const;
    QStringList absoluteFileValues(
            const QString &variable, const QString &baseDirectory, const QStringList &searchDirs,
            const ProFile *pro) const;
    QString propertyValue(const QString &val) const;

    // for our descendents
    virtual void aboutToEval(ProFile *proFile); // only .pri, but not .prf. or .pro
    virtual void logMessage(const QString &msg);
    virtual void errorMessage(const QString &msg); // .pro parse errors
    virtual void fileMessage(const QString &msg); // error() and message() from .pro file

private:
    Private *d;

    // This doesn't help gcc 3.3 ...
    template<typename T> friend class QTypeInfo;

    friend struct ProFileOption;
};

// This struct is from qmake, but we are not using everything.
struct ProFileOption
{
    ProFileOption();
    ~ProFileOption();

    //simply global convenience
    //QString libtool_ext;
    //QString pkgcfg_ext;
    //QString prf_ext;
    //QString prl_ext;
    //QString ui_ext;
    //QStringList h_ext;
    //QStringList cpp_ext;
    //QString h_moc_ext;
    //QString cpp_moc_ext;
    //QString obj_ext;
    //QString lex_ext;
    //QString yacc_ext;
    //QString h_moc_mod;
    //QString cpp_moc_mod;
    //QString lex_mod;
    //QString yacc_mod;
    QString dir_sep;
    QString dirlist_sep;
    QString qmakespec;
    QString cachefile;
    QHash<QString, QString> properties;
    ProFileCache *cache;

    enum TARG_MODE { TARG_UNIX_MODE, TARG_WIN_MODE, TARG_MACX_MODE, TARG_MAC9_MODE, TARG_QNX6_MODE };
    TARG_MODE target_mode;
    //QString pro_ext;
    //QString res_ext;

  private:
    friend class ProFileEvaluator;
    friend class ProFileEvaluator::Private;
    static QString field_sep; // Just a cache for quick construction
    QHash<QString, QStringList> base_valuemap; // Cached results of qmake.conf, .qmake.cache & default_pre.prf
    ProFileEvaluator::FunctionDefs base_functions;
    QStringList feature_roots;
};

QT_END_NAMESPACE

#endif // PROFILEEVALUATOR_H
