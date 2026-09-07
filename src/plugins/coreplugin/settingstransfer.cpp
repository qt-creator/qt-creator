// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingstransfer.h"

#include "coreplugintr.h"
#include "documentmanager.h"
#include "icore.h"

#include <utils/aspects.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/store.h>

#include <QGuiApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>

using namespace Utils;

namespace Core {

const char kType[] = "type";
const char kVersion[] = "version";
const char kSettings[] = "settings";

const int currentVersion = 1;

static QString fileFilter()
{
    return Tr::tr("Settings File (*.json)") + ";;" + DocumentManager::allFilesFilterString();
}

/*!
    Writes the settings of \a transfer to \a filePath as JSON.

    The pending values are written, so that what a preferences page shows is
    what ends up in the file, and only those differing from their default,
    which is what lets an import reproduce the exported state exactly.
*/
Result<> exportSettings(const SettingsTransfer &transfer, const FilePath &filePath)
{
    QTC_ASSERT(transfer.container, return ResultError(QString("No settings to export.")));

    Store settings;
    transfer.container->volatileToMap(settings);

    Store document;
    document.insert(kType, transfer.type.toSetting());
    document.insert(kVersion, currentVersion);
    document.insert(kSettings, Utils::variantFromStore(settings));

    const Result<qint64> written = filePath.writeFileContents(Utils::jsonFromStore(document));
    if (!written)
        return ResultError(written.error());
    return ResultOk;
}

/*!
    Reads a file written by exportSettings() from \a filePath into the pending
    values of \a transfer, and reports how many of its settings exist in the
    container and how many do not.

    Settings the file does not mention are reset to their default, so that the
    file describes the whole state rather than a set of changes. The values
    only become effective once the container is applied, which for a
    preferences page means that Apply is enabled and Cancel still reverts.
*/
Result<SettingsImport> importSettings(const SettingsTransfer &transfer, const FilePath &filePath)
{
    QTC_ASSERT(transfer.container, return ResultError(QString("No settings to import.")));

    const Result<QByteArray> contents = filePath.fileContents();
    if (!contents)
        return ResultError(contents.error());

    const Result<Store> document = Utils::storeFromJson(*contents);
    if (!document)
        return ResultError(document.error());

    const Id type = Id::fromSetting(document->value(kType));
    if (type != transfer.type) {
        return ResultError(
            Tr::tr("The file holds \"%1\" settings, which do not belong to \"%2\".")
                .arg(type.isValid() ? type.toString() : Tr::tr("unknown"))
                .arg(transfer.type.toString()));
    }

    const int version = document->value(kVersion).toInt();
    if (version > currentVersion) {
        return ResultError(
            Tr::tr("The file was written by a newer version of %1.")
                .arg(QGuiApplication::applicationDisplayName()));
    }

    const Store settings = Utils::storeFromVariant(document->value(kSettings));
    transfer.container->volatileFromMap(settings);

    QSet<Key> known;
    transfer.container->forEachAspect([&known](BaseAspect *aspect) {
        if (!aspect->settingsKey().isEmpty())
            known.insert(aspect->settingsKey());
    });

    SettingsImport imported;
    for (auto it = settings.cbegin(), end = settings.cend(); it != end; ++it) {
        if (known.contains(it.key()))
            ++imported.applied;
        else
            ++imported.ignored;
    }
    return imported;
}

/*!
    Returns \uicontrol Export and \uicontrol Import buttons for \a transfer,
    for a preferences page to add to its layout.
*/
Layouting::Layout settingsTransferButtons(const SettingsTransfer &transfer)
{
    using namespace Layouting;

    auto exportButton = new QPushButton(Tr::tr("Export..."));
    exportButton->setToolTip(Tr::tr("Writes these settings to a file."));

    auto importButton = new QPushButton(Tr::tr("Import..."));
    importButton->setToolTip(Tr::tr("Replaces these settings with the contents of a file."));

    QObject::connect(exportButton, &QPushButton::clicked, transfer.container, [transfer] {
        const FilePath filePath = DocumentManager::getSaveFileNameWithExtension(
            Tr::tr("Export %1").arg(transfer.displayName),
            DocumentManager::fileDialogInitialDirectory() / (transfer.fileNameBase + ".json"),
            fileFilter());
        if (filePath.isEmpty())
            return;

        if (const Result<> result = exportSettings(transfer, filePath); !result) {
            QMessageBox::critical(
                ICore::dialogParent(),
                Tr::tr("Export Failed"),
                Tr::tr("Cannot export to %1: %2")
                    .arg(filePath.toUserOutput())
                    .arg(result.error()));
        }
    });

    QObject::connect(importButton, &QPushButton::clicked, transfer.container, [transfer] {
        const FilePath filePath = FileUtils::getOpenFilePath(
            Tr::tr("Import %1").arg(transfer.displayName),
            DocumentManager::fileDialogInitialDirectory(),
            fileFilter());
        if (filePath.isEmpty())
            return;

        const Result<SettingsImport> imported = importSettings(transfer, filePath);
        if (!imported) {
            QMessageBox::critical(
                ICore::dialogParent(),
                Tr::tr("Import Failed"),
                Tr::tr("Cannot import %1: %2")
                    .arg(filePath.toUserOutput())
                    .arg(imported.error()));
        } else if (imported->applied == 0 && imported->ignored > 0) {
            QMessageBox::warning(
                ICore::dialogParent(),
                Tr::tr("Nothing Imported"),
                Tr::tr("None of the settings in %1 exist here, so everything was reset to its "
                       "default. The file may have been written by a different version of %2.")
                    .arg(filePath.toUserOutput())
                    .arg(QGuiApplication::applicationDisplayName()));
        }
    });

    return Row{noMargin, exportButton, importButton};
}

} // namespace Core
