/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmakeglobals.h"
#include "profileparser.h"
#include "profileevaluator.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QString>
#include <QStringList>
#include <QTextCodec>

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
                    bool cumulative, QMakeGlobals *option, ProFileParser *parser, int level)
{
    static QSet<QString> visited;
    if (visited.contains(fileName))
        return 0;
    visited.insert(fileName);

    ProFileEvaluator visitor(option, parser, &evalHandler);
#ifdef PROEVALUATOR_CUMULATIVE
    visitor.setCumulative(cumulative);
#endif
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
        qFatal("need at least two arguments: [-v] <cumulative?> <filenme> [<out_pwd> [<qmake options>]]");

    QMakeGlobals option;
    option.initProperties(QLibraryInfo::location(QLibraryInfo::BinariesPath) + QLatin1String("/qmake"));
    if (args.count() >= 4)
        option.setCommandLineArguments(args.mid(3));
    ProFileParser parser(0, &parseHandler);

    bool cumulative = args[0] == QLatin1String("true");
    QFileInfo infi(args[1]);
    QString file = infi.absoluteFilePath();
    QString in_pwd = infi.absolutePath();
    QString out_pwd = (args.count() > 2) ? QFileInfo(args[2]).absoluteFilePath() : in_pwd;

    return evaluate(file, in_pwd, out_pwd, cumulative, &option, &parser, level);
}
