/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qsystem.h"

#include <QApplication>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QtNetwork/QUdpSocket>
#include <QTimer>
#include <QHostInfo>

#include <stdlib.h>

#if defined Q_OS_UNIX
# include <sys/utsname.h>
# include <sys/stat.h>
# include <utime.h>
#endif
// WinCE doesn't have time.h
#ifndef Q_OS_TEMP
# include <time.h>
#endif

#ifdef Q_OS_WIN32
# include <windows.h>
# include <stdlib.h>
#endif

#ifdef Q_OS_UNIX
# if defined(Q_OS_MAC)
#  include <crt_externs.h>
#  define environ (*_NSGetEnviron())
# else
#  include <unistd.h>
   extern char **environ;
# endif
#endif

//workaround for Qt 4
#if !defined(QT_THREAD_SUPPORT) && !defined(QT_NO_THREAD)
#  define QT_THREAD_SUPPORT
#endif

/*!
    \class QSystem qsystem.h
    \mainclass
    \brief The QSystem class is a wrapper class that provides a platform independent
    API for some frequently used platform dependent functions.
*/

/*!
    Returns the lowercase username as known to the system.
*/
QString QSystem::userName()
{
    QString userName;

#if defined Q_OS_TEMP
    userName = "WinCE";
#elif defined Q_OS_WIN32
    userName = getenv("USERNAME");
#elif defined Q_OS_UNIX
    userName = getenv("USER");
    if (userName == "")
        userName = getenv("LOGNAME");
#elif defined Q_OS_MAC
    userName = getenv();
#endif

    return userName.toLower();
}

/*!
    Returns the lowercase hostname (computername) as known to the system.
    Only the first part of a hostname is returned, e.g. foo.troll.no will return foo
    as the hostname.
*/
QString QSystem::hostName()
{
    static QString hostName;
    if (!hostName.isEmpty())
        return hostName;

    hostName = QHostInfo::localHostName();

    // convert anarki.troll.no to anarki
    int pos = hostName.indexOf(".");
    if (pos > 0)
        hostName = hostName.left(pos);

    return hostName;
}

/*!
    Returns the operating system on which the app is running.
*/
QString QSystem::OSName()
{
#if defined(Q_OS_MAC)
    return "Mac-OSX";
#elif defined(Q_OS_MSDOS)
    return "MSDOS";
#elif defined(Q_OS_OS2EMX)
    return "OS2EMX";
#elif defined(Q_OS_OS2)
    return "OS2";
#elif defined(Q_OS_WIN32) || defined (Q_OS_WIN64)
    return "Windows";
#elif defined(Q_OS_SOLARIS)
    return "Solaris";
#elif defined(Q_OS_SUN) && defined(Q_OS_BSD4)
    return "Sun";
#elif defined(Q_OS_HPUX)
    return "HPUX";
#elif defined(Q_OS_ULTRIX)
    return "ULTRIX";
#elif defined(Q_OS_RELIANT)
    return "RELIANT";
#elif defined(Q_OS_LINUX)
    return "Linux";
#elif defined(Q_OS_FREEBSD) && defined(Q_OS_BSD4)
    return "FREEBSD";
#elif defined(Q_OS_NETBSD) && defined(Q_OS_BSD4)
    return "NETBSD";
#elif defined(Q_OS_OPENBSD) && defined(Q_OS_BSD4)
    return "OPENBSD";
#elif defined(Q_OS_BSDI) && defined(Q_OS_BSD4)
    return "BSDI";
#elif defined(Q_OS_IRIX)
    return "IRIX";
#elif defined(Q_OS_OSF)
    return "OSF";
#elif defined(Q_OS_AIX)
    return "AIX";
#elif defined(Q_OS_LYNX)
    return "LYNX";
#elif defined(Q_OS_UNIXWARE)
    return "UNIXWARE";
#elif defined(Q_OS_HURD)
    return "HURD";
#elif defined(Q_OS_DGUX)
    return "DGUX";
#elif defined(Q_OS_QNX6)
    return "QNX6";
#elif defined(Q_OS_QNX)
    return "QNX";
#elif defined(Q_OS_SCO)
    return "SCO";
#elif defined(Q_OS_UNIXWARE7)
    return "UNIXWARE7";
#elif defined(Q_OS_DYNIX)
    return "DYNIX";
#elif defined(Q_OS_SVR4)
    return "SVR4";
#else
    return "UNKNOWN";
#endif
}

