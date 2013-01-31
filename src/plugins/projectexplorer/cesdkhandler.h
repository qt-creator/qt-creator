/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CE_SDK_HANDLER_H
#define CE_SDK_HANDLER_H

#include <projectexplorer/projectexplorer.h>

#include <QStringList>
#include <QDir>

namespace Utils {
class Environment;
}

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT CeSdkInfo
{
public:
    CeSdkInfo();
    inline QString                  name() const;
    inline QString                  binPath() const;
    inline QString                  includePath() const;
    inline QString                  libPath() const;
    void                            addToEnvironment(Utils::Environment &env);
    inline bool                     isValid() const;
    inline int                      majorVersion() const;
    inline int                      minorVersion() const;
    inline bool                     isSupported() const;
private:
    friend class                    CeSdkHandler;
    QString                         m_name;
    QString                         m_bin;
    QString                         m_include;
    QString                         m_lib;
    int                             m_major;
    int                             m_minor;
};

inline QString CeSdkInfo::name() const { return m_name; }
inline QString CeSdkInfo::binPath() const { return m_bin; }
inline QString CeSdkInfo::includePath() const { return m_include; }
inline QString CeSdkInfo::libPath() const { return m_lib; }
inline bool CeSdkInfo::isValid() const { return !m_name.isEmpty() && !m_bin.isEmpty() && !m_include.isEmpty() && !m_lib.isEmpty(); }
inline int CeSdkInfo::majorVersion() const { return m_major; }
inline int CeSdkInfo::minorVersion() const { return m_minor; }
inline bool CeSdkInfo::isSupported() const { return m_major >= 5; }

class PROJECTEXPLORER_EXPORT CeSdkHandler
{
public:
                                    CeSdkHandler();
    bool                            parse(const QString &path);
    inline QList<CeSdkInfo>         listAll() const;
    CeSdkInfo                       find(const QString &name) const;
    static QString                  platformName(const QString &qtpath);
private:
    inline QString                  fixPaths(QString path) const;
    QList<CeSdkInfo>                m_list;
    QString                         VCInstallDir;
    QString                         VSInstallDir;
};

inline QList<CeSdkInfo> CeSdkHandler::listAll() const
{
    return m_list;
}

inline QString CeSdkHandler::fixPaths(QString path) const
{
    const char vcInstallMacro[] = "$(VCInstallDir)";
    const char vsInstallMacro[] = "$(VSInstallDir)";

    path.replace(QLatin1String(vcInstallMacro), VCInstallDir);
    path.replace(QLatin1String(vsInstallMacro), VSInstallDir);
    path.remove(QLatin1String(";$(PATH)"));
    return QDir::toNativeSeparators(path);
}

} // namespace Qt4ProjectManager

#endif // CE_SDK_HANDLER_H
