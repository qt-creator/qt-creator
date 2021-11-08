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

#include <coreplugin/core_global.h>

#include <utils/id.h>
#include <utils/mimetypes/mimetype.h>

#include <QObject>
#include <QStringList>

#include <functional>

namespace Utils { class FilePath; }

namespace Core {

class IExternalEditor;
class IEditor;
class IEditorFactory;
class EditorType;

using EditorFactoryList = QList<IEditorFactory *>;
using EditorTypeList = QList<EditorType *>;

class CORE_EXPORT EditorType : public QObject
{
    Q_OBJECT
public:
    ~EditorType() override;

    static const EditorTypeList allEditorTypes();
    static EditorType *editorTypeForId(const Utils::Id &id);
    static const EditorTypeList defaultEditorTypes(const Utils::MimeType &mimeType);
    static const EditorTypeList preferredEditorTypes(const Utils::FilePath &filePath);

    Utils::Id id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    QStringList mimeTypes() const { return m_mimeTypes; }

    virtual IEditorFactory *asEditorFactory() { return nullptr; };
    virtual IExternalEditor *asExternalEditor() { return nullptr; };

protected:
    EditorType();
    void setId(Utils::Id id) { m_id = id; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setMimeTypes(const QStringList &mimeTypes) { m_mimeTypes = mimeTypes; }
    void addMimeType(const QString &mimeType) { m_mimeTypes.append(mimeType); }

private:
    Utils::Id m_id;
    QString m_displayName;
    QStringList m_mimeTypes;
};

class CORE_EXPORT IEditorFactory : public EditorType
{
    Q_OBJECT

public:
    IEditorFactory();
    ~IEditorFactory() override;

    static const EditorFactoryList allEditorFactories();
    static const EditorFactoryList preferredEditorFactories(const Utils::FilePath &filePath);

    IEditor *createEditor() const;

    IEditorFactory *asEditorFactory() override { return this; }

protected:
    void setEditorCreator(const std::function<IEditor *()> &creator);

private:
    std::function<IEditor *()> m_creator;
};

} // namespace Core