QString which_p(const QString &path, const QString &applicationName)
{
    QStringList paths;
#if defined Q_OS_WIN32
    paths = path.split(";");
#else
    paths = path.split(":");
#endif
    foreach (const QString &p, paths) {
        QString fname = p + QDir::separator() + applicationName;
        if (QFile::exists(fname))
            return fname;
#if defined Q_OS_WIN32
        if (QFile::exists(fname + ".exe"))
            return fname + ".exe";
        if (QFile::exists(fname + ".bat"))
            return fname + ".bat";
#endif
    }
    return "";
}

/*!
    Traverses through the PATH setting as specified in \a path and returns the full
    path + appname if found. On Windows the application name may be appended with .exe or .bat
    (if either exists).
*/
QString QSystem::which(const QString &path, const QString &applicationName)
{
    QString ret = which_p(path, applicationName);
    if (ret.contains(" "))
        return '"' + ret + '"';
    return ret;
}

/*!
    Processes a string \a envString. If the envString contains a set (export)
    command then the information in the envString is used to update \a envList.
    If the envString does not contain a set command then the string is processed
    and all references to environment values are replaced with their actual values
    from \a envList.
    The function returns true if 'something' is converted.

    Example1:
    envString == "export PATH=/home/myname/bin:/home/bin"
    envList == {
    export PATH=foo
    export QTDIR=/home/qt
    export QTESTDIR=/home/qtest
    }
    QSystem::processEnvValue(envList, value);
    envList == {
    export PATH=/home/myname/bin:/home/bin
    export QTDIR=/home/qt
    export QTESTDIR=/home/qtest
    }

    Example2:
    envString == "cd $(QTDIR)/bin"
    envList == {
    export PATH=foo
    export QTDIR=/home/qt
    export QTESTDIR=/home/qtest
    }

    QSystem::processEnvValue(envList, value);
    envString == "cd /home/qt/bin"

  \sa getEnvList()
*/
bool QSystem::processEnvValue(QStringList *envList, QString &envString)
{
    if (envList == 0)
        return false;

    QString _envString = envString;
    QString prefix;
    bool isSetCmd = false;

    if (_envString.startsWith("export ")) {
        _envString = _envString.mid(7);
        prefix = "export ";
        isSetCmd = true;
    }

    if (_envString.startsWith("set ")) {
        _envString = _envString.mid(4);
        prefix = "set ";
        isSetCmd = true;
    }

    if (isSetCmd) {
        int pos = _envString.indexOf("=");
        if (pos < 0) {
            // nothing to do
            return false;
        }
        QString key = _envString.left(pos);
        QString value = _envString.mid(pos + 1);

        if (processEnvValue(envList, value)) {
            setEnvKey(envList, key, value);

            envString = /*prefix +*/ key + "=" + value;
            return true;
        } else {
            return false;
        }

    } else {
        int pos = -1;
        int start = -1;
        QString key;
        QString replacementValue;
        while (hasEnvKey(_envString, key, pos, start)) {
            // prevent infinite loops! If there is no value we replace the key with an empty string
            replacementValue = envKey(envList, key);
            int len = key.length()+1;
            if (_envString.at(pos) == '%')
                len++;
            if (_envString.indexOf("("+ key + ")") == (pos + 1))
                    len += 2;
            _envString.remove(pos, len);
            _envString.insert(pos, replacementValue);
            start = pos + 1;
        }

        // We really can't be sure we're dealing with a path...
        envString = _envString;

        return true;
    }
}

/*!
    Searches the given \a envString for the specified \a key after position
    \a start.
    If the \a key is found, \a pos returns the position of the key in the string and
    the function returns true.
*/
bool QSystem::hasEnvKey(const QString &envString, QString &key, int &pos, int start)
{
    pos = -1;
    key = "";
    int end = -1;
    bool winKey = false;

    // we actually want to start after the position we specify
    ++start;

    int pos1, pos2, pos3;
    pos1 = envString.indexOf("$", start);
    pos2 = envString.indexOf("%", start);
    pos3 = envString.indexOf("%", pos2 + 1);

    if (pos1 < 0 && pos2 < 0)
        return false;

    if (pos1 < 0) {
        if (pos3 < 0)
            return false;
        pos = pos2;
        winKey = true; // must search for a terminating % char...
    } else if (pos2 < 0) {
        pos = pos1;
    } else {
        if (pos1 < pos2) {
            pos = pos1;
        } else {
            if ((pos3 < 0) || (pos3 > pos1)) {
                pos = pos1;
            } else {
                pos = pos2;
                winKey = true; // must search for a terminating % char...
            }
        }
    }

    if (pos >= start) {
        end = -1;
        if (winKey) {
            // if we found a % we only look for a closing one
            end = envString.indexOf("%", pos+1);

        } else {

            // else look for some other characters that close a KEY
            const uint maxSrch = 8;
            int term[maxSrch];
            term[0] = envString.indexOf("/", pos+1);
            term[1] = envString.indexOf(":", pos+1);
            term[2] = envString.indexOf("\\", pos+1);
            term[3] = envString.indexOf(";", pos+1);
            term[4] = envString.indexOf(" ", pos+1);
            term[5] = envString.indexOf("-", pos+1);
            term[6] = envString.indexOf(".", pos+1);
            term[7] = envString.length();

            // now lets see which char comes first after pos: Thats the one we are looking for.
            for (uint i = 0; i < maxSrch; ++i) {
                if (end < 0)
                    end = term[i];
                else if ((term[i] >= 0) && (term[i] < end))
                    end = term[i];
            }
        }

        if (end > pos) {
            int extraKey = 0;
            if (winKey)
                extraKey = 1;

            QString tmpKey = envString.mid(pos + 1, end - (pos+1));

            // see if we have a string that looks like $(QTDIR) and convert it to QTDIR
            if (tmpKey.startsWith("(") && tmpKey.endsWith(")"))
                tmpKey = tmpKey.mid(1, tmpKey.length()-2);

            if (!tmpKey.isEmpty()) {
                key = tmpKey;
                return true;
            }
        }
    }

    return false;
}

