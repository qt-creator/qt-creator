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

//#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QVector>
#include <QTextStream>
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc != 4) {
        std::cerr << "Usage: qpatch file.list oldQtDir newQtDir" << std::endl;
        return EXIT_FAILURE;
    }

    const QByteArray files = argv[1];
    const QByteArray qtDirPath = argv[2];
    const QByteArray newQtPath = argv[3];

    if (qtDirPath.size() < newQtPath.size()) {
        std::cerr << "qpatch: error: newQtDir needs to be less than " << qtDirPath.size() << " characters."
                << std::endl;
        return EXIT_FAILURE;
    }

    QFile fn(QFile::decodeName(files));
    if (! fn.open(QFile::ReadOnly)) {
        std::cerr << "qpatch: error: file not found" << std::endl;
        return EXIT_FAILURE;
    }

    QStringList filesToPatch, textFilesToPatch;
    bool readingTextFilesToPatch = false;

    // read the input file
    QTextStream in(&fn);

    forever {
        const QString line = in.readLine();

        if (line.isNull())
            break;

        else if (line.isEmpty())
            continue;

        else if (line.startsWith(QLatin1String("%%")))
            readingTextFilesToPatch = true;

        else if (readingTextFilesToPatch)
            textFilesToPatch.append(line);

        else
            filesToPatch.append(line);
    }

    foreach (QString fileName, filesToPatch) {
        QString prefix = QFile::decodeName(newQtPath);

        if (! prefix.endsWith(QLatin1Char('/')))
            prefix += QLatin1Char('/');

        fileName.prepend(prefix);

        QFile file(fileName);
        if (! file.open(QFile::ReadOnly)) {
            std::cerr << "qpatch: warning: file `" << qPrintable(fileName) << "' not found" << std::endl;
            continue;
        }

        const QByteArray source = file.readAll();
        file.close();

        int index = 0;

        if (! file.open(QFile::WriteOnly | QFile::Truncate)) {
            std::cerr << "qpatch: error: file `" << qPrintable(fileName) << "' not writable" << std::endl;
            continue;
        }
        std::cout << "patching file `" << qPrintable(fileName) << "'" << std::endl;

        forever {
            int start = source.indexOf(qtDirPath, index);
            if (start == -1)
                break;

            int endOfString = start;
            for (; endOfString < source.size(); ++endOfString) {
                if (! source.at(endOfString))
                    break;
            }

            ++endOfString; // include the '\0'

            if (index != start)
                file.write(source.constData() + index, start - index);

            int length = endOfString - start;
            QVector<char> s;

            for (const char *x = newQtPath.constData(); x != newQtPath.constEnd(); ++x)
                s.append(*x);

            const int qtDirPathLength = qtDirPath.size();

            for (const char *x = source.constData() + start + qtDirPathLength;
                    x != source.constData() + endOfString; ++x)
                s.append(*x);

            const int oldSize = s.size();

            for (int i = oldSize; i < length; ++i)
                s.append('\0');

#if 0
            std::cout << "replace string: " << source.mid(start, endOfString - start).constData()
                    << " with: " << s.constData() << std::endl;
#endif

            file.write(s.constData(), s.size());

            index = endOfString;
        }

        if (index == 0) {
            std::cerr << "qpatch: warning: file `" << qPrintable(fileName) << "' didn't contain string to patch" << std::endl;
        }

        if (index != source.size())
            file.write(source.constData() + index, source.size() - index);
    }

    foreach (QString fileName, textFilesToPatch) {
        QString prefix = QFile::decodeName(newQtPath);

        if (! prefix.endsWith(QLatin1Char('/')))
            prefix += QLatin1Char('/');

        fileName.prepend(prefix);

        QFile file(fileName);
        if (! file.open(QFile::ReadOnly)) {
            std::cerr << "qpatch: warning: file `" << qPrintable(fileName) << "' not found" << std::endl;
            continue;
        }

        QByteArray source = file.readAll();
        file.close();

        if (! file.open(QFile::WriteOnly | QFile::Truncate)) {
            std::cerr << "qpatch: error: file `" << qPrintable(fileName) << "' not writable" << std::endl;
            continue;
        }

        std::cout << "patching text file `" << qPrintable(fileName) << "'" << std::endl;

        source.replace(qtDirPath, newQtPath);
        file.write(source);
    }

    return 0;
}
