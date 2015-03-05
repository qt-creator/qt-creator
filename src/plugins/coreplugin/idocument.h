/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IDOCUMENT_H
#define IDOCUMENT_H

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
    virtual ~IDocument();

    void setId(Id id);
    Id id() const;

    virtual bool save(QString *errorString, const QString &fileName = QString(), bool autoSave = false) = 0;
    virtual bool setContents(const QByteArray &contents);

    const Utils::FileName &filePath() const;
    virtual void setFilePath(const Utils::FileName &filePath);
    QString displayName() const;
    void setPreferredDisplayName(const QString &name);
    QString plainDisplayName() const;
    void setUniqueDisplayName(const QString &name);

    virtual bool isFileReadOnly() const;
    bool isTemporary() const;
    void setTemporary(bool temporary);

    virtual QString defaultPath() const = 0;
    virtual QString suggestedFileName() const = 0;

    QString mimeType() const;
    void setMimeType(const QString &mimeType);

    virtual bool shouldAutoSave() const;
    virtual bool isModified() const = 0;
    virtual bool isSaveAsAllowed() const = 0;

    virtual ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    virtual bool reload(QString *errorString, ReloadFlag flag, ChangeType type) = 0;

    virtual void checkPermissions();

    bool autoSave(QString *errorString, const QString &filePath);
    void setRestoredFrom(const QString &name);
    void removeAutoSaveFile();

    bool hasWriteWarning() const;
    void setWriteWarning(bool has);

    InfoBar *infoBar();

signals:
    void changed();
    void mimeTypeChanged();

    void aboutToReload();
    void reloadFinished(bool success);

    void filePathChanged(const Utils::FileName &oldName, const Utils::FileName &newName);

private:
    Internal::IDocumentPrivate *d;
};

} // namespace Core

#endif // IDOCUMENT_H
