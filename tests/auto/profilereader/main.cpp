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
#include "main.moc"
