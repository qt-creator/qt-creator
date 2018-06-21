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

#include "bineditorservice.h"

#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/ieditorfactory.h>

namespace BinEditor {
namespace Internal {

class BinEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BinEditor.json")

public:
    BinEditorPlugin() {}
    ~BinEditorPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final {}
};

class BinEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    BinEditorFactory();

    Core::IEditor *createEditor() final;
};

class FactoryServiceImpl : public QObject, public FactoryService
{
    Q_OBJECT
    Q_INTERFACES(BinEditor::FactoryService)

public:
    EditorService *createEditorService(const QString &title0, bool wantsEditor) final;
};

} // namespace Internal
} // namespace BinEditor
