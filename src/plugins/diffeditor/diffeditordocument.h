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

#ifndef DIFFEDITORDOCUMENT_H
#define DIFFEDITORDOCUMENT_H

#include "diffutils.h"

#include <coreplugin/textdocument.h>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace DiffEditor {

class DiffEditorController;

namespace Internal {

class DiffEditorDocument : public Core::BaseTextDocument
{
    Q_OBJECT
    Q_PROPERTY(QString plainText READ plainText STORED false) // For access by code pasters
public:
    DiffEditorDocument();

    DiffEditorController *controller() const;

    QString makePatch(int fileIndex, int chunkIndex, bool revert, bool addPrefix = false) const;

    void setDiffFiles(const QList<FileData> &data, const QString &directory);
    QList<FileData> diffFiles() const;
    QString baseDirectory() const;

    void setDescription(const QString &description);
    QString description() const;

    void setContextLineCount(int lines);
    int contextLineCount() const;
    void forceContextLineCount(int lines);
    bool isContextLineCountForced() const;
    void setIgnoreWhitespace(bool ignore);
    bool ignoreWhitespace() const;

    bool setContents(const QByteArray &contents) override;
    QString defaultPath() const override;
    QString suggestedFileName() const override;

    bool isModified() const override { return false; }
    bool isSaveAsAllowed() const override { return true; }
    bool save(QString *errorString, const QString &fileName, bool autoSave) override;
    void reload();
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    OpenResult open(QString *errorString, const QString &fileName,
                    const QString &realFileName) override;

    QString plainText() const;

signals:
    void temporaryStateChanged();
    void documentChanged();
    void descriptionChanged();
    void chunkActionsRequested(QMenu *menu, int diffFileIndex, int chunkIndex);
    void requestMoreInformation();

public slots:
    void beginReload();
    void endReload(bool success);

private:
    void setController(DiffEditorController *controller);

    DiffEditorController *m_controller;
    QList<FileData> m_diffFiles;
    QString m_baseDirectory;
    QString m_description;
    int m_contextLineCount;
    bool m_isContextLineCountForced;
    bool m_ignoreWhitespace;

    friend class ::DiffEditor::DiffEditorController;
};

} // namespace Internal
} // namespace DiffEditor

#endif // DIFFEDITORDOCUMENT_H
