// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "itemlibraryassetimporter.h"
#include "modelnode.h"

#include <QDialog>
#include <QJsonObject>
#include <QSet>

QT_BEGIN_NAMESPACE
class QGridLayout;
QT_END_NAMESPACE

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
                                          const QString &defaulTargetDirectory,
                                          const QVariantMap &supportedExts,
                                          const QVariantMap &supportedOpts,
                                          const QJsonObject &defaultOpts,
                                          const QSet<QString> &preselectedFilesForOverwrite,
                                          QWidget *parent = nullptr);
    ~ItemLibraryAssetImportDialog();

    static void updateImport(const ModelNode &updateNode,
                             const QVariantMap &supportedExts,
                             const QVariantMap &supportedOpts);

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
    void toggleAdvanced();

    void createTab(const QString &tabLabel, int optionsIndex, const QJsonObject &groups);
    QGridLayout *createOptionsGrid(QWidget *contentWidget, bool advanced, int optionsIndex,
                                   const QJsonObject &groups);
    void updateUi();

    bool isSimpleGroup(const QString &id);
    bool isSimpleOption(const QString &id);
    bool isHiddenOption(const QString &id);

    Ui::ItemLibraryAssetImportDialog *ui = nullptr;
    Utils::OutputFormatter *m_outputFormatter = nullptr;

    struct OptionsData
    {
        int optionsRows = 0;
        int optionsHeight = 0;
        QList<QWidget *> contentWidgets; // Tab content widgets
    };

    QStringList m_quick3DFiles;
    QString m_quick3DImportPath;
    ItemLibraryAssetImporter m_importer;
    QVector<QJsonObject> m_importOptions;
    QHash<QString, int> m_extToImportOptionsMap;
    QSet<QString> m_preselectedFilesForOverwrite;
    bool m_closeOnFinish = true;
    QList<QHash<QString, QWidget *>> m_labelToControlWidgetMaps;
    OptionsData m_simpleData;
    OptionsData m_advancedData;
    bool m_advancedMode = false;
    int m_dialogHeight = 350;
};
}
