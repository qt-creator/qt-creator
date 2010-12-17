/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "profileparser.h"
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

static void print(const QString &fileName, int lineNo, const QString &msg)
{
    if (lineNo)
        qWarning("%s(%d): %s", qPrintable(fileName), lineNo, qPrintable(msg));
    else
        qWarning("%s", qPrintable(msg));
}

class ParseHandler : public ProFileParserHandler {
public:
    virtual void parseError(const QString &fileName, int lineNo, const QString &msg)
        { print(fileName, lineNo, msg); }
};

class EvalHandler : public ProFileEvaluatorHandler {
public:
    virtual void configError(const QString &msg)
        { qWarning("%s", qPrintable(msg)); }
    virtual void evalError(const QString &fileName, int lineNo, const QString &msg)
        { print(fileName, lineNo, msg); }
    virtual void fileMessage(const QString &msg)
        { qWarning("%s", qPrintable(msg)); }

    virtual void aboutToEval(ProFile *, ProFile *, EvalFileType) {}
    virtual void doneWithEval(ProFile *) {}
};

static ParseHandler parseHandler;
static EvalHandler evalHandler;

static QString value(ProFileEvaluator &reader, const QString &variable)
{
    QStringList vals = reader.values(variable);
    if (!vals.isEmpty())
        return vals.first();

    return QString();
}

static int evaluate(const QString &fileName, const QString &in_pwd, const QString &out_pwd,
                    bool cumulative, ProFileOption *option, ProFileParser *parser, int level)
{
    static QSet<QString> visited;
    if (visited.contains(fileName))
        return 0;
    visited.insert(fileName);

    ProFileEvaluator visitor(option, parser, &evalHandler);
    visitor.setCumulative(cumulative);
    visitor.setOutputDir(out_pwd);

    ProFile *pro;
    if (!(pro = parser->parsedProFile(fileName)))
        return 2;
    if (!visitor.accept(pro)) {
        pro->deref();
        return 2;
    }

    if (visitor.templateType() == ProFileEvaluator::TT_Subdirs) {
        QStringList subdirs = visitor.values(QLatin1String("SUBDIRS"));
        subdirs.removeDuplicates();
        foreach (const QString &subDirVar, subdirs) {
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

            QString inFile = QDir::cleanPath(info.absoluteFilePath()),
                    inPwd = QDir::cleanPath(info.path()),
                    outPwd = QDir::cleanPath(QDir(out_pwd).absoluteFilePath(
                            QDir(in_pwd).relativeFilePath(info.path())));
            int nlevel = level;
            if (nlevel >= 0) {
                printf("%sReading %s%s\n", QByteArray().fill(' ', nlevel).constData(),
                       qPrintable(inFile), (inPwd == outPwd) ? "" :
                       qPrintable(QString(QLatin1String(" [") + outPwd + QLatin1Char(']'))));
                fflush(stdout);
                nlevel++;
            }
            evaluate(inFile, inPwd, outPwd, cumulative, option, parser, nlevel);
        }
    }

    pro->deref();
    return 0;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QStringList args = app.arguments();
    args.removeFirst();
    int level = -1; // verbose
    if (args.count() && args.first() == QLatin1String("-v"))
        level = 0, args.removeFirst();
    if (args.count() < 2)
        qFatal("need at least two arguments: [-v] <cumulative?> <filenme> [<out_pwd>]");

    ProFileOption option;
    ProFileParser parser(0, &parseHandler);

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

    return evaluate(file, in_pwd, out_pwd, cumulative, &option, &parser, level);
}
