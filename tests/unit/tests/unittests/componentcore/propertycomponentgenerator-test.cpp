// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <googletest.h>
#include <mocks/abstractviewmock.h>
#include <mocks/modelresourcemanagementmock.h>
#include <mocks/projectstoragemock.h>
#include <mocks/sourcepathcachemock.h>
#include <strippedstring-matcher.h>

#include <propertycomponentgenerator.h>

using namespace Qt::StringLiterals;

namespace QmlDesigner {

std::ostream &operator<<(std::ostream &out, const PropertyComponentGenerator::BasicProperty &property)
{
    return out << "BasicProperty(" << property.propertyName << ", " << property.component << ")";
}

std::ostream &operator<<(std::ostream &out, const PropertyComponentGenerator::ComplexProperty &property)
{
    return out << "ComplexPropertiesFromTemplates(" << property.propertyName << ", "
               << property.component << ")";
}

} // namespace QmlDesigner

namespace {

using BasicProperty = QmlDesigner::PropertyComponentGenerator::BasicProperty;
using ComplexProperty = QmlDesigner::PropertyComponentGenerator::ComplexProperty;

template<typename Matcher>
auto IsBasicProperty(const Matcher &matcher)
{
    return VariantWith<BasicProperty>(Field(&BasicProperty::component, matcher));
}

template<typename Matcher>
auto IsComplexProperty(const Matcher &matcher)
{
    return VariantWith<ComplexProperty>(Field(&ComplexProperty::component, matcher));
}

constexpr Utils::SmallStringView sourcesPath = UNITTEST_DIR
    "/../../../../share/qtcreator/qmldesigner/propertyEditorQmlSources";

class PropertyComponentGenerator : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        simpleReaderNode = createTemplateConfiguration(QString{sourcesPath});
    }

    static void TearDownTestSuite() { simpleReaderNode.reset(); }

    static QmlJS::SimpleReaderNode::Ptr createTemplateConfiguration(const QString &propertyEditorResourcesPath)
    {
        QmlJS::SimpleReader reader;
        const QString fileName = propertyEditorResourcesPath + u"/PropertyTemplates/TemplateTypes.qml";
        auto templateConfiguration = reader.readFile(fileName);

        if (!templateConfiguration)
            qWarning().nospace() << "template definitions:" << reader.errors();

        return templateConfiguration;
    }

    template<typename Type>
    static Type getProperty(const QmlJS::SimpleReaderNode *node, const QString &name)
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

    static QString getSource(const QString &name)
    {
        const auto &nodes = simpleReaderNode->children();
        for (const QmlJS::SimpleReaderNode::Ptr &node : nodes) {
            if (auto typeName = getProperty<QString>(node.get(), "typeNames"); typeName != name)
                continue;

            auto sourcePath = getProperty<QString>(node.get(), "sourceFile");

            return getContent(QString{sourcesPath} + "/PropertyTemplates/" + sourcePath);
        }

        return {};
    }

    static QString getContent(const QString &path)
    {
        QFile file{path};

        if (file.open(QIODevice::ReadOnly))
            return QString::fromUtf8(file.readAll());

        return {};
    }

    static QString getExpectedContent(const QString &typeName,
                                      const QString &propertyName,
                                      const QString &binding)
    {
        auto source = getSource(typeName);

        return source.arg(propertyName, binding);
    }

    QmlDesigner::NodeMetaInfo createTypeWithProperties(Utils::SmallStringView name,
                                                       QmlDesigner::TypeId propertyTypeId)
    {
        auto typeId = projectStorageMock.createValue(qmlModuleId, name);
        projectStorageMock.createProperty(typeId, "sub1", propertyTypeId);
        projectStorageMock.createProperty(typeId, "sub2", propertyTypeId);

        return {typeId, &projectStorageMock};
    }

    QmlDesigner::NodeMetaInfo createType(Utils::SmallStringView name)
    {
        auto typeId = projectStorageMock.createValue(qmlModuleId, name);

        return {typeId, &projectStorageMock};
    }

    QmlDesigner::PropertyMetaInfo createProperty(QmlDesigner::TypeId typeId,
                                                 Utils::SmallString name,
                                                 QmlDesigner::Storage::PropertyDeclarationTraits traits,
                                                 QmlDesigner::TypeId propertyTypeId)
    {
        auto propertyId = projectStorageMock.createProperty(typeId, name, traits, propertyTypeId);

        return {propertyId, &projectStorageMock};
    }

    QString toString(const QmlDesigner::PropertyComponentGenerator::Property &propertyComponent,
                     const QString &baseName,
                     const QString &propertyName)
    {
        auto text = std::visit(
            [](const auto &p) -> QString {
                if constexpr (!std::is_same_v<std::decay_t<decltype(p)>, std::monostate>)
                    return p.component;
                else
                    return {};
            },
            propertyComponent);

        text.replace(propertyName, baseName + "."_L1 + propertyName);
        text.replace("backendValues."_L1 + baseName + "."_L1 + propertyName,
                     "backendValues."_L1 + baseName + "_"_L1 + propertyName);

        return text;
    }

