// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

#include <QFile>
#include <QPair>
#include <QStringList>

namespace ClearCase::Internal {

class ClearCaseSettings;

using QStringPair = QPair<QString, QString>;

class FileStatus
{
public:
    enum Status
    {
        Unknown    = 0x0f,
        CheckedIn  = 0x01,
        CheckedOut = 0x02,
        Hijacked   = 0x04,
        NotManaged = 0x08,
        Missing    = 0x10,
        Derived    = 0x20
    } status;

    QFile::Permissions permissions;

    FileStatus(Status _status = Unknown, QFile::Permissions perm = {})
        : status(_status), permissions(perm)
    { }
};

using StatusMap = QHash<QString, FileStatus>;

class ViewData
{
public:
    QString name;
    bool isDynamic = false;
    bool isUcm = false;
    QString root;
};

bool newActivity();
const QList<QStringPair> activities(int *current);
QStringList ccGetActiveVobs();
void refreshActivities();
const ViewData viewData();
void setStatus(const QString &file, FileStatus::Status status, bool update);
const ClearCaseSettings &settings();
void setSettings(const ClearCaseSettings &s);
std::shared_ptr<StatusMap> statusMap();

} // ClearCase::Internal
