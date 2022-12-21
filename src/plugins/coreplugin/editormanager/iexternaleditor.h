// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ieditorfactory.h"

#include <coreplugin/core_global.h>

#include <utils/id.h>

#include <QObject>

namespace Utils {
class FilePath;
class MimeType;
}

namespace Core {

class IExternalEditor;

using ExternalEditorList = QList<IExternalEditor *>;

class CORE_EXPORT IExternalEditor : public EditorType
{
    Q_OBJECT

public:
    explicit IExternalEditor();
    ~IExternalEditor() override;

    static const ExternalEditorList allExternalEditors();
    static const ExternalEditorList externalEditors(const Utils::MimeType &mimeType);

    IExternalEditor *asExternalEditor() override { return this; }

    virtual bool startEditor(const Utils::FilePath &filePath, QString *errorMessage) = 0;
};

} // namespace Core
