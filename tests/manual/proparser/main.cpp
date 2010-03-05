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

#include "profileevaluator.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLibraryInfo>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextCodec>

static QString value(ProFileEvaluator &reader, const QString &variable)
{
    QStringList vals = reader.values(variable);
    if (!vals.isEmpty())
        return vals.first();

    return QString();
}

static int evaluate(const QString &fileName, const QString &in_pwd, const QString &out_pwd,
                    bool cumulative, ProFileOption *option)
{
    static QSet<QString> visited;
    if (visited.contains(fileName))
        return 0;
    visited.insert(fileName);

    ProFileEvaluator visitor(option);
    visitor.setVerbose(true);
    visitor.setCumulative(cumulative);
    visitor.setOutputDir(out_pwd);

    ProFile *pro;
    if (!(pro = visitor.parsedProFile(fileName)))
        return 2;
    if (!visitor.accept(pro))
        return 2;

    if (visitor.templateType() == ProFileEvaluator::TT_Subdirs) {
        foreach (const QString &subDirVar, visitor.values(QLatin1String("SUBDIRS"))) {
            QString realDir;
            const QString subDirKey = subDirVar + QLatin1String(".subdir");
            const QString subDirFileKey = subDirVar + QLatin1String(".file");
            if (visitor.contains(subDirKey))
                realDir = QFileInfo(value(visitor, subDirKey)).filePath();
            else if (visitor.contains(subDirFileKey))
                realDir = QFileInfo(value(visitor, subDirFileKey)).filePath();
            else
                realDir = subDirVar;
            QFileInfo info(realDir);
            if (!info.isAbsolute())
                info.setFile(in_pwd + QLatin1Char('/') + realDir);
            if (info.isDir())
                info.setFile(QString::fromLatin1("%1/%2.pro").arg(info.filePath(), info.fileName()));
            if (!info.exists()) {
                qDebug() << "Could not find sub dir" << info.filePath();
                continue;
            }

            evaluate(QDir::cleanPath(info.absoluteFilePath()),
                     QDir::cleanPath(info.path()),
                     QDir::cleanPath(QDir(out_pwd).absoluteFilePath(
                             QDir(in_pwd).relativeFilePath(info.path()))),
                     cumulative, option);
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QStringList args = app.arguments();
    args.removeFirst();
    if (args.count() < 2)
        qFatal("need at least two arguments: <cumulative?> <filenme> [<out_pwd>]");

    ProFileOption option;

    static const struct {
        const char * const name;
        QLibraryInfo::LibraryLocation index;
    } props[] = {
        { "QT_INSTALL_DATA", QLibraryInfo::DataPath },
        { "QT_INSTALL_LIBS", QLibraryInfo::LibrariesPath },
        { "QT_INSTALL_HEADERS", QLibraryInfo::HeadersPath },
        { "QT_INSTALL_DEMOS", QLibraryInfo::DemosPath },
        { "QT_INSTALL_EXAMPLES", QLibraryInfo::ExamplesPath },
        { "QT_INSTALL_CONFIGURATION", QLibraryInfo::SettingsPath },
        { "QT_INSTALL_TRANSLATIONS", QLibraryInfo::TranslationsPath },
        { "QT_INSTALL_PLUGINS", QLibraryInfo::PluginsPath },
        { "QT_INSTALL_BINS", QLibraryInfo::BinariesPath },
        { "QT_INSTALL_DOCS", QLibraryInfo::DocumentationPath },
        { "QT_INSTALL_PREFIX", QLibraryInfo::PrefixPath }
    };
    for (unsigned i = 0; i < sizeof(props)/sizeof(props[0]); ++i)
        option.properties.insert(QLatin1String(props[i].name),
                                 QLibraryInfo::location(props[i].index));

    option.properties.insert(QLatin1String("QT_VERSION"), QLatin1String(qVersion()));

    bool cumulative = args[0] == QLatin1String("true");
    QFileInfo infi(args[1]);
    QString file = infi.absoluteFilePath();
    QString in_pwd = infi.absolutePath();
    QString out_pwd = (args.count() > 2) ? QFileInfo(args[2]).absoluteFilePath() : in_pwd;

    return evaluate(file, in_pwd, out_pwd, cumulative, &option);
}
