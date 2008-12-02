/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include "directoryparser.h"
#include "quickopenplugin.h"

using namespace QuickOpen::Internal;

DirectoryParser::DirectoryParser(QObject *parent)
	: QThread(parent)
{
}

DirectoryParser::~DirectoryParser()
{
    if (isRunning())
        terminate();
}

void DirectoryParser::parse(Filter filter)
{
    m_dirs = filter.directories();
    m_filters = filter.acceptedFileExtensions().split(';');
    m_blackList.clear();
    foreach (QString s, filter.skipDirectories()) {
        if (!s.trimmed().isEmpty() && !m_blackList.contains(s))
            m_blackList.insert(s);
    }
    if (!isRunning())
        start(QThread::NormalPriority);
}

void DirectoryParser::setDirectoryNameBlackList(const QStringList &lst)
{
    m_blackList.clear();
    foreach (QString s, lst) {
        if (!m_blackList.contains(s))
            m_blackList.insert(s);
    }
}

QSet<QString> DirectoryParser::files() const
{
    return m_files;
}

void DirectoryParser::run()
{
    m_files.clear();
    m_runFiles.clear();
    foreach (QString s, m_dirs) {
        if (s.isEmpty())
            continue;
        QDir dir(s);
        if (dir.exists()) {
            m_runFilters = m_filters;
            m_runBlackList = m_blackList;
            collectFiles(dir);
        }
    }
    m_files = m_runFiles;
    emit directoriesParsed();
}

void DirectoryParser::collectFiles(const QDir &dir)
{
    QString dirName = dir.absolutePath() + QLatin1String("/");
    foreach (QString f, dir.entryList(m_runFilters, QDir::Files)) {
        m_runFiles.insert(dirName + f);
    }
    foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!m_runBlackList.contains(d))
            collectFiles(dir.absolutePath() + QDir::separator() + d);
    }
}
