// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <abstractview.h>

#include <memory.h>

namespace QmlDesigner {
class QMLDESIGNERCORE_EXPORT AuxiliaryPropertyStorageView final : public AbstractView
{
public:
    AuxiliaryPropertyStorageView(Sqlite::Database &database,
                                 ExternalDependenciesInterface &externalDependencies);
    ~AuxiliaryPropertyStorageView();

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId) override;

    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;

    void auxiliaryDataChanged(const ModelNode &node,
                              AuxiliaryDataKeyView type,
                              const QVariant &data) override;

    struct Data;

    void resetForTestsOnly();

private:
    std::unique_ptr<Data> d;
};
} // namespace QmlDesigner
