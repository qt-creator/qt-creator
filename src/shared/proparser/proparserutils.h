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

#ifndef PROPARSERUTILS_H
#define PROPARSERUTILS_H

QT_BEGIN_NAMESPACE

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
        Option::dirlist_sep = QLatin1Char(';');
        Option::dir_sep = QLatin1Char('\\');
#else
        Option::dirlist_sep = QLatin1Char(':');
        Option::dir_sep = QLatin1Char(QLatin1Char('/'));
#endif
        Option::qmakespec = QString::fromLatin1(qgetenv("QMAKESPEC").data());
        Option::field_sep = QLatin1Char(' ');
    }

    enum StringFixFlags {
        FixNone                 = 0x00,
        FixEnvVars              = 0x01,
        FixPathCanonicalize     = 0x02,
        FixPathToLocalSeparators  = 0x04,
        FixPathToTargetSeparators = 0x08
    };
    static QString fixString(QString string, uchar flags);

    inline static QString fixPathToLocalOS(const QString &in, bool fix_env = true, bool canonical = true)
    {
        uchar flags = FixPathToLocalSeparators;
        if (fix_env)
            flags |= FixEnvVars;
        if (canonical)
            flags |= FixPathCanonicalize;
        return fixString(in, flags);
    }
};
#if defined(Q_OS_WIN32)
Option::TARG_MODE Option::target_mode = Option::TARG_WIN_MODE;
#elif defined(Q_OS_MAC)
Option::TARG_MODE Option::target_mode = Option::TARG_MACX_MODE;
#elif defined(Q_OS_QNX6)
Option::TARG_MODE Option::target_mode = Option::TARG_QNX6_MODE;
#else
Option::TARG_MODE Option::target_mode = Option::TARG_UNIX_MODE;
#endif

QString Option::qmakespec;
QString Option::dirlist_sep;
QString Option::dir_sep;
QChar Option::field_sep;

QT_END_NAMESPACE

#endif // PROPARSERUTILS_H
