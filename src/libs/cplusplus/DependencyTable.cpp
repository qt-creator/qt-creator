// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "CppDocument.h"

#include <QDebug>
#include <QFuture>

using namespace Utils;

namespace CPlusPlus {

Utils::FilePaths DependencyTable::filesDependingOn(const Utils::FilePath &fileName) const
{
    Utils::FilePaths deps;

    int index = fileIndex.value(fileName, -1);
    if (index == -1)
        return deps;

    for (int i = 0; i < files.size(); ++i) {
        const QBitArray &bits = includeMap.at(i);

        if (bits.testBit(index))
            deps.append(files.at(i));
    }

    return deps;
}

void DependencyTable::build(const std::optional<QFuture<void>> &future, const Snapshot &snapshot)
{
    files.clear();
    fileIndex.clear();
    includes.clear();
    includeMap.clear();

    if (future && future->isCanceled())
        return;

    const int documentCount = snapshot.size();
    files.resize(documentCount);
    includeMap.resize(documentCount);

    int i = 0;
    for (Snapshot::const_iterator it = snapshot.begin(); it != snapshot.end();
            ++it, ++i) {
        files[i] = it.key();
        fileIndex[it.key()] = i;
    }

    if (future && future->isCanceled())
        return;

    for (int i = 0; i < files.size(); ++i) {
        const Utils::FilePath &fileName = files.at(i);
        if (Document::Ptr doc = snapshot.document(fileName)) {
            QBitArray bitmap(files.size());
            QList<int> directIncludes;
            const FilePaths documentIncludes = doc->includedFiles();

            for (const FilePath &includedFile : documentIncludes) {
                int index = fileIndex.value(includedFile);

                if (index == -1)
                    continue;
                else if (! directIncludes.contains(index))
                    directIncludes.append(index);

                bitmap.setBit(index, true);
                if (future && future->isCanceled())
                    return;
            }

            includeMap[i] = bitmap;
            includes[i] = directIncludes;
            if (future && future->isCanceled())
                return;
        }
    }

    bool changed;

    do {
        changed = false;

        for (int i = 0; i < files.size(); ++i) {
            QBitArray bitmap = includeMap.value(i);
            QBitArray previousBitmap = bitmap;

            const QList<int> includedFileIndexes = includes.value(i);
            for (const int includedFileIndex : includedFileIndexes) {
                bitmap |= includeMap.value(includedFileIndex);
                if (future && future->isCanceled())
                    return;
            }

            if (bitmap != previousBitmap) {
                includeMap[i] = bitmap;
                changed = true;
            }
            if (future && future->isCanceled())
                return;
        }
        if (future && future->isCanceled())
            return;
    } while (changed);
}

} // CPlusPlus
