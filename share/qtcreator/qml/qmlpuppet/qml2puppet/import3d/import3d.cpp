/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "import3d.h"

#ifdef IMPORT_QUICK3D_ASSETS
#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>
#endif

#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QTimer>
#include <QScopedPointer>
#include <QDebug>

namespace Import3D
{

void import3D(const QString &sourceAsset, const QString &outDir, int exitId, const QString &options)
{
#ifdef IMPORT_QUICK3D_ASSETS
    QScopedPointer importer {new QSSGAssetImportManager};

    QJsonParseError error;
    QJsonDocument optDoc = QJsonDocument::fromJson(options.toUtf8(), &error);

    if (!optDoc.isNull() && optDoc.isObject()) {
        QString errorStr;
        QJsonObject optObj = optDoc.object();
        if (importer->importFile(sourceAsset, outDir, optObj.toVariantMap(), &errorStr)
                != QSSGAssetImportManager::ImportState::Success) {
            qWarning() << __FUNCTION__ << "Failed to import 3D asset"
                       << sourceAsset << "with error:" << errorStr;
        } else {
            // Allow little time for file operations to finish
            QTimer::singleShot(2000, nullptr, [exitId]() {
                qApp->exit(exitId);
            });
            return;
        }
    } else {
        qWarning() << __FUNCTION__ << "Failed to parse import options:" << error.errorString();
    }
#else
    Q_UNUSED(sourceAsset)
    Q_UNUSED(outDir)
    Q_UNUSED(exitId)
    Q_UNUSED(options)
    qWarning() << __FUNCTION__ << "Failed to parse import options, Quick3DAssetImport not available";
#endif
    QTimer::singleShot(0, nullptr, [exitId]() {
        // Negative exitId means import failure
        qApp->exit(-exitId);
    });
}

}
