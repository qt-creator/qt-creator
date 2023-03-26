// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nativefilegenerator.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

namespace MesonProjectManager {
namespace Internal {

NativeFileGenerator::NativeFileGenerator() {}

inline void addEntry(QIODevice *nativeFile, const QString &key, const QString &value)
{
    nativeFile->write(QString("%1 = '%2'\n").arg(key).arg(value).toUtf8());
}

void writeBinariesSection(QIODevice *nativeFile, const KitData &kitData)
{
    nativeFile->write("[binaries]\n");
    addEntry(nativeFile, "c", kitData.cCompilerPath);
    addEntry(nativeFile, "cpp", kitData.cxxCompilerPath);
    addEntry(nativeFile, "qmake", kitData.qmakePath);
    if (kitData.qtVersion == Utils::QtMajorVersion::Qt4)
        addEntry(nativeFile, QString{"qmake-qt4"}, kitData.qmakePath);
    else if (kitData.qtVersion == Utils::QtMajorVersion::Qt5)
        addEntry(nativeFile, QString{"qmake-qt5"}, kitData.qmakePath);
    else if (kitData.qtVersion == Utils::QtMajorVersion::Qt6)
        addEntry(nativeFile, QString{"qmake-qt6"}, kitData.qmakePath);
    addEntry(nativeFile, "cmake", kitData.cmakePath);
}

void NativeFileGenerator::makeNativeFile(QIODevice *nativeFile, const KitData &kitData)
{
    QTC_ASSERT(nativeFile, return );
    writeBinariesSection(nativeFile, kitData);
}

} // namespace Internal
} // namespace MesonProjectManager
