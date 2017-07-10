/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "vcsbase_global.h"
#include <diffeditor/diffeditorcontroller.h>

namespace Core { class IDocument; }

namespace VcsBase {

class VcsBaseClientImpl;
class VcsBaseDiffEditorControllerPrivate;

class VCSBASE_EXPORT VcsBaseDiffEditorController : public DiffEditor::DiffEditorController
{
    Q_OBJECT

public:
    VcsBaseDiffEditorController(Core::IDocument *document,
                                VcsBaseClientImpl *client,
                                const QString &workingDirectory);
    ~VcsBaseDiffEditorController();

protected:
    void runCommand(const QList<QStringList> &args, unsigned flags, QTextCodec *codec = nullptr);
    virtual void processCommandOutput(const QString &output);

    VcsBaseClientImpl *client() const;
    QString workingDirectory() const;
    void setStartupFile(const QString &startupFile);
    QString startupFile() const;

private:
    friend class VcsBaseDiffEditorControllerPrivate;
    VcsBaseDiffEditorControllerPrivate *d;
};

} //namespace VcsBase
