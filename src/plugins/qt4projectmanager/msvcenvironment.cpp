/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "msvcenvironment.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryFile>

using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::Environment;

MSVCEnvironment::MSVCEnvironment(const QString &name, const QString &path)
    : m_name(name), m_path(path), m_valuesSet(false)
{

}

QString MSVCEnvironment::name() const
{
    return m_name;
}

QString MSVCEnvironment::path() const
{
    return m_path;
}

QList<MSVCEnvironment> MSVCEnvironment::availableVersions()
{
    QList<MSVCEnvironment> result;

    QSettings registry("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VS7", 
                       QSettings::NativeFormat);
    QStringList versions = registry.allKeys();
    foreach (const QString &version, versions) {
        QString dir = registry.value(version).toString();
        result << MSVCEnvironment(version, dir);
    }
    return result;
}

QString MSVCEnvironment::description() const
{
    QString desc;
    desc = "Microsoft Visual Studio " + m_name + " from " + m_path;
    return desc;
}

// This method is ugly as hell, but that is to be expected
// It crates a temporary batch file which first runs vsvars32.bat from MSVC
// and then runs *set* to get the environment
// We cache that information.
Environment MSVCEnvironment::addToEnvironment(const Environment &env) const
{
    Environment e(env);
    if (!m_valuesSet) {
        QString desc;
        QString varsbat = m_path + "Common7\\Tools\\vsvars32.bat";
        if (QFileInfo(varsbat).exists()) {
            QTemporaryFile tf(QDir::tempPath() + "\\XXXXXX.bat");
            if (!tf.open())
                return e;
            QString filename = tf.fileName();
            tf.write("call \"" + varsbat.toLocal8Bit()+"\"\r\n");
            tf.write(("set > \"" + QDir::tempPath() + "\\qtcreator-msvc-environment.txt\"\r\n").toLocal8Bit());
            tf.flush();
            tf.waitForBytesWritten(30000);

            QProcess run;
            QString cmdPath = e.searchInPath("cmd");
            run.start(cmdPath, QStringList()<<"/c"<<filename);
            run.waitForFinished();
            tf.close();

            QFile vars(QDir::tempPath() + "\\qtcreator-msvc-environment.txt");
            if (vars.exists() && vars.open(QIODevice::ReadOnly)) {
                while (!vars.atEnd()) {
                    QByteArray line = vars.readLine();
                    QString line2 = QString::fromLocal8Bit(line);
                    line2 = line2.trimmed();
                    QRegExp regexp("(\\w*)=(.*)");
                    if (regexp.exactMatch(line2)) {
                        QString variable = regexp.cap(1);
                        QString value = regexp.cap(2);
                        value.replace('%' + variable + '%', e.value(variable));
                        m_values.append(QPair<QString, QString>(variable, value));

                    }
                }
                vars.close();
                vars.remove();
            }
        }
        m_valuesSet = true;
    }

    QList< QPair<QString, QString> >::const_iterator it, end;
    end = m_values.constEnd();
    for (it = m_values.constBegin(); it != end; ++it) {
        e.set((*it).first, (*it).second);
        qDebug()<<"variable:"<<(*it).first<<"value:"<<(*it).second;
    }


//    QFile varsbat(m_path + "Common7\\Tools\\vsvars32.bat");
//    if (varsbat.exists() && varsbat.open(QIODevice::ReadOnly)) {
//        while (!varsbat.atEnd()) {
//            QByteArray line = varsbat.readLine();
//            QString line2 = QString::fromLocal8Bit(line);
//            line2 = line2.trimmed();
//            QRegExp regexp("\\s*@?(S|s)(E|e)(T|t)\\s*(\\w*)=(.*)");
//            if (regexp.exactMatch(line2)) {
//                QString variable = regexp.cap(4);
//                QString value = regexp.cap(5);
//                value.replace('%' + variable + '%', e.value(variable));
//                e.set(variable, value);
//            }
//        }
//        varsbat.close();
//    }

    return e;
}