/*!
    Removes any occurance of environment \a key from the environment \a list.
    The function returns true if the list was valid and no more instances of \a key
    exist in the list.
*/
bool QSystem::unsetEnvKey(QStringList *list, const QString &key)
{
    if (list == 0)
        return false;

    if (list->size() == 0)
        return true;

    QString envKey = key.toUpper();
    QStringList::Iterator it;
    for (int i = list->size() - 1; i > 0; --i) {
        QString s = list->at(i);
        int pos = s.indexOf("=");
        if (pos > 0) {
            QString keyName = (s.left(pos)).toUpper();
            if (keyName == envKey) {
                list->removeAt(i);
            }
        }
    }
    return true;
}

/*!
    Replaces or appends an environment \a key with \a value in the environment \a list.
    The key/value will be appended to the list if it is not found.
*/
bool QSystem::setEnvKey(QStringList *list, const QString &key, const QString &value)
{
    if (list == 0)
        return false;

    QString envKey = key.toUpper();
    QString envValue = value;

    QString replacementValue;
    int pos;
    int start = -1;
    bool found;

    do {
        found = false;
        QString tmpKey;
        if (hasEnvKey(envValue, tmpKey, pos, start)) {
            replacementValue = QSystem::envKey(list, tmpKey.toUpper());
            if (!replacementValue.isEmpty()) {
                int len = tmpKey.length() + 1;
                if (envValue.at(pos) == '%')
                    ++len;
                envValue.remove(pos, len);
                envValue.insert(pos, replacementValue);
            }
            found = true;
            start = pos + 1;
        }
    } while ((pos >= 0) && found);

    QStringList::Iterator end = list->end();
    for (QStringList::Iterator it = list->begin(); it != end; ++it) {
        QString s = *it;
        pos = s.indexOf("=");
        if (pos > 0) {
            QString keyName = (s.left(pos)).toUpper();
            if (keyName == envKey) {
                *it = envKey + "=" + envValue;
                list->sort();
                return true;
            }
        }
    }

    list->append(envKey + "=" + envValue);
    list->sort();
    return true;
}

void QSystem::addEnvPath(QStringList *environment, const QString &key, const QString &addedPath)
{
    QString dyldPath = QSystem::envKey(environment, key);
    if (!dyldPath.contains(addedPath)) {
#ifdef Q_OS_WIN
        dyldPath = addedPath + ";" + dyldPath;
#else
        dyldPath = addedPath + ":" + dyldPath;
#endif
        QSystem::setEnvKey(environment, key, dyldPath);
    }
}

/*!
    Searches the environment \a list for a \a key.
    If the key is found the function retuns true and \a value returns the value found in the
    list.
*/
QString QSystem::envKey(QStringList *list, const QString &key)
{
    QString value;
    if (list == 0)
        return "";

    QString srchKey = key.toUpper();

    int pos;
    QStringList::Iterator end = list->end();
    for (QStringList::Iterator it = list->begin(); it != end; ++it) {
        QString s = *it;
        pos = s.indexOf("=");
        if (pos > 0) {
            QString keyName = (s.left(pos)).toUpper();
            if (keyName == srchKey) {
                value = s.mid(pos+1);
                if (value.endsWith(";%" + keyName + "%")) {
                    value = value.left(value.length() - (3 + keyName.length()));
                } else if (value.endsWith(":$" + keyName)) {
                    value = value.left(value.length() - (2 + keyName.length()));
                }
                return value;
            }
        }
    }

    return "";
}
