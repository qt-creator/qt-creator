// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/result.h>

#include <QMap>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QObject)

namespace QtSupport::Internal {

// The properties a "qmake -query" would report, read from the files a Qt installation
// describes itself with. Keys and values are the qmake property names and values, e.g.
// "QT_INSTALL_BINS" -> "/opt/qt-6.13.0/bin".
Utils::Result<QMap<QString, QString>> qtPropertiesFromPrefix(const Utils::FilePath &prefix);

// The "[Paths]" section of a qt.conf or target_qt.conf, verbatim.
QMap<QString, QString> qtConfPaths(const QString &contents);

#ifdef WITH_TESTS
QObject *createQtVersionFromFilesTest();
#endif

} // namespace QtSupport::Internal
