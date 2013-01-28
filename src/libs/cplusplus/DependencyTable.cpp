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

#include "DependencyTable.h"
#include "CppDocument.h"

#include <QDebug>

using namespace CPlusPlus;

QStringList DependencyTable::filesDependingOn(const QString &fileName) const
{
    int index = fileIndex.value(fileName, -1);
    if (index == -1) {
        qWarning() << fileName << "not in the snapshot";
        return QStringList();
    }

    QStringList deps;
    for (int i = 0; i < files.size(); ++i) {
        const QBitArray &bits = includeMap.at(i);

        if (bits.testBit(index))
            deps.append(files.at(i));
    }

    return deps;
}

QHash<QString, QStringList> DependencyTable::dependencyTable() const
{
    QHash<QString, QStringList> depMap;

    for (int index = 0; index < files.size(); ++index) {
        QStringList deps;
        for (int i = 0; i < files.size(); ++i) {
            const QBitArray &bits = includeMap.at(i);

            if (bits.testBit(index))
                deps.append(files.at(i));
        }
        depMap[files.at(index)] = deps;
    }

    return depMap;
}

bool DependencyTable::isValidFor(const Snapshot &snapshot) const
{
    const int documentCount = snapshot.size();
    if (documentCount != files.size())
        return false;

    for (Snapshot::const_iterator it = snapshot.begin(); it != snapshot.end(); ++it) {
        QHash<QString, QStringList>::const_iterator i = includesPerFile.find(it.key());
        if (i == includesPerFile.end())
            return false;

        if (i.value() != it.value()->includedFiles())
            return false;
    }

    return true;
}

void DependencyTable::build(const Snapshot &snapshot)
{
    includesPerFile.clear();
    files.clear();
    fileIndex.clear();
    includes.clear();
    includeMap.clear();

    const int documentCount = snapshot.size();
    files.resize(documentCount);
    includeMap.resize(documentCount);

    int i = 0;
    for (Snapshot::const_iterator it = snapshot.begin(); it != snapshot.end();
            ++it, ++i) {
        files[i] = it.key();
        fileIndex[it.key()] = i;
    }

    for (int i = 0; i < files.size(); ++i) {
        const QString fileName = files.at(i);
        if (Document::Ptr doc = snapshot.document(files.at(i))) {
            QBitArray bitmap(files.size());
            QList<int> directIncludes;
            const QStringList documentIncludes = doc->includedFiles();
            includesPerFile.insert(fileName, documentIncludes);

            foreach (const QString &includedFile, documentIncludes) {
                int index = fileIndex.value(includedFile);

                if (index == -1)
                    continue;
                else if (! directIncludes.contains(index))
                    directIncludes.append(index);

                bitmap.setBit(index, true);
            }

            includeMap[i] = bitmap;
            includes[i] = directIncludes;
        }
    }

    bool changed;

    do {
        changed = false;

        for (int i = 0; i < files.size(); ++i) {
            QBitArray bitmap = includeMap.value(i);
            QBitArray previousBitmap = bitmap;

            foreach (int includedFileIndex, includes.value(i)) {
                bitmap |= includeMap.value(includedFileIndex);
            }

            if (bitmap != previousBitmap) {
                includeMap[i] = bitmap;
                changed = true;
            }
        }
    } while (changed);
}
