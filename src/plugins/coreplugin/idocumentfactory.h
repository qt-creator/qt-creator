// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/id.h>

#include <QStringList>

#include <functional>
#include <optional>

namespace Utils { class FilePath; }

namespace Core {

class IDocument;

class CORE_EXPORT IDocumentFactory
{
public:
    IDocumentFactory();
    ~IDocumentFactory();

    static const QList<IDocumentFactory *> allDocumentFactories();

    using Opener = std::function<IDocument *(const Utils::FilePath &filePath)>;
    IDocument *open(const Utils::FilePath &filePath);

    QStringList mimeTypes() const { return m_mimeTypes; }

    void setOpener(const Opener &opener) { m_opener = opener; }
    void setMimeTypes(const QStringList &mimeTypes) { m_mimeTypes = mimeTypes; }
    void addMimeType(const char *mimeType) { m_mimeTypes.append(QLatin1String(mimeType)); }
    void addMimeType(const QString &mimeType) { m_mimeTypes.append(mimeType); }
    void setIsProjectFactory(bool isProjectFactory) { m_isProjectFactory = isProjectFactory; }
    bool isProjectFactory() const { return m_isProjectFactory; }
    void setUsedForOpenFile(bool usedForOpenFile) { m_usedForOpenFile = usedForOpenFile; }
    bool isUsedForOpenFile() const { return m_usedForOpenFile.value_or(!m_isProjectFactory); }

private:
    Opener m_opener;
    QStringList m_mimeTypes;
    QString m_displayName;
    bool m_isProjectFactory = false;
    std::optional<bool> m_usedForOpenFile;
};

} // namespace Core
