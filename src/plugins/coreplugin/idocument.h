// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/filepath.h>
#include <utils/id.h>

#include <QObject>

namespace Utils {
class InfoBar;
class MinimizableInfoBars;
} // namespace Utils

namespace Core {

namespace Internal {
class IDocumentPrivate;
}

class CORE_EXPORT IDocument : public QObject
{
    Q_OBJECT

public:
    enum class OpenResult {
        Success,
        ReadError,
        CannotHandle
    };

    // This enum must match the indexes of the reloadBehavior widget
    // in generalsettings.ui
    enum ReloadSetting {
        AlwaysAsk = 0,
        ReloadUnmodified = 1,
        IgnoreAll = 2
    };

    enum ChangeTrigger {
        TriggerInternal,
        TriggerExternal
    };

    enum ChangeType {
        TypeContents,
        TypeRemoved
    };

    enum ReloadBehavior {
        BehaviorAsk,
        BehaviorSilent
    };

    enum ReloadFlag {
        FlagReload,
        FlagIgnore
    };

    IDocument(QObject *parent = nullptr);
    ~IDocument() override;

    void setId(Utils::Id id);
    Utils::Id id() const;

    virtual OpenResult open(QString *errorString, const Utils::FilePath &filePath, const Utils::FilePath &realFilePath);

    bool save(QString *errorString, const Utils::FilePath &filePath = Utils::FilePath(), bool autoSave = false);

    virtual QByteArray contents() const;
    virtual bool setContents(const QByteArray &contents);
    virtual void formatContents();

    const Utils::FilePath &filePath() const;
    virtual void setFilePath(const Utils::FilePath &filePath);
    QString displayName() const;
    void setPreferredDisplayName(const QString &name);
    QString preferredDisplayName() const;
    QString plainDisplayName() const;
    void setUniqueDisplayName(const QString &name);
    QString uniqueDisplayName() const;

    bool isFileReadOnly() const;
    bool isTemporary() const;
    void setTemporary(bool temporary);

    virtual Utils::FilePath fallbackSaveAsPath() const;
    virtual QString fallbackSaveAsFileName() const;

    QString mimeType() const;
    void setMimeType(const QString &mimeType);

    virtual bool shouldAutoSave() const;
    virtual bool isModified() const;
    virtual bool isSaveAsAllowed() const;
    bool isSuspendAllowed() const;
    void setSuspendAllowed(bool value);

    virtual ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    virtual bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

    void checkPermissions();

    bool autoSave(QString *errorString, const Utils::FilePath &filePath);
    void setRestoredFrom(const Utils::FilePath &path);
    void removeAutoSaveFile();

    bool hasWriteWarning() const;
    void setWriteWarning(bool has);

    Utils::InfoBar *infoBar();
    Utils::MinimizableInfoBars *minimizableInfoBars();

signals:
    // For meta data changes: file name, modified state, ...
    void changed();

    // For changes in the contents of the document
    void contentsChanged();

    void mimeTypeChanged();

    void aboutToReload();
    void reloadFinished(bool success);
    void aboutToSave(const Utils::FilePath &filePath, bool autoSave);
    void saved(const Utils::FilePath &filePath, bool autoSave);

    void filePathChanged(const Utils::FilePath &oldName, const Utils::FilePath &newName);

protected:
    virtual bool saveImpl(QString *errorString,
                          const Utils::FilePath &filePath = Utils::FilePath(),
                          bool autoSave = false);

private:
    Internal::IDocumentPrivate *d;
};

} // namespace Core
