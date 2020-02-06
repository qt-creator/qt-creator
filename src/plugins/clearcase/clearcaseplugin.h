/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "clearcasesettings.h"

#include <extensionsystem/iplugin.h>

#include <QFile>
#include <QPair>
#include <QStringList>
#include <QMetaType>
#include <QSharedPointer>

namespace ClearCase {
namespace Internal {

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
    bool initialize(const QStringList &arguments, QString *error_message) final;
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

} // namespace Internal
} // namespace ClearCase
