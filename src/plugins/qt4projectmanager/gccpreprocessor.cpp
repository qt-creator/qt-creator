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

#include "gccpreprocessor.h"

#include <QProcess>
#include <QString>
#include <QFile>
#include <QtDebug>

using namespace Qt4ProjectManager::Internal;

GCCPreprocessor::GCCPreprocessor(const QString &gcc)
    : m_gcc(gcc)
{ }

GCCPreprocessor::~GCCPreprocessor()
{ }

void GCCPreprocessor::setGcc(const QString &gcc)
{
    if (m_gcc == gcc)
        return;
    m_gcc = gcc;
    m_predefinedMacros.clear();;
    m_systemHeaderPaths.clear();;
}

QByteArray GCCPreprocessor::predefinedMacros()
{
    if (m_predefinedMacros.isEmpty()) {
        QStringList arguments;
        arguments << QLatin1String("-xc++")
                  << QLatin1String("-E")
                  << QLatin1String("-dM")
                  << QLatin1String("-");

        QProcess cpp;
        cpp.start(m_gcc, arguments);
        cpp.closeWriteChannel();
        cpp.waitForFinished();
        m_predefinedMacros = cpp.readAllStandardOutput();
    }
    return m_predefinedMacros;
}

QList<HeaderPath> GCCPreprocessor::systemHeaderPaths()
{
    if (m_systemHeaderPaths.isEmpty()) {
        QStringList arguments;
        arguments << QLatin1String("-xc++")
                  << QLatin1String("-E")
                  << QLatin1String("-v")
                  << QLatin1String("-");

        QProcess cpp;
        cpp.setReadChannelMode(QProcess::MergedChannels);
        cpp.start(m_gcc, arguments);
        cpp.closeWriteChannel();
        cpp.waitForFinished();

        QByteArray line;
        while (cpp.canReadLine()) {
            line = cpp.readLine();
            if (line.startsWith("#include"))
                break;
        }

        if (! line.isEmpty() && line.startsWith("#include")) {
            HeaderPath::Kind kind = HeaderPath::UserHeaderPath;
            while (cpp.canReadLine()) {
                line = cpp.readLine();
                if (line.startsWith("#include")) {
                    kind = HeaderPath::GlobalHeaderPath;
                } else if (! line.isEmpty() && QChar(line.at(0)).isSpace()) {
                    HeaderPath::Kind thisHeaderKind = kind;

                    line = line.trimmed();
                    if (line.endsWith('\n'))
                        line.chop(1);

                    int index = line.indexOf(" (framework directory)");
                    if (index != -1) {
                        line = line.left(index);
                        thisHeaderKind = HeaderPath::FrameworkHeaderPath;
                    }

                    m_systemHeaderPaths.append(HeaderPath(QFile::decodeName(line), thisHeaderKind));
                } else if (line.startsWith("End of search list.")) {
                    break;
                } else {
                    qWarning() << "ignore line:" << line;
                }
            }
        }
    }
    return m_systemHeaderPaths;
}
