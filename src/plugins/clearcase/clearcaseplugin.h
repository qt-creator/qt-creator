// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

#include <QFile>
#include <QPair>
#include <QStringList>
#include <QSharedPointer>

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

class ClearCasePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClearCase.json")

    ~ClearCasePlugin() final;

public:
    static bool newActivity();
    static const QList<QStringPair> activities(int *current);
    static QStringList ccGetActiveVobs();
    static void refreshActivities();
    static const ViewData viewData();
    static void setStatus(const QString &file, FileStatus::Status status, bool update);
    static const ClearCaseSettings &settings();
    static void setSettings(const ClearCaseSettings &s);
    static QSharedPointer<StatusMap> statusMap();

private:
    void initialize() final;
    void extensionsInitialized() final;

#ifdef WITH_TESTS
private slots:
    void initTestCase();
    void cleanupTestCase();
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
    void testFileStatusParsing_data();
    void testFileStatusParsing();
    void testFileNotManaged();
    void testFileCheckedOutDynamicView();
    void testFileCheckedInDynamicView();
    void testFileNotManagedDynamicView();
    void testStatusActions_data();
    void testStatusActions();
    void testVcsStatusDynamicReadonlyNotManaged();
    void testVcsStatusDynamicNotManaged();
#endif
};

} // ClearCase::Internal
