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

#ifndef SUBMITEDITORFILE_H
#define SUBMITEDITORFILE_H

#include "vcsbasesubmiteditor.h"

#include <coreplugin/idocument.h>

namespace VcsBase {
class VcsBaseSubmitEditor;

namespace Internal {

class SubmitEditorFile : public Core::IDocument
{
    Q_OBJECT
public:
    explicit SubmitEditorFile(const VcsBaseSubmitEditorParameters *parameters,
                              VcsBaseSubmitEditor *parent = 0);

    OpenResult open(QString *errorString, const QString &fileName,
                    const QString &realFileName) override;
    bool setContents(const QByteArray &contents) override;
    QString defaultPath() const override { return QString(); }
    QString suggestedFileName() const override { return QString(); }

    bool isModified() const override { return m_modified; }
    bool isSaveAsAllowed() const override { return false; }
    bool save(QString *errorString, const QString &fileName, bool autoSave) override;
    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

    void setModified(bool modified = true);

private:
    bool m_modified;
    VcsBaseSubmitEditor *m_editor;
};


} // namespace Internal
} // namespace VcsBase

#endif // SUBMITEDITORFILE_H
