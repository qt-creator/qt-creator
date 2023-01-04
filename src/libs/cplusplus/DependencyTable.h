// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CPlusPlusForwardDeclarations.h>

#include <utils/fileutils.h>

#include <QBitArray>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

QT_BEGIN_NAMESPACE
class QFutureInterfaceBase;
QT_END_NAMESPACE

namespace CPlusPlus {

class Snapshot;

class CPLUSPLUS_EXPORT DependencyTable
{
private:
    friend class Snapshot;
    void build(QFutureInterfaceBase &futureInterface, const Snapshot &snapshot);
    Utils::FilePaths filesDependingOn(const Utils::FilePath &fileName) const;

    QVector<Utils::FilePath> files;
    QHash<Utils::FilePath, int> fileIndex;
    QHash<int, QList<int> > includes;
    QVector<QBitArray> includeMap;
};

} // namespace CPlusPlus
