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

#include "vcsbasesubmiteditor.h"

#include <coreplugin/idocument.h>

namespace VcsBase {
class VcsBaseSubmitEditor;

namespace Internal {

class SubmitEditorFile : public Core::IDocument
{
public:
    explicit SubmitEditorFile(const VcsBaseSubmitEditorParameters *parameters,
                              VcsBaseSubmitEditor *parent = 0);

    OpenResult open(QString *errorString, const QString &fileName,
                    const QString &realFileName) override;
    QByteArray contents() const override;
    bool setContents(const QByteArray &contents) override;

    bool isModified() const override { return m_modified; }
    bool save(QString *errorString, const QString &fileName, bool autoSave) override;
    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;

    void setModified(bool modified = true);

private:
    bool m_modified;
    VcsBaseSubmitEditor *m_editor;
};

} // namespace Internal
} // namespace VcsBase
