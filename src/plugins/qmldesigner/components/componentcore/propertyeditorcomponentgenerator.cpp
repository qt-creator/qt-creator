// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorcomponentgenerator.h"

#include <utils/environment.h>
#include <utils/span.h>

#include <modelnode.h>

namespace QmlDesigner {

namespace {

using GeneratorProperty = PropertyComponentGeneratorInterface::Property;
using BasicProperty = PropertyComponentGeneratorInterface::BasicProperty;
using ComplexProperty = PropertyComponentGeneratorInterface::ComplexProperty;
using GeneratorProperties = std::vector<GeneratorProperty>;

QString createImports(const QStringList &imports)
{
    QString importsContent;
    importsContent.reserve(512);

    for (const QString &import : imports) {
        importsContent += import;
        importsContent += '\n';
    }

    return importsContent;
}

QString componentButton(bool isComponent)
{
    if (isComponent)
        return "ComponentButton {}";

    return {};
}

void createBasicPropertySections(QString &components,
                                 Utils::span<GeneratorProperty> generatorProperties)
{
    components += R"xy(
      Column {
        width: parent.width
        leftPadding: 8
        bottomPadding: 10
        SectionLayout {)xy";

    for (const auto &generatorProperty : generatorProperties)
        components += std::get_if<BasicProperty>(&generatorProperty)->component;

    components += R"xy(
        }
     })xy";
}

void createComplexPropertySections(QString &components,
                                   Utils::span<GeneratorProperty> generatorProperties)
{
    for (const auto &generatorProperty : generatorProperties)
        components += std::get_if<ComplexProperty>(&generatorProperty)->component;
}

Utils::SmallStringView propertyName(const GeneratorProperty &property)
{
    return std::visit(
        [](const auto &p) -> Utils::SmallStringView {
            if constexpr (!std::is_same_v<std::decay_t<decltype(p)>, std::monostate>)
                return p.propertyName;
            else
                return {};
        },
        property);
}

PropertyMetaInfos getUnmangedProperties(const NodeMetaInfos &prototypes)
{
    PropertyMetaInfos properties;
    properties.reserve(128);

    for (const auto &prototype : prototypes) {
        if (prototype.propertyEditorPathId())
            break;

        auto localProperties = prototype.localProperties();

        properties.insert(properties.end(), localProperties.begin(), localProperties.end());
    }

    return properties;
}

GeneratorProperties createSortedGeneratorProperties(
    const PropertyMetaInfos &properties, const PropertyComponentGeneratorType &propertyGenerator)
{
    GeneratorProperties generatorProperties;
    generatorProperties.reserve(properties.size());

    for (const auto &property : properties) {
        auto generatedProperty = propertyGenerator.create(property);
        if (!std::holds_alternative<std::monostate>(generatedProperty))
            generatorProperties.push_back(std::move(generatedProperty));
    }
    std::sort(generatorProperties.begin(),
              generatorProperties.end(),
              [](const auto &first, const auto &second) {
                  // this is sensitive to the order of the variant types but the test should catch any error
                  return std::forward_as_tuple(first.index(), propertyName(first))
                         < std::forward_as_tuple(second.index(), propertyName(second));
              });

    return generatorProperties;
}

QString createPropertySections(const PropertyComponentGeneratorType &propertyGenerator,
                               const NodeMetaInfos &prototypeChain)
{
    QString propertyComponents;
    propertyComponents.reserve(100000);

    auto generatorProperties = createSortedGeneratorProperties(getUnmangedProperties(prototypeChain),
                                                               propertyGenerator);

    const auto begin = generatorProperties.begin();
    const auto end = generatorProperties.end();

    auto point = std::partition_point(begin, end, [](const auto &p) {
        return !std::holds_alternative<ComplexProperty>(p);
    });

    if (begin != point)
        createBasicPropertySections(propertyComponents, Utils::span<GeneratorProperty>{begin, point});

    if (point != end)
        createComplexPropertySections(propertyComponents, Utils::span<GeneratorProperty>{point, end});

    return propertyComponents;
}

} // namespace

PropertyEditorComponentGenerator::PropertyEditorComponentGenerator(
    const PropertyComponentGeneratorType &propertyGenerator)
    : m_propertyGenerator{propertyGenerator}
{}

QString PropertyEditorComponentGenerator::create(const NodeMetaInfos &prototypeChain, bool isComponent)
{
    return QString{R"xy(
      %1
      Column {
        width: parent.width
        %2
        Section {
          caption: "%3"
          anchors.left: parent.left
          anchors.right: parent.right
          leftPadding: 0
          rightPadding: 0
          bottomPadding: 0
          Column {
            width: parent.width
            %4
          }
        }
      })xy"}
        .arg(createImports(m_propertyGenerator.imports()),
             componentButton(isComponent),
             QObject::tr("Exposed Custom Properties"),
             createPropertySections(m_propertyGenerator, prototypeChain));
}

} // namespace QmlDesigner
