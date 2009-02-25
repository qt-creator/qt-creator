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

#include "indenter.h"

#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>

#include <stdio.h>

typedef SharedTools::Indenter<QStringList::const_iterator> Indenter;

static QString fileContents(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        const QString msg = QString(QLatin1String("error: Cannot open file '%1' for reading: %2")).
                            arg(fileName).arg(f.errorString());
        qWarning(msg.toLatin1().constData());
        return QString::null;
    }

    const QString contents = QString::fromUtf8(f.readAll());
    f.close();
    if ( contents.isEmpty() ) {
        const QString msg =  QString(QLatin1String("error: File '%1' is empty")).arg(fileName);
        qWarning(msg.toLatin1().constData());
        return QString::null;
    }
    return contents;
}

static void printUsage(char *a0)
{
    const QFileInfo fi(QString::fromUtf8(a0));
    const QString usage = QString(QLatin1String("Usage: %1 [-i indent-size] [-t tab-size] file.cpp")).
                          arg(fi.fileName());
    qWarning(usage.toUtf8().constData());
}

static int integerOptionArgument(char ** &aptr, char **end)
{
    if (++aptr == end)
        return -1;
    const QString arg = QString::fromUtf8(*aptr);
    bool ok;
    const int rc = arg.toInt (&ok);
    if (!ok)
        return -1;
    return rc;
}

static QStringList parseCommandLine(char **begin, char **end)
{
    char **aptr = begin;
    if (++aptr == end)
        return QStringList();

    QStringList  fileNames;
    for ( ; aptr != end; ++aptr) {
        const char *arg = *aptr;
        if (arg[0] == '-') {
            switch (arg[1]) {
            case 't': {
                const int tabSize = integerOptionArgument(aptr, end);
                if ( tabSize == -1)
                    return QStringList();
                Indenter::instance().setTabSize(tabSize);
            }
                break;
            case 'i': {
                const int indentSize =  integerOptionArgument(aptr, end);
                if (indentSize  == -1)
                    return QStringList();
                Indenter::instance().setIndentSize(indentSize);
            }
                break;
            default:
                return  QStringList();
            }
        } else {
            fileNames.push_back(QString::fromUtf8(arg));
        }
    }
    return  fileNames;
}

int format(const QString &fileName)
{
    const QString code = fileContents(fileName);
    if (code == QString::null)
        return 1;

    QStringList program = code.split(QLatin1Char('\n'), QString::KeepEmptyParts);
    while (!program.isEmpty()) {
        if (!program.back().trimmed().isEmpty())
            break;
        program.pop_back();
    }

    QStringList p;
    QString out;

    const QChar colon = QLatin1Char(':');
    const QChar blank = QLatin1Char(' ');
    const QChar newLine = QLatin1Char('\n');

    QStringList::const_iterator cend = program.constEnd();
    for (QStringList::const_iterator it = program.constBegin(); it != cend; ++it) {
        p.push_back(*it);
        QString &line = p.back();

        QChar typedIn = Indenter::instance().firstNonWhiteSpace(line);
        if (p.last().endsWith(colon))
            typedIn = colon;

        const int indent = Indenter::instance().indentForBottomLine(p.constBegin(), p.constEnd(), typedIn);

        const QString trimmed = line.trimmed();
        // Indent the line in the list so that the formatter code sees the indented line.
        if (!trimmed.isEmpty()) {
            line = QString(indent, blank);
            line += trimmed;
        }
        out += line;
        out += newLine ;
    }

    while (out.endsWith(newLine))
        out.truncate(out.length() - 1 );

    fputs(out.toUtf8().constData(), stdout);
    return 0;
}

int main(int argc, char **argv)
{
    const QStringList fileNames = parseCommandLine(argv, argv + argc);
    if (fileNames.empty()) {
        printUsage(argv[0]);
        return 1;
    }

    foreach (const QString &fileName, fileNames)
        if (const int rc = format(fileName))
            return rc;

    return 0;
}
