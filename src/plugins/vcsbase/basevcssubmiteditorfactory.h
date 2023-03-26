// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <coreplugin/editormanager/ieditorfactory.h>
#include <functional>

#include <QAction>

namespace VcsBase {

class VcsBaseSubmitEditor;
class VcsBaseSubmitEditorParameters;
class VcsBasePluginPrivate;

// Parametrizable base class for editor factories creating instances of
// VcsBaseSubmitEditor subclasses.

class VCSBASE_EXPORT VcsSubmitEditorFactory : public Core::IEditorFactory
{
public:
    typedef std::function<VcsBaseSubmitEditor *()> EditorCreator;

    VcsSubmitEditorFactory(const VcsBaseSubmitEditorParameters &parameters,
                           const EditorCreator &editorCreator,
                           VcsBasePluginPrivate *plugin);

    ~VcsSubmitEditorFactory();

private:
    QAction m_submitAction;
    QAction m_diffAction;
    QAction m_undoAction;
    QAction m_redoAction;
};

} // namespace VcsBase
