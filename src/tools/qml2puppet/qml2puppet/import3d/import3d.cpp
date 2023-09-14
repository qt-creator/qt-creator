// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

void import3D([[maybe_unused]] const QString &sourceAsset,
              [[maybe_unused]] const QString &outDir,
              [[maybe_unused]] const QString &options)
{
    QString errorStr;
#ifdef IMPORT_QUICK3D_ASSETS
    QScopedPointer importer {new QSSGAssetImportManager};

    QJsonParseError error;
    QJsonDocument optDoc = QJsonDocument::fromJson(options.toUtf8(), &error);

    if (!optDoc.isNull() && optDoc.isObject()) {
        QJsonObject optObj = optDoc.object();
        const auto &optionsMap = optObj;
        if (importer->importFile(sourceAsset, outDir, optionsMap, &errorStr)
                != QSSGAssetImportManager::ImportState::Success) {
        }
    } else {
        errorStr = QObject::tr("Failed to parse import options: %1").arg(error.errorString());
    }
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

    QTimer::singleShot(0, nullptr, []() {
        qApp->quit();
    });
}

}
