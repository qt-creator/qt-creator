/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "itemlibraryassetimporter.h"

#include <QtWidgets/qdialog.h>
#include <QtCore/qjsonobject.h>

namespace Utils {
class OutputFormatter;
}

namespace QmlDesigner {
class ItemLibraryAssetImporter;

namespace Ui {
class ItemLibraryAssetImportDialog;
}

class ItemLibraryAssetImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ItemLibraryAssetImportDialog(const QStringList &importFiles,
                               const QString &defaulTargetDirectory, QWidget *parent = nullptr);
    ~ItemLibraryAssetImportDialog();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void addError(const QString &error, const QString &srcPath = {});
    void addWarning(const QString &warning, const QString &srcPath = {});
    void addInfo(const QString &info, const QString &srcPath = {});

private:
    void setCloseButtonState(bool importing);

    void onImport();
    void setImportProgress(int value, const QString &text);
    void onImportNearlyFinished();
    void onImportFinished();
    void onClose();

    void createTab(const QString &tabLabel, int optionsIndex, const QJsonObject &groups);
    void updateUi();

    Ui::ItemLibraryAssetImportDialog *ui = nullptr;
    Utils::OutputFormatter *m_outputFormatter = nullptr;

    QStringList m_quick3DFiles;
    QString m_quick3DImportPath;
    ItemLibraryAssetImporter m_importer;
    QVector<QJsonObject> m_importOptions;
    QHash<QString, int> m_extToImportOptionsMap;
    int m_optionsHeight = 0;
    int m_optionsRows = 0;
};
}
