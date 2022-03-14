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

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopedPointer>
#include <QTextStream>
#include <QTimer>

namespace Import3D
{

void import3D(const QString &sourceAsset, const QString &outDir, const QString &options)
{
    QString errorStr;
#ifdef IMPORT_QUICK3D_ASSETS
    QScopedPointer importer {new QSSGAssetImportManager};

    QJsonParseError error;
    QJsonDocument optDoc = QJsonDocument::fromJson(options.toUtf8(), &error);

    if (!optDoc.isNull() && optDoc.isObject()) {
        QJsonObject optObj = optDoc.object();
        if (importer->importFile(sourceAsset, outDir, optObj.toVariantMap(), &errorStr)
                != QSSGAssetImportManager::ImportState::Success) {
        }
    } else {
        errorStr = QObject::tr("Failed to parse import options: %1").arg(error.errorString());
    }
#else
    errorStr = QObject::tr("QtQuick3D is not available.");
    Q_UNUSED(sourceAsset)
    Q_UNUSED(outDir)
    Q_UNUSED(options)
#endif
    if (!errorStr.isEmpty()) {
        qWarning() << __FUNCTION__ << "Failed to import asset:" << errorStr << outDir;

        // Write the error into a file in outDir to pass it to creator side
        QString errorFileName = outDir + "/__error.log";
        QFile file(errorFileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream out(&file);
            out << errorStr;
            file.close();
        }
    }

    // Allow little time for file operations to finish
    QTimer::singleShot(2000, nullptr, []() {
        qApp->exit(0);
    });
}

}
