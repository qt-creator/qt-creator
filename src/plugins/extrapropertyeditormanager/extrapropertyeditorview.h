// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

namespace QmlDesigner {

class ExtraPropertyEditorView : public AbstractView
{
    Q_OBJECT
public:
    ExtraPropertyEditorView(class AsynchronousImageCache &imageCache,
                            ExternalDependenciesInterface &externalDependencies);
    ~ExtraPropertyEditorView() override;

private:
    AsynchronousImageCache &m_imageCache;
    ExternalDependenciesInterface &m_externalDependencies;
};

} //  namespace QmlDesigner
