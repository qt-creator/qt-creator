// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <projectstorage-matcher.h>
#include <strippedstring-matcher.h>

#include <projectstorage/projectstorage.h>
#include <projectstorage/sourcepathcache.h>
#include <projectstorage/typeannotationreader.h>

namespace {


class TypeAnnotationReader : public testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        static_database = std::make_unique<Sqlite::Database>(":memory:", Sqlite::JournalMode::Memory);

        static_projectStorage = std::make_unique<QmlDesigner::ProjectStorage>(
            *static_database, static_database->isInitialized());
    }

    static void TearDownTestSuite()
    {
        static_projectStorage.reset();
        static_database.reset();
    }

    auto moduleId(Utils::SmallStringView name) const
    {
        return storage.moduleId(name, QmlDesigner::Storage::ModuleKind::QmlLibrary);
    }

protected:
    inline static std::unique_ptr<Sqlite::Database> static_database;
    Sqlite::Database &database = *static_database;
    inline static std::unique_ptr<QmlDesigner::ProjectStorage> static_projectStorage;
    QmlDesigner::ProjectStorage &storage = *static_projectStorage;
    QmlDesigner::Storage::TypeAnnotationReader reader{storage};
    QmlDesigner::SourceId sourceId = QmlDesigner::SourceId::create(33);
    QmlDesigner::SourceId directorySourceId = QmlDesigner::SourceId::create(77);
};

TEST_F(TypeAnnotationReader, parse_type)
{
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"
        }
        Type {
            name: "QtQuick.Item"
            icon: "images/item-icon16.png"
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                UnorderedElementsAre(IsTypeAnnotation(sourceId,
                                                      directorySourceId,
                                                      "Frame",
                                                      moduleId("QtQuick.Controls"),
                                                      "/path/images/frame-icon16.png",
                                                      traits,
                                                      IsEmpty(),
                                                      IsEmpty()),
                                     IsTypeAnnotation(sourceId,
                                                      directorySourceId,
                                                      "Item",
                                                      moduleId("QtQuick"),
                                                      "/path/images/item-icon16.png",
                                                      traits,
                                                      IsEmpty(),
                                                      IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_canBeContainer)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                canBeContainer: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.canBeContainer = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_forceClip)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                forceClip: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.forceClip = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_doesLayoutChildren)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                doesLayoutChildren: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.doesLayoutChildren = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_canBeDroppedInFormEditor)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                canBeDroppedInFormEditor: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.canBeDroppedInFormEditor = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_canBeDroppedInNavigator)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                canBeDroppedInNavigator: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.canBeDroppedInNavigator = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_canBeDroppedInView3D)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                canBeDroppedInView3D: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.canBeDroppedInView3D = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_isMovable)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                isMovable: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.isMovable = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_isResizable)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                isResizable: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.isResizable = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_hasFormEditorItem)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                hasFormEditorItem: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.hasFormEditorItem = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_isStackedContainer)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                isStackedContainer: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.isStackedContainer = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_takesOverRenderingOfChildren)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                takesOverRenderingOfChildren: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.takesOverRenderingOfChildren = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_visibleInNavigator)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                visibleInNavigator: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.visibleInNavigator = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_true_visibleInLibrary)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                visibleInLibrary: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;
    traits.visibleInLibrary = FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_false)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                isMovable: false
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_complex_expression)
{
    using QmlDesigner::FlagIs;
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            Hints {
                isMovable: true || false
                visibleNonDefaultProperties: "layer.effect"
            }
        }

        Type {
            name: "QtQuick.Item"
            icon: "images/item-icon16.png"

            Hints {
                canBeContainer: true
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits frameTraits;
    frameTraits.isMovable = QmlDesigner::FlagIs::Set;
    QmlDesigner::Storage::TypeTraits itemTraits;
    itemTraits.canBeContainer = QmlDesigner::FlagIs::True;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                UnorderedElementsAre(IsTypeAnnotation(sourceId,
                                                      directorySourceId,
                                                      "Frame",
                                                      moduleId("QtQuick.Controls"),
                                                      "/path/images/frame-icon16.png",
                                                      frameTraits,
                                                      StrippedStringEq(R"xy({"isMovable":"true || false",
                                                                             "visibleNonDefaultProperties":"layer.effect"})xy"),
                                                      IsEmpty()),
                                     IsTypeAnnotation(sourceId,
                                                      directorySourceId,
                                                      "Item",
                                                      moduleId("QtQuick"),
                                                      "/path/images/item-icon16.png",
                                                      itemTraits,
                                                      IsEmpty(),
                                                      IsEmpty())));
}

