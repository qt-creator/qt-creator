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

#include "profilereader.h"
#include "profilecache.h"
#include "proitems.h"

#include <QtTest/QtTest>
#include <QtCore/QSet>

using Qt4ProjectManager::Internal::ProFileReader;
using Qt4ProjectManager::Internal::ProFileCache;


class tst_ProFileReader : public QObject
{
    Q_OBJECT

private slots:
    void readProFile();
    void includeFiles();
};

void tst_ProFileReader::readProFile()
{
    ProFileCache cache;
    ProFileReader reader(&cache);
    QCOMPARE(reader.readProFile("nonexistant"), false);
    QCOMPARE(reader.readProFile("data/includefiles/test.pro"), true);
}

void tst_ProFileReader::includeFiles()
{
    ProFileCache cache;
    ProFileReader reader(&cache);
    QCOMPARE(reader.readProFile("data/includefiles/test.pro"), true);
    QCOMPARE(reader.includeFiles().size(), 2);
}

QTEST_MAIN(tst_ProFileReader)
#include "main.moc"
