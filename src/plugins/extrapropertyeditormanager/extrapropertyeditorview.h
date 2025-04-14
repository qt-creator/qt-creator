// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

namespace QmlDesigner {

class ExtraPropertyEditorAction;

class ExtraPropertyEditorView : public AbstractView
{
    Q_OBJECT
public:
    ExtraPropertyEditorView(class AsynchronousImageCache &imageCache,
                            ExternalDependenciesInterface &externalDependencies);
    ~ExtraPropertyEditorView() override;

    void customNotification(const AbstractView *view,
                            const QString &identifier,
                            const QList<ModelNode> &nodeList,
                            const QList<QVariant> &data) override;

    ExtraPropertyEditorAction *unifiedAction() const;

private:
    AsynchronousImageCache &m_imageCache;
    ExternalDependenciesInterface &m_externalDependencies;
    ExtraPropertyEditorAction *m_unifiedAction = nullptr;
};

} //  namespace QmlDesigner
