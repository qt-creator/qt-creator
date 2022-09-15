// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

#include <memory>

namespace QmlDesigner {

class ModelMerger;

class QMLDESIGNERCORE_EXPORT DesignDocumentView : public AbstractView
{
        Q_OBJECT
public:
    DesignDocumentView();
    ~DesignDocumentView() override;

    ModelNode insertModel(const ModelNode &modelNode);
    void replaceModel(const ModelNode &modelNode);

    void toClipboard() const;
    void fromClipboard();

    QString toText() const;
    void fromText(const QString &text);

    static std::unique_ptr<Model> pasteToModel();
    static void copyModelNodes(const QList<ModelNode> &nodesToCopy);

private:
    std::unique_ptr<ModelMerger> m_modelMerger;
};

}// namespace QmlDesigner
