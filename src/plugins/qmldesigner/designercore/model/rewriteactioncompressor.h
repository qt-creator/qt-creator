// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "rewriteaction.h"

namespace QmlDesigner {
namespace Internal {

class RewriteActionCompressor
{
public:
    RewriteActionCompressor(const PropertyNameList &propertyOrder, ModelNodePositionStorage *positionStore) :
        m_propertyOrder(propertyOrder),
        m_positionStore(positionStore)
    {}

    void operator()(QList<RewriteAction *> &actions, const TextEditor::TabSettings &tabSettings) const;

private:
    void compressImports(QList<RewriteAction *> &actions) const;

    void compressRereparentActions(QList<RewriteAction *> &actions) const;
    void compressReparentIntoSamePropertyActions(QList<RewriteAction *> &actions) const;
    void compressReparentIntoNewPropertyActions(QList<RewriteAction *> &actions) const;
    void compressAddEditRemoveNodeActions(QList<RewriteAction *> &actions) const;
    void compressPropertyActions(QList<RewriteAction *> &actions) const;
    void compressAddEditActions(QList<RewriteAction *> &actions, const TextEditor::TabSettings &tabSettings) const;
    void compressAddReparentActions(QList<RewriteAction *> &actions) const;

private:
    PropertyNameList m_propertyOrder;
    ModelNodePositionStorage *m_positionStore;
};

} // namespace Internal
} // namespace QmlDesigner
