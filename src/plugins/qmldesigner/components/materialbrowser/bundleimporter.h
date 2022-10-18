/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <utils/filepath.h>

#include "nodemetainfo.h"

#include <QTimer>
#include <QVariantHash>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

namespace QmlDesigner::Internal {

class BundleImporter : public QObject
{
    Q_OBJECT

public:
    BundleImporter(const QString &bundleDir,
                   const QString &bundleId,
                   const QStringList &sharedFiles,
                   QObject *parent = nullptr);
    ~BundleImporter() = default;

    QString importComponent(const QString &qmlFile,
                            const QStringList &files);
    QString unimportComponent(const QString &qmlFile);
    Utils::FilePath resolveBundleImportPath();

signals:
    // The metaInfo parameter will be invalid if an error was encountered during
    // asynchronous part of the import. In this case all remaining pending imports have been
    // terminated, and will not receive separate importFinished notifications.
    void importFinished(const QmlDesigner::NodeMetaInfo &metaInfo);
    void unimportFinished(const QmlDesigner::NodeMetaInfo &metaInfo);

private:
    void handleImportTimer();
    QVariantHash loadAssetRefMap(const Utils::FilePath &bundlePath);
    void writeAssetRefMap(const Utils::FilePath &bundlePath, const QVariantHash &assetRefMap);

    Utils::FilePath m_bundleDir;
    QString m_bundleId;
    QString m_moduleName;
    QStringList m_sharedFiles;
    QTimer m_importTimer;
    int m_importTimerCount = 0;
    bool m_importAddPending = false;
    bool m_fullReset = false;
    QHash<QString, bool> m_pendingTypes; // <type, isImport>
};

} // namespace QmlDesigner::Internal
