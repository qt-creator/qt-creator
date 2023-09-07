// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "propertycomponentgeneratorinterface.h"

#include <modelfwd.h>
#include <nodemetainfo.h>

#include <qmljs/qmljssimplereader.h>

#include <QPointer>

#include <optional>
#include <variant>
#include <vector>

namespace QmlDesigner {

class PropertyComponentGenerator final : public PropertyComponentGeneratorInterface
{
public:
    PropertyComponentGenerator(const QString &propertyEditorResourcesPath, Model *model);

    Property create(const PropertyMetaInfo &property) const override;

    struct Entry
    {
        NodeMetaInfo type;
        Utils::SmallString typeName;
        QString propertyTemplate;
        bool separateSection = false;
        bool needsTypeArg = false;
    };

    using Entries = std::vector<Entry>;

    QStringList imports() const override { return m_imports; }

    void setModel(Model *model);

    void refreshMetaInfos(const TypeIds &deletedTypeIds);

private:
    const Entry *findEntry(const NodeMetaInfo &type) const;
    QString generateSubComponentText(Utils::SmallStringView propertyBaseName,
                                     const PropertyMetaInfo &subProperty) const;
    QString generateComplexComponentText(Utils::SmallStringView propertyName,
                                         const NodeMetaInfo &propertyType) const;
    Property generateComplexComponent(const PropertyMetaInfo &property,
                                      const NodeMetaInfo &propertyType) const;

    void setEntries(QmlJS::SimpleReaderNode::Ptr templateConfiguration,
                    Model *model,
                    const QString &propertyTemplatesPath);

private:
    Entries m_entries;
    TypeIds m_entryTypeIds;
    QStringList m_imports;
    QPointer<Model> m_model;
    QmlJS::SimpleReaderNode::Ptr m_templateConfiguration;
    QString m_propertyTemplatesPath;
    bool m_hasInvalidTemplates = false;
};

} // namespace QmlDesigner
