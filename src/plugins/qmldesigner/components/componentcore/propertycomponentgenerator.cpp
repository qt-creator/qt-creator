// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertycomponentgenerator.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/set_algorithm.h>

#include <model.h>

#include <QFile>
#include <QString>

#include <memory>

using namespace Qt::StringLiterals;

namespace QmlDesigner {

namespace {

QmlJS::SimpleReaderNode::Ptr createTemplateConfiguration(const QString &propertyEditorResourcesPath)
{
    QmlJS::SimpleReader reader;
    const QString fileName = propertyEditorResourcesPath + u"/PropertyTemplates/TemplateTypes.qml";
    auto templateConfiguration = reader.readFile(fileName);

    if (!templateConfiguration)
        qWarning().nospace() << "template definitions:" << reader.errors();

    return templateConfiguration;
}

template<typename Type>
Type getProperty(const QmlJS::SimpleReaderNode *node, const QString &name)
{
    if (auto property = node->property(name)) {
        const auto &value = property.value;
        if (value.type() == QVariant::List) {
            auto list = value.toList();
            if (list.size())
                return list.front().value<Type>();
        } else {
            return property.value.value<Type>();
        }
    }

    return {};
}

QString getContent(const QString &path)
{
    QFile file{path};

    if (file.open(QIODevice::ReadOnly))
        return QString::fromUtf8(file.readAll());

    return {};
}

QString generateComponentText(Utils::SmallStringView propertyName,
                              QStringView source,
                              Utils::SmallStringView typeName,
                              bool needsTypeArg)
{
    QString underscoreName{propertyName};
    underscoreName.replace('.', '_');
    if (needsTypeArg)
        return source.arg(QString{propertyName}, underscoreName, QString{typeName});

    return source.arg(QString{propertyName}, underscoreName);
}

PropertyComponentGenerator::Property generate(const PropertyMetaInfo &property,
                                              const PropertyComponentGenerator::Entry &entry)
{
    auto propertyName = property.name();
    auto component = generateComponentText(propertyName,
                                           entry.propertyTemplate,
                                           entry.typeName,
                                           entry.needsTypeArg);

    if (entry.separateSection)
        return PropertyComponentGenerator::ComplexProperty{propertyName, component};

    return PropertyComponentGenerator::BasicProperty{propertyName, component};
}

auto getRandomExportedName(const NodeMetaInfo &metaInfo)
{
    const auto &names = metaInfo.allExportedTypeNames();

    using Result = decltype(names.front().name);

    if (!names.empty())
        return names.front().name;

    return Result{};
}

} // namespace

QString PropertyComponentGenerator::generateSubComponentText(Utils::SmallStringView propertyBaseName,
                                                             const PropertyMetaInfo &subProperty) const
{
    auto propertyType = subProperty.propertyType();
    if (auto entry = findEntry(propertyType)) {
        auto propertyName = Utils::SmallString::join({propertyBaseName, subProperty.name()});
        return generateComponentText(propertyName,
                                     entry->propertyTemplate,
                                     entry->typeName,
                                     entry->needsTypeArg);
    }

    return {};
}

QString PropertyComponentGenerator::generateComplexComponentText(Utils::SmallStringView propertyName,
                                                                 const NodeMetaInfo &propertyType) const
{
    auto subProperties = propertyType.properties();

    if (std::empty(subProperties))
        return {};

    auto propertyTypeName = getRandomExportedName(propertyType);
    static QString templateText = QStringLiteral(
        R"xy(
           Section {
             caption: %1 - %2
             anchors.left: parent.left
             anchors.right: parent.right
             leftPadding: 8
             rightPadding: 0
             expanded: false
             level: 1
             SectionLayout {
     )xy");

    auto component = templateText.arg(QString{propertyName}, QString{propertyTypeName});

    auto propertyBaseName = Utils::SmallString::join({propertyName, "."});

    bool subPropertiesHaveContent = false;
    for (const auto &subProperty : propertyType.properties()) {
        auto subPropertyContent = generateSubComponentText(propertyBaseName, subProperty);
        subPropertiesHaveContent = subPropertiesHaveContent || subPropertyContent.size();
        component += subPropertyContent;
    }

    component += "}\n}\n"_L1;

    if (subPropertiesHaveContent)
        return component;

    return {};
}

PropertyComponentGenerator::Property PropertyComponentGenerator::generateComplexComponent(
    const PropertyMetaInfo &property, const NodeMetaInfo &propertyType) const
{
    auto propertyName = property.name();
    auto component = generateComplexComponentText(propertyName, propertyType);

    if (component.isEmpty())
        return {};

    return ComplexProperty{propertyName, component};
}

namespace {

std::optional<PropertyComponentGenerator::Entry> createEntry(QmlJS::SimpleReaderNode *node,
                                                             Model *model,
                                                             const QString &templatesPath)
{
    auto moduleName = getProperty<QByteArray>(node, "module");
    if (moduleName.isEmpty())
        return {};

    auto module = model->module(moduleName);

    auto typeName = getProperty<QByteArray>(node, "typeNames");

    auto type = model->metaInfo(module, typeName);
    if (!type)
        return {};

    auto path = getProperty<QString>(node, "sourceFile");
    if (path.isEmpty())
        return {};

    auto content = getContent(templatesPath + path);
    if (content.isEmpty())
        return {};

    bool needsTypeArg = getProperty<bool>(node, "needsTypeArg");

    bool separateSection = getProperty<bool>(node, "separateSection");

    return PropertyComponentGenerator::Entry{std::move(type),
                                             std::move(typeName),
                                             std::move(content),
                                             separateSection,
                                             needsTypeArg};
}

std::tuple<PropertyComponentGenerator::Entries, bool> createEntries(
    QmlJS::SimpleReaderNode::Ptr templateConfiguration, Model *model, const QString &templatesPath)
{
    bool hasInvalidTemplates = false;
    PropertyComponentGenerator::Entries entries;
    entries.reserve(32);

    const auto &nodes = templateConfiguration->children();
    for (const QmlJS::SimpleReaderNode::Ptr &node : nodes) {
        if (auto entry = createEntry(node.get(), model, templatesPath))
            entries.push_back(*entry);
        else
            hasInvalidTemplates = true;
    }

    return {entries, hasInvalidTemplates};
}

QStringList createImports(QmlJS::SimpleReaderNode *templateConfiguration)
{
    auto property = templateConfiguration->property("imports");
    return Utils::transform<QStringList>(property.value.toList(),
                                         [](const auto &entry) { return entry.toString(); });
}

} // namespace

PropertyComponentGenerator::PropertyComponentGenerator(const QString &propertyEditorResourcesPath,
                                                       Model *model)
    : m_templateConfiguration{createTemplateConfiguration(propertyEditorResourcesPath)}
    , m_propertyTemplatesPath{propertyEditorResourcesPath + "/PropertyTemplates/"}
{
    setModel(model);
    m_imports = createImports(m_templateConfiguration.get());
}

PropertyComponentGenerator::Property PropertyComponentGenerator::create(const PropertyMetaInfo &property) const
{
    auto propertyType = property.propertyType();
    if (auto entry = findEntry(propertyType))
        return generate(property, *entry);

    if (property.isWritable() && property.isPointer())
        return {};

    return generateComplexComponent(property, propertyType);
}

void PropertyComponentGenerator::setModel(Model *model)
{
    if (model && m_model && m_model->projectStorage() == model->projectStorage()) {
        m_model = model;
        return;
    }

    if (model) {
        setEntries(m_templateConfiguration, model, m_propertyTemplatesPath);
    } else {
        m_entries.clear();
        m_entryTypeIds.clear();
    }
    m_model = model;
}

namespace {

bool insect(const TypeIds &first, const TypeIds &second)
{
    bool intersecting = false;

    std::set_intersection(first.begin(),
                          first.end(),
                          second.begin(),
                          second.end(),
                          Utils::make_iterator([&](const auto &) { intersecting = true; }));

    return intersecting;
}

} // namespace

void PropertyComponentGenerator::setEntries(QmlJS::SimpleReaderNode::Ptr templateConfiguration,
                                            Model *model,
                                            const QString &propertyTemplatesPath)
{
    auto [entries, hasInvalidTemplates] = createEntries(templateConfiguration,
                                                        model,
                                                        propertyTemplatesPath);
    m_entries = std::move(entries);
    m_hasInvalidTemplates = hasInvalidTemplates;
    m_entryTypeIds = Utils::transform<TypeIds>(m_entries,
                                               [](const auto &entry) { return entry.type.id(); });
    std::sort(m_entryTypeIds.begin(), m_entryTypeIds.end());
}

void PropertyComponentGenerator::refreshMetaInfos(const TypeIds &deletedTypeIds)
{
    if (!insect(deletedTypeIds, m_entryTypeIds) && !m_hasInvalidTemplates)
        return;

    setEntries(m_templateConfiguration, m_model, m_propertyTemplatesPath);
}

const PropertyComponentGenerator::Entry *PropertyComponentGenerator::findEntry(const NodeMetaInfo &type) const
{
    auto found = std::find_if(m_entries.begin(), m_entries.end(), [&](const auto &entry) {
        return entry.type == type;
    });

    if (found != m_entries.end())
        return std::addressof(*found);

    return nullptr;
}

} // namespace QmlDesigner
