// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extrapropertyeditorview.h"

#include <qmldesignerplugin.h>

namespace QmlDesigner {

ExtraPropertyEditorView::ExtraPropertyEditorView(class AsynchronousImageCache &imageCache,
                                                 ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_imageCache{imageCache}
    , m_externalDependencies{externalDependencies}
{}

ExtraPropertyEditorView::~ExtraPropertyEditorView() {};

} //  namespace QmlDesigner
