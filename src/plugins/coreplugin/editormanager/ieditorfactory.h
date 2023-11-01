// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/id.h>

#include <QStringList>

#include <functional>

namespace Utils {
class FilePath;
class MimeType;
}

namespace Core {

class IEditor;
class IEditorFactory;

using EditorFactories = QList<IEditorFactory *>;

class CORE_EXPORT IEditorFactory
{
public:
    virtual ~IEditorFactory();

    static const EditorFactories allEditorFactories();
    static IEditorFactory *editorFactoryForId(const Utils::Id &id);
    static const EditorFactories defaultEditorFactories(const Utils::MimeType &mimeType);
    static const EditorFactories preferredEditorTypes(const Utils::FilePath &filePath);
    static const EditorFactories preferredEditorFactories(const Utils::FilePath &filePath);

    Utils::Id id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    QStringList mimeTypes() const { return m_mimeTypes; }

    bool isInternalEditor() const;
    bool isExternalEditor() const;

    IEditor *createEditor() const;
    bool startEditor(const Utils::FilePath &filePath, QString *errorMessage);

protected:
    IEditorFactory();

    void setId(Utils::Id id) { m_id = id; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setMimeTypes(const QStringList &mimeTypes) { m_mimeTypes = mimeTypes; }
    void addMimeType(const QString &mimeType) { m_mimeTypes.append(mimeType); }
    void setEditorCreator(const std::function<IEditor *()> &creator);
    void setEditorStarter(const std::function<bool(const Utils::FilePath &, QString *)> &starter);

private:
    Utils::Id m_id;
    QString m_displayName;
    QStringList m_mimeTypes;
    std::function<IEditor *()> m_creator;
    std::function<bool(const Utils::FilePath &, QString *)> m_starter;
};

} // namespace Core
