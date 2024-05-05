// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/document_controller/documentcontroller.h"

#include <utils/filepath.h>

namespace ModelEditor {
namespace Internal {

class ElementTasks;
class PxNodeController;

class ExtDocumentController :
        public qmt::DocumentController
{
    Q_OBJECT
    class ExtDocumentControllerPrivate;

public:
    explicit ExtDocumentController(QObject *parent = nullptr);
    ~ExtDocumentController();

    ElementTasks *elementTasks() const;
    PxNodeController *pxNodeController() const;

private:
    void onProjectFileNameChanged(const Utils::FilePath &fileName);

private:
    ExtDocumentControllerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
