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

#include "qmakeglobals.h"
#include "qmakeparser.h"
#include "qmakeevaluator.h"
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

static void print(const QString &fileName, int lineNo, int type, const QString &msg)
{
    QString pfx = ((type & QMakeHandler::CategoryMask) == QMakeHandler::WarningMessage)
                  ? QString::fromLatin1("WARNING: ") : QString();
    if (lineNo > 0)
        qWarning("%s%s:%d: %s", qPrintable(pfx), qPrintable(fileName), lineNo, qPrintable(msg));
    else if (lineNo)
        qWarning("%s%s: %s", qPrintable(pfx), qPrintable(fileName), qPrintable(msg));
    else
        qWarning("%s%s", qPrintable(pfx), qPrintable(msg));
}

class EvalHandler : public QMakeHandler {
public:
    virtual void message(int type, const QString &msg, const QString &fileName, int lineNo)
        { print(fileName, lineNo, type, msg); }

    virtual void fileMessage(const QString &msg)
        { qWarning("%s", qPrintable(msg)); }

    virtual void aboutToEval(ProFile *, ProFile *, EvalFileType) {}
    virtual void doneWithEval(ProFile *) {}
};

static EvalHandler evalHandler;

static int evaluate(const QString &fileName, const QString &in_pwd, const QString &out_pwd,
                    bool cumulative, ProFileGlobals *option, QMakeParser *parser, int level)
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
    if (!(pro = parser->parsedProFile(fileName))) {
        if (!QFile::exists(fileName)) {
            qCritical("Input file %s does not exist.", qPrintable(fileName));
            return 3;
        }
        return 2;
    }
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
                realDir = QFileInfo(visitor.value(subDirKey)).filePath();
            else if (visitor.contains(subDirFileKey))
                realDir = QFileInfo(visitor.value(subDirFileKey)).filePath();
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

    ProFileGlobals option;
    QString qmake = QString::fromLocal8Bit(qgetenv("TESTREADER_QMAKE"));
    if (qmake.isEmpty())
        qmake = QLibraryInfo::location(QLibraryInfo::BinariesPath) + QLatin1String("/qmake");
    option.qmake_abslocation = QDir::cleanPath(qmake);
    option.initProperties();

    QStringList args = app.arguments();
    args.removeFirst();
    int level = -1; // verbose
    bool cumulative = false;
    QString file;
    QString in_pwd;
    QString out_pwd;
    QMakeCmdLineParserState state(QDir::currentPath());
    for (int pos = 0; ; ) {
        QMakeGlobals::ArgumentReturn cmdRet = option.addCommandLineArguments(state, args, &pos);
        if (cmdRet == QMakeGlobals::ArgumentsOk)
            break;
        if (cmdRet == QMakeGlobals::ArgumentMalformed) {
            qCritical("argument %s needs a parameter", qPrintable(args.at(pos - 1)));
            return 3;
        }
        Q_ASSERT(cmdRet == QMakeGlobals::ArgumentUnknown);
        QString arg = args.at(pos++);
        if (arg == QLatin1String("-v")) {
            level = 0;
        } else if (arg == QLatin1String("-d")) {
            option.debugLevel++;
        } else if (arg == QLatin1String("-c")) {
            cumulative = true;
        } else if (arg.startsWith(QLatin1Char('-'))) {
            qCritical("unrecognized option %s", qPrintable(arg));
            return 3;
        } else if (file.isEmpty()) {
            QFileInfo infi(arg);
            file = QDir::cleanPath(infi.absoluteFilePath());
            in_pwd = QDir::cleanPath(infi.absolutePath());
        } else if (out_pwd.isEmpty()) {
            out_pwd = QDir::cleanPath(QFileInfo(arg).absoluteFilePath());
        } else {
            qCritical("excess argument '%s'", qPrintable(arg));
            return 3;
        }
    }
    if (file.isEmpty()) {
        qCritical("usage: testreader [-v] [-d [-d]] [-c] <filenme> [<out_pwd>] [<variable assignments>]");
        return 3;
    }
    if (out_pwd.isEmpty())
        out_pwd = in_pwd;
    option.setDirectories(in_pwd, out_pwd);

    QMakeParser parser(0, &evalHandler);
    return evaluate(file, in_pwd, out_pwd, cumulative, &option, &parser, level);
}
