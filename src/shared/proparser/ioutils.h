// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmake_global.h"

#include <qstring.h>

QT_BEGIN_NAMESPACE

namespace QMakeInternal {

/*!
  This class provides replacement functionality for QFileInfo, QFile & QDir,
  as these are abysmally slow.
*/
class QMAKE_EXPORT IoUtils {
public:
    enum FileType {
        FileNotFound = 0,
        FileIsRegular = 1,
        FileIsDir = 2
    };

    static FileType fileType(const QString &device, const QString &fileName);
    static bool exists(const QString &device, const QString &fileName) { return fileType(device, fileName) != FileNotFound; }
    static bool isRelativePath(const QString &device, const QString &fileName);
    static bool isAbsolutePath(const QString &device, const QString &fileName) { return !isRelativePath(device, fileName); }
    static QStringView pathName(const QString &fileName); // Requires normalized path
    static QStringView fileName(const QString &fileName); // Requires normalized path
    static QString resolvePath(const QString &device, const QString &baseDir, const QString &fileName);
    static QString shellQuoteUnix(const QString &arg);
    static QString shellQuoteWin(const QString &arg);
    static QString shellQuote(const QString &arg)
#ifdef Q_OS_UNIX
        { return shellQuoteUnix(arg); }
#else
        { return shellQuoteWin(arg); }
#endif
#if defined(PROEVALUATOR_FULL)
    static bool touchFile(const QString &targetFileName, const QString &referenceFileName, QString *errorString);
# if defined(QT_BUILD_QMAKE) && defined(Q_OS_UNIX)
    static bool readLinkTarget(const QString &symlinkPath, QString *target);
# endif
#endif
};

} // namespace ProFileEvaluatorInternal

QT_END_NAMESPACE
