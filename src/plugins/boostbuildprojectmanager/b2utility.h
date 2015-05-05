//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#ifndef BBUTILITY_HPP
#define BBUTILITY_HPP

#include "b2projectmanagerconstants.h"
// Qt
#include <QDebug>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

//////////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG

#define BBPM_QDEBUG(msg) \
    qDebug() \
        << "[" << BoostBuildProjectManager::Constants::BOOSTBUILD << "] " \
        << "(" << __PRETTY_FUNCTION__ << ")"; \
    qDebug().nospace() << "\t" << msg

#else
#define BBPM_QDEBUG(msg)

#endif // _DEBUG

#define BBPM_C(CONSTANT) QLatin1String(BoostBuildProjectManager::Constants::CONSTANT)

//////////////////////////////////////////////////////////////////////////////////////////
namespace BoostBuildProjectManager {
namespace Utility {

// Read all lines from a file.
QStringList readLines(QString const& absoluteFileName);

// Converts the path from relative to the project to an absolute path.
QStringList makeAbsolutePaths(QString const& basePath, QStringList const& paths);

QStringList& makeRelativePaths(QString const& basePath, QStringList& paths);

QHash<QString, QStringList> sortFilesIntoPaths(QString const& basePath
                                             , QSet<QString> const& files);

QString parseJamfileProjectName(QString const& fileName);

} // namespace Utility
} // namespace BoostBuildProjectManager

#endif // BBUTILITY_HPP
