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
** contact the sales department at http://www.qtsoftware.com/contact.
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

class ProFileEvaluator
{
public:

    // This struct is from qmake, but we are not using everything.
    struct Option
    {
        //simply global convenience
        //static QString libtool_ext;
        //static QString pkgcfg_ext;
        //static QString prf_ext;
        //static QString prl_ext;
        //static QString ui_ext;
        //static QStringList h_ext;
        //static QStringList cpp_ext;
        //static QString h_moc_ext;
        //static QString cpp_moc_ext;
        //static QString obj_ext;
        //static QString lex_ext;
        //static QString yacc_ext;
        //static QString h_moc_mod;
        //static QString cpp_moc_mod;
        //static QString lex_mod;
        //static QString yacc_mod;
        static QString dir_sep;
        static QString dirlist_sep;
        static QString qmakespec;
        static QChar field_sep;

        enum TARG_MODE { TARG_UNIX_MODE, TARG_WIN_MODE, TARG_MACX_MODE, TARG_MAC9_MODE, TARG_QNX6_MODE };
        static TARG_MODE target_mode;
        //static QString pro_ext;
        //static QString res_ext;

        static void init()
        {
#ifdef Q_OS_WIN
            dirlist_sep = QLatin1Char(';');
            dir_sep = QLatin1Char('\\');
#else
            dirlist_sep = QLatin1Char(':');
            dir_sep = QLatin1Char('/');
#endif
            qmakespec = QString::fromLatin1(qgetenv("QMAKESPEC").data());
            field_sep = QLatin1Char(' ');
        }
    };

    enum TemplateType {
        TT_Unknown = 0,
        TT_Application,
        TT_Library,
        TT_Script,
        TT_Subdirs
    };

    ProFileEvaluator();
    virtual ~ProFileEvaluator();

    ProFileEvaluator::TemplateType templateType();
    virtual bool contains(const QString &variableName) const;
    void setVerbose(bool on); // Default is false
    void setCumulative(bool on); // Default is true!
    void setOutputDir(const QString &dir); // Default is empty

    // -nocache, -cache, -spec, QMAKESPEC
    // -set persistent value
    void setUserConfigCmdArgs(const QStringList &addUserConfigCmdArgs, const QStringList &removeUserConfigCmdArgs);
    void setParsePreAndPostFiles(bool on); // Default is true

    bool queryProFile(ProFile *pro);
    bool queryProFile(ProFile *pro, const QString &content); // the same as above but the content is read from "content" string, not from filesystem
    bool accept(ProFile *pro);

    void addVariables(const QHash<QString, QStringList> &variables);
    void addProperties(const QHash<QString, QString> &properties);
    QStringList values(const QString &variableName) const;
    QStringList values(const QString &variableName, const ProFile *pro) const;
    QStringList absolutePathValues(const QString &variable, const QString &baseDirectory) const;
    QStringList absoluteFileValues(
            const QString &variable, const QString &baseDirectory, const QStringList &searchDirs,
            const ProFile *pro) const;
    QString propertyValue(const QString &val) const;

    // for our descendents
    virtual ProFile *parsedProFile(const QString &fileName);
    virtual void releaseParsedProFile(ProFile *proFile);
    virtual void logMessage(const QString &msg);
    virtual void errorMessage(const QString &msg); // .pro parse errors
    virtual void fileMessage(const QString &msg); // error() and message() from .pro file

private:
    class Private;
    Private *d;

    // This doesn't help gcc 3.3 ...
    template<typename T> friend class QTypeInfo;
};

QT_END_NAMESPACE

#endif // PROFILEEVALUATOR_H
