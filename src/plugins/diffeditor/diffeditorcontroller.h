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

#ifndef DIFFEDITORCONTROLLER_H
#define DIFFEDITORCONTROLLER_H

#include "diffeditor_global.h"
#include "diffutils.h"

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Core { class IDocument; }

namespace DiffEditor {

namespace Internal { class DiffEditorDocument; }

class DIFFEDITOR_EXPORT DiffEditorController : public QObject
{
    Q_OBJECT
public:
    explicit DiffEditorController(Core::IDocument *document);

    void requestReload();
    bool isReloading() const;

    QString baseDirectory() const;
    int contextLineCount() const;
    bool ignoreWhitespace() const;

    QString revisionFromDescription() const;

    QString makePatch(bool revert, bool addPrefix = false) const;

    static Core::IDocument *findOrCreateDocument(const QString &vcsId, const QString &displayName);
    static DiffEditorController *controller(Core::IDocument *document);

public slots:
    void informationForCommitReceived(const QString &output);

signals:
    void chunkActionsRequested(QMenu *menu, bool isValid);
    void requestInformationForCommit(const QString &revision);

protected:
    // reloadFinished() should be called
    // inside reload() (for synchronous reload)
    // or later (for asynchronous reload)
    virtual void reload() = 0;
    virtual void reloadFinished(bool success);

    void setDiffFiles(const QList<FileData> &diffFileList,
                      const QString &baseDirectory = QString());
    void setDescription(const QString &description);
    void forceContextLineCount(int lines);

private:
    void requestMoreInformation();
    void requestChunkActions(QMenu *menu, int diffFileIndex, int chunkIndex);

    QString prepareBranchesForCommit(const QString &output);

    Internal::DiffEditorDocument *const m_document;

    bool m_isReloading;
    int m_diffFileIndex;
    int m_chunkIndex;

    friend class Internal::DiffEditorDocument;
};

} // namespace DiffEditor

#endif // DIFFEDITORCONTROLLER_H
