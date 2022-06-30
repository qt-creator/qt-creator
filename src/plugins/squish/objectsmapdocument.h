/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include <coreplugin/idocument.h>

#include <QList>

namespace Squish {
namespace Internal {

class ObjectsMapModel;

class ObjectsMapDocument : public Core::IDocument
{
    Q_OBJECT
public:
    ObjectsMapDocument();

    OpenResult open(QString *errorString,
                    const Utils::FilePath &fileName,
                    const Utils::FilePath &realFileName) override;
    bool save(QString *errorString, const Utils::FilePath &fileName, bool autoSave) override;
    Utils::FilePath fallbackSaveAsPath() const override;
    QString fallbackSaveAsFileName() const override;
    bool isModified() const override { return m_isModified; }
    void setModified(bool modified);
    bool isSaveAsAllowed() const override { return true; }
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

    bool shouldAutoSave() const override { return true; }
    bool setContents(const QByteArray &contents) override;
    QByteArray contents() const override;
    ObjectsMapModel *model() const { return m_contentModel; }

private:
    OpenResult openImpl(QString *error,
                        const Utils::FilePath &fileName,
                        const Utils::FilePath &realFileName);
    bool buildObjectsMapTree(const QByteArray &contents);
    bool writeFile(const Utils::FilePath &fileName) const;
    void syncXMLFromEditor();

    ObjectsMapModel *m_contentModel;
    bool m_isModified;
};

} // namespace Internal
} // namespace Squish
