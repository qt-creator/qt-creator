//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the  GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#include "b2project.h"
#include "b2projectfile.h"
#include "b2projectmanagerconstants.h"
#include "b2utility.h"

namespace BoostBuildProjectManager {
namespace Internal {

ProjectFile::ProjectFile(Project* project, QString const& fileName)
    : Core::IDocument(project)
    , project_(project)
{
    Q_ASSERT(!fileName.isEmpty());

    setFilePath(Utils::FileName::fromString(fileName));

    BBPM_QDEBUG(fileName);
}

bool ProjectFile::save(QString* errorString, QString const& fileName, bool autoSave)
{
    Q_UNUSED(errorString);
    Q_UNUSED(fileName);
    Q_UNUSED(autoSave);

    BBPM_QDEBUG("TODO");
    return false;
}

QString ProjectFile::defaultPath() const
{
    BBPM_QDEBUG("TODO");
    return QString();
}

QString ProjectFile::suggestedFileName() const
{
    return QString();
}

QString ProjectFile::mimeType() const
{
    BBPM_QDEBUG("TODO");
    return QLatin1String(Constants::MIMETYPE_JAMFILE);
}

bool ProjectFile::isModified() const
{
    return false;
}

bool ProjectFile::isSaveAsAllowed() const
{
    BBPM_QDEBUG("TODO");
    return false;
}

bool ProjectFile::reload(QString* errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString);
    Q_UNUSED(flag);
    Q_UNUSED(type);

    BBPM_QDEBUG("TODO");
    return false;
}

} // namespace Internal
} // namespace AutotoolsProjectManager
