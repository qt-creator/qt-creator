// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <QPointer>
#include <functional>

namespace QmlDesigner {

class AbstractView;
class ModelNode;

using MergePredicate = std::function<bool(const ModelNode &)>;

class QMLDESIGNERCORE_EXPORT ModelMerger
{
public:
    ModelMerger(AbstractView *view) : m_view(view) {}

    ModelNode insertModel(const ModelNode &modelNode,
                          const MergePredicate &predicate = [](const ModelNode &) { return true; });
    void replaceModel(const ModelNode &modelNode,
                      const MergePredicate &predicate = [](const ModelNode &) { return true; });

protected:
    AbstractView *view() const
    { return m_view.data(); }

private:
    QPointer<AbstractView> m_view;
};

} //namespace QmlDesigner