protected:
    inline static QSharedPointer<const QmlJS::SimpleReaderNode> simpleReaderNode;
    NiceMock<AbstractViewMock> viewMock;
    NiceMock<SourcePathCacheMockWithPaths> pathCacheMock{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock{pathCacheMock.sourceId};
    NiceMock<ModelResourceManagementMock> resourceManagementMock;
    QmlDesigner::Model model{{projectStorageMock, pathCacheMock},
                             "Item",
                             -1,
                             -1,
                             nullptr,
                             std::make_unique<ModelResourceManagementMockWrapper>(
                                 resourceManagementMock)};
    QmlDesigner::PropertyComponentGenerator generator{QString{sourcesPath}, &model};
    QmlDesigner::NodeMetaInfo itemMetaInfo = model.qtQuickItemMetaInfo();
    QmlDesigner::ModuleId qmlModuleId = projectStorageMock.createModule("QML");
};

TEST_F(PropertyComponentGenerator,
       basic_property_is_returned_for_property_with_a_template_with_no_separate_section)
{
    QString expected = getExpectedContent("real", "x", "x");
    auto xProperty = itemMetaInfo.property("x");

    auto propertyComponent = generator.create(xProperty);

    ASSERT_THAT(propertyComponent, IsBasicProperty(StrippedStringEq(expected)));
}

TEST_F(PropertyComponentGenerator,
       complex_property_is_returned_for_property_with_a_template_with_a_separate_section)
{
    QString expected = getExpectedContent("font", "font", "font");
    auto fontProperty = itemMetaInfo.property("font");

    auto propertyComponent = generator.create(fontProperty);

    ASSERT_THAT(propertyComponent, IsComplexProperty(StrippedStringEq(expected)));
}

