// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <rewriterview.h>
#include <model/modeltotextmerger.h>

namespace QmlDesigner {

namespace Internal {

class TestModelToTextMerger : public ModelToTextMerger
{
public:
    bool isNodeScheduledForRemoval(const ModelNode &node) const;
    bool isNodeScheduledForAddition(const ModelNode &node) const;
    VariantProperty findAddedVariantProperty(const VariantProperty &property) const;
};

} // Internal

class TestRewriterView : public RewriterView
{

Q_OBJECT

public:
    TestRewriterView(ExternalDependenciesInterface &externalDependencies,
                     DifferenceHandling differenceHandling = RewriterView::Validate);

    Internal::TestModelToTextMerger *modelToTextMerger() const;
    Internal::TextToModelMerger *textToModelMerger() const;

    bool isModificationGroupActive() const;
    void setModificationGroupActive(bool active);
    void applyModificationGroupChanges();

    using RewriterView::auxiliaryDataAsQML;
};

} // QmlDesigner
