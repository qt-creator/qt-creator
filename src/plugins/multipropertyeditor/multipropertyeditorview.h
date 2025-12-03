// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

namespace QmlDesigner {

class MultiPropertyEditorAction;
class PropertyEditorView;

class MultiPropertyEditorView : public AbstractView
{
    Q_OBJECT
public:
    MultiPropertyEditorView(
        class AsynchronousImageCache &imageCache,
        ExternalDependenciesInterface &externalDependencies);
    ~MultiPropertyEditorView() override;

    MultiPropertyEditorAction *unifiedAction() const;

private: // functions
    PropertyEditorView *createView(const QString &parentId);
    void setTargetNode(const ModelNode &node);
    void setCallbacks(PropertyEditorView *view);

private: // variables
    AsynchronousImageCache &m_imageCache;
    ExternalDependenciesInterface &m_externalDependencies;
    MultiPropertyEditorAction *m_unifiedAction = nullptr;
};

} //  namespace QmlDesigner