TEST_F(PropertyComponentGenerator,
       complex_property_is_returned_for_property_without_a_template_and_subproperties)
{
    auto stringId = projectStorageMock.createValue(qmlModuleId, "string");
    auto fooNodeInfo = createTypeWithProperties("Foo", stringId);
    auto sub1Text = toString(generator.create(fooNodeInfo.property("sub1")), "foo", "sub1");
    auto sub2Text = toString(generator.create(fooNodeInfo.property("sub2")), "foo", "sub2");
    auto fooProperty = createProperty(itemMetaInfo.id(), "foo", {}, fooNodeInfo.id());
    QString expectedText = QStringLiteral(
        R"xy(
           Section {
             caption: foo - Foo
             anchors.left: parent.left
             anchors.right: parent.right
             leftPadding: 8
             rightPadding: 0
             expanded: false
             level: 1
             SectionLayout {
     )xy");
    expectedText += sub1Text;
    expectedText += sub2Text;
    expectedText += "}}";

    auto propertyComponent = generator.create(fooProperty);

    ASSERT_THAT(propertyComponent, IsComplexProperty(StrippedStringEq(expectedText)));
}

TEST_F(PropertyComponentGenerator,
       pointer_readonly_complex_property_is_returned_for_property_without_a_template_and_subproperties)
{
    auto stringId = projectStorageMock.createValue(qmlModuleId, "string");
    auto fooNodeInfo = createTypeWithProperties("Foo", stringId);
    auto sub1Text = toString(generator.create(fooNodeInfo.property("sub1")), "foo", "sub1");
    auto sub2Text = toString(generator.create(fooNodeInfo.property("sub2")), "foo", "sub2");
    auto fooProperty = createProperty(itemMetaInfo.id(),
                                      "foo",
                                      QmlDesigner::Storage::PropertyDeclarationTraits::IsPointer
                                          | QmlDesigner::Storage::PropertyDeclarationTraits::IsReadOnly,
                                      fooNodeInfo.id());
    QString expectedText = QStringLiteral(
        R"xy(
           Section {
             caption: foo - Foo
             anchors.left: parent.left
             anchors.right: parent.right
             leftPadding: 8
             rightPadding: 0
             expanded: false
             level: 1
             SectionLayout {
     )xy");
    expectedText += sub1Text;
    expectedText += sub2Text;
    expectedText += "}}";

    auto propertyComponent = generator.create(fooProperty);

    ASSERT_THAT(propertyComponent, IsComplexProperty(StrippedStringEq(expectedText)));
}

TEST_F(PropertyComponentGenerator, basic_property_without_template_is_returning_monostate)
{
    auto fooNodeInfo = createType("Foo");
    auto fooProperty = createProperty(itemMetaInfo.id(), "foo", {}, fooNodeInfo.id());

    auto propertyComponent = generator.create(fooProperty);

    ASSERT_THAT(propertyComponent, VariantWith<std::monostate>(std::monostate{}));
}

TEST_F(PropertyComponentGenerator,
       complex_property_without_templates_and_writeable_is_returning_monostate)
{
    auto barTypeId = projectStorageMock.createValue(qmlModuleId, "bar");
    auto fooNodeInfo = createTypeWithProperties("Foo", barTypeId);
    auto fooProperty = createProperty(itemMetaInfo.id(), "foo", {}, fooNodeInfo.id());

    auto propertyComponent = generator.create(fooProperty);

    ASSERT_THAT(propertyComponent, VariantWith<std::monostate>(std::monostate{}));
}

TEST_F(PropertyComponentGenerator,
       pointer_writeable_complex_property_without_templates_and_with_only_subproperties_without_templates_is_returning_monostate)
{
    auto stringId = projectStorageMock.createValue(qmlModuleId, "string");
    auto fooNodeInfo = createTypeWithProperties("Foo", stringId);
    auto sub1Text = toString(generator.create(fooNodeInfo.property("sub1")), "foo", "sub1");
    auto sub2Text = toString(generator.create(fooNodeInfo.property("sub2")), "foo", "sub2");
    auto fooProperty = createProperty(itemMetaInfo.id(),
                                      "foo",
                                      QmlDesigner::Storage::PropertyDeclarationTraits::IsPointer,
                                      fooNodeInfo.id());

    auto propertyComponent = generator.create(fooProperty);

    ASSERT_THAT(propertyComponent, VariantWith<std::monostate>(std::monostate{}));
}

TEST_F(PropertyComponentGenerator, get_imports)
{
    auto imports = generator.imports();

    ASSERT_THAT(imports,
                ElementsAre(Eq("import HelperWidgets 2.0"),
                            Eq("import QtQuick 2.15"),
                            Eq("import QtQuick.Layouts 1.15"),
                            Eq("import StudioTheme 1.0 as StudioTheme")));
}

TEST_F(PropertyComponentGenerator, set_model_to_null_removes_creates_only_monostates)
{
    QString expected = getExpectedContent("real", "x", "x");
    auto xProperty = itemMetaInfo.property("x");

    generator.setModel(nullptr);

    ASSERT_THAT(generator.create(xProperty), VariantWith<std::monostate>(std::monostate{}));
}

TEST_F(PropertyComponentGenerator, set_model_fromn_null_updates_internal_state)
{
    generator.setModel(nullptr);
    QString expected = getExpectedContent("real", "x", "x");
    auto xProperty = itemMetaInfo.property("x");

    generator.setModel(&model);

    ASSERT_THAT(generator.create(xProperty), IsBasicProperty(StrippedStringEq(expected)));
}

TEST_F(PropertyComponentGenerator, after_refresh_meta_infos_type_was_deleted)
{
    auto xProperty = itemMetaInfo.property("x");
    auto doubleMetaInfo = model.doubleMetaInfo();
    projectStorageMock.removeExportedTypeName(doubleMetaInfo.id(),
                                              projectStorageMock.createModule("QML"),
                                              "real");

    generator.refreshMetaInfos({doubleMetaInfo.id()});

    ASSERT_THAT(generator.create(xProperty), VariantWith<std::monostate>(std::monostate{}));
}

TEST_F(PropertyComponentGenerator, after_refresh_meta_infos_type_was_added)
{
    QString expected = getExpectedContent("real", "x", "x");
    auto xProperty = itemMetaInfo.property("x");
    auto doubleMetaInfo = model.doubleMetaInfo();
    projectStorageMock.removeExportedTypeName(doubleMetaInfo.id(),
                                              projectStorageMock.createModule("QML"),
                                              "real");
    generator.refreshMetaInfos({doubleMetaInfo.id()});
    projectStorageMock.addExportedTypeName(doubleMetaInfo.id(),
                                           projectStorageMock.createModule("QML"),
                                           "real");

    generator.refreshMetaInfos({});

    ASSERT_THAT(generator.create(xProperty), IsBasicProperty(StrippedStringEq(expected)));
}

} // namespace