TEST_F(TypeAnnotationReader, parse_item_library_entry)
{
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            ItemLibraryEntry {
                name: "Frame"
                category: "Qt Quick - Controls 2"
                libraryIcon: "images/frame-icon.png"
                requiredImport: "QtQuick.Controls"
                toolTip: qsTr("An untitled container for a group of controls.")
            }

            ItemLibraryEntry {
                name: "Large Frame"
                category: "Qt Quick - Controls 2"
                libraryIcon: "images/frame-icon.png"
                requiredImport: "QtQuick.Controls"
                toolTip: qsTr("An large container for a group of controls.")
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             StrippedStringEq(R"xy([
                                                  {"category":"Qt Quick - Controls 2",
                                                   "iconPath":"/path/images/frame-icon.png",
                                                   "import":"QtQuick.Controls",
                                                   "name":"Frame",
                                                   "toolTip":"qsTr(\"An untitled container for a group of controls.\")"},
                                                  {"category":"Qt Quick - Controls 2",
                                                   "iconPath":"/path/images/frame-icon.png",
                                                   "import":"QtQuick.Controls",
                                                   "name":"Large Frame",
                                                   "toolTip":"qsTr(\"An large container for a group of controls.\")"}]
                                             )xy"))));
}

TEST_F(TypeAnnotationReader, parse_item_library_entry_with_properties)
{
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"
            icon: "images/frame-icon16.png"

            ItemLibraryEntry {
                name: "Frame"
                category: "Qt Quick - Controls 2"
                libraryIcon: "images/frame-icon.png"
                requiredImport: "QtQuick.Controls"
                toolTip: qsTr("An untitled container for a group of controls.")

                Property { name: "width"; type: "int"; value: 200 }
                Property { name: "height"; type: "int"; value: 100 }
            }

            ItemLibraryEntry {
                name: "Large Frame"
                category: "Qt Quick - Controls 2"
                libraryIcon: "images/frame-icon.png"
                requiredImport: "QtQuick.Controls"
                toolTip: qsTr("An large container for a group of controls.")

                Property { name: "width"; type: "int"; value: 2000 }
                Property { name: "height"; type: "int"; value: 1000 }
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             "/path/images/frame-icon16.png",
                                             traits,
                                             IsEmpty(),
                                             StrippedStringEq(R"xy([
                                                  {"category":"Qt Quick - Controls 2",
                                                   "iconPath":"/path/images/frame-icon.png",
                                                   "import":"QtQuick.Controls",
                                                   "name":"Frame",
                                                   "properties":[["width","int",200.0],["height","int",100.0]],
                                                   "toolTip":"qsTr(\"An untitled container for a group of controls.\")"},
                                                  {"category":"Qt Quick - Controls 2",
                                                   "iconPath":"/path/images/frame-icon.png",
                                                   "import":"QtQuick.Controls",
                                                   "name":"Large Frame",
                                                   "properties":[["width","int",2000.0],["height","int",1000.0]],
                                                   "toolTip":"qsTr(\"An large container for a group of controls.\")"}]
                                             )xy"))));
}

TEST_F(TypeAnnotationReader, parse_item_library_entry_template_path)
{
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"

            ItemLibraryEntry {
                name: "Frame"

                QmlSource{ source: "templates/frame.qml" }
            }
        }
        Type {
            name: "QtQuick.Item"

            ItemLibraryEntry {
                name: "Item"

                QmlSource{ source: "templates/item.qml" }
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             Utils::SmallStringView{},
                                             traits,
                                             IsEmpty(),
                                             StrippedStringEq(R"xy([
                                                  {"name":"Frame",
                                                   "templatePath":"/path/templates/frame.qml"}]
                                             )xy")),
                            IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Item",
                                             moduleId("QtQuick"),
                                             Utils::SmallStringView{},
                                             traits,
                                             IsEmpty(),
                                             StrippedStringEq(R"xy([
                                                  {"name":"Item",
                                                   "templatePath":"/path/templates/item.qml"}]
                                             )xy"))));
}

TEST_F(TypeAnnotationReader, parse_item_library_entry_extra_file_paths)
{
    auto content = QString{R"xy(
    MetaInfo {
        Type {
            name: "QtQuick.Controls.Frame"

            ItemLibraryEntry {
                name: "Frame"

                ExtraFile{ source: "templates/frame.png" }
                ExtraFile{ source: "templates/frame.frag" }
            }
        }
        Type {
            name: "QtQuick.Item"

            ItemLibraryEntry {
                name: "Item"

                ExtraFile{ source: "templates/item.png" }
            }
        }
    })xy"};
    QmlDesigner::Storage::TypeTraits traits;

    auto annotations = reader.parseTypeAnnotation(content, "/path", sourceId, directorySourceId);

    ASSERT_THAT(annotations,
                ElementsAre(IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Frame",
                                             moduleId("QtQuick.Controls"),
                                             Utils::SmallStringView{},
                                             traits,
                                             IsEmpty(),
                                             StrippedStringEq(R"xy([
                                                  {"extraFilePaths":["/path/templates/frame.png", "/path/templates/frame.frag"],
                                                   "name":"Frame"}]
                                             )xy")),
                            IsTypeAnnotation(sourceId,
                                             directorySourceId,
                                             "Item",
                                             moduleId("QtQuick"),
                                             Utils::SmallStringView{},
                                             traits,
                                             IsEmpty(),
                                             StrippedStringEq(R"xy([
                                                  {"extraFilePaths":["/path/templates/item.png"],
                                                   "name":"Item"}]
                                             )xy"))));
}

} // namespace
