/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "core_global.h"

#include <QObject>

namespace Utils { class FileName; }

namespace Core {
class Id;
class InfoBar;

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
        TypePermissions,
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

    IDocument(QObject *parent = 0);
    ~IDocument() override;

    void setId(Id id);
    Id id() const;

    // required to be re-implemented for documents of IEditors
    virtual OpenResult open(QString *errorString, const QString &fileName, const QString &realFileName);

    virtual bool save(QString *errorString, const QString &fileName = QString(), bool autoSave = false);

    virtual QByteArray contents() const;
    virtual bool setContents(const QByteArray &contents);

    const Utils::FileName &filePath() const;
    virtual void setFilePath(const Utils::FileName &filePath);
    QString displayName() const;
    void setPreferredDisplayName(const QString &name);
    QString preferredDisplayName() const;
    QString plainDisplayName() const;
    void setUniqueDisplayName(const QString &name);
    QString uniqueDisplayName() const;

    virtual bool isFileReadOnly() const;
    bool isTemporary() const;
    void setTemporary(bool temporary);

    virtual QString fallbackSaveAsPath() const;
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

    virtual void checkPermissions();

    bool autoSave(QString *errorString, const QString &filePath);
    void setRestoredFrom(const QString &name);
    void removeAutoSaveFile();

    bool hasWriteWarning() const;
    void setWriteWarning(bool has);

    InfoBar *infoBar();

signals:
    // For meta data changes: file name, modified state, ...
    void changed();

    // For changes in the contents of the document
    void contentsChanged();

    void mimeTypeChanged();

    void aboutToReload();
    void reloadFinished(bool success);

    void filePathChanged(const Utils::FileName &oldName, const Utils::FileName &newName);

private:
    Internal::IDocumentPrivate *d;
};

} // namespace Core
