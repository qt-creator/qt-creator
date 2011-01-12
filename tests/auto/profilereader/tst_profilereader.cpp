/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "profilecache.h"

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
    void conditionalScopes();
};

void tst_ProFileReader::readProFile()
{
    ProFileReader reader;
    QCOMPARE(reader.readProFile("nonexistant"), false);
    QCOMPARE(reader.readProFile("data/includefiles/test.pro"), true);
}

void tst_ProFileReader::includeFiles()
{
    ProFileReader reader;
    QCOMPARE(reader.readProFile("data/includefiles/test.pro"), true);
    QCOMPARE(reader.includeFiles().size(), 2);
}

void tst_ProFileReader::conditionalScopes()
{
    ProFileReader reader;
    QCOMPARE(reader.readProFile("data/includefiles/test.pro"), true);

    QStringList scopedVariable = reader.values("SCOPED_VARIABLE");
    QCOMPARE(scopedVariable.count(), 1);
    QCOMPARE(scopedVariable.first(), QLatin1String("1"));
}

QTEST_MAIN(tst_ProFileReader)
#include "tst_profilereader.moc"
