// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstoragetypes.h"

#include <modelfwd.h>
#include <modulesstorage/modulesstorage.h>
#include <tracing/qmldesignertracing.h>

#include <algorithm>
#include <tuple>
#include <type_traits>

QT_BEGIN_NAMESPACE
class QColor;
class QDateTime;
class QString;
class QUrl;
class QVariant;
class QVector2D;
class QVector3D;
class QVector4D;
QT_END_NAMESPACE

namespace QmlDesigner::Storage::Info {

inline constexpr StaticString Affector3D = {"Affector3D"};
inline constexpr StaticString ArcItem = {"ArcItem"};
inline constexpr StaticString Attractor3D = {"Attractor3D"};
inline constexpr StaticString BakedLightmap = {"BakedLightmap"};
inline constexpr StaticString BoolType = {"bool"};
inline constexpr StaticString BorderImage = {"BorderImage"};
inline constexpr StaticString Buffer = {"Buffer"};
inline constexpr StaticString Camera = {"Camera"};
inline constexpr StaticString ColumnLayout = {"ColumnLayout"};
inline constexpr StaticString Column = {"Column"};
inline constexpr StaticString Command = {"Command"};
inline constexpr StaticString Component = {"Component"};
inline constexpr StaticString Connections = {"Connections"};
inline constexpr StaticString Control = {"Control"};
inline constexpr StaticString CubeMapTexture = {"CubeMapTexture"};
inline constexpr StaticString DefaultMaterial = {"DefaultMaterial"};
inline constexpr StaticString DirectionalLight = {"DirectionalLight"};
inline constexpr StaticString DoubleType = {"double"};
inline constexpr StaticString Effect = {"Effect"};
inline constexpr StaticString FloatType = {"float"};
inline constexpr StaticString Flow = {"Flow"};
inline constexpr StaticString Gradient = {"Gradient"};
inline constexpr StaticString GridLayout = {"GridLayout"};
inline constexpr StaticString GridView = {"GridView"};
inline constexpr StaticString Grid = {"Grid"};
inline constexpr StaticString GroupItem = {"GroupItem"};
inline constexpr StaticString Image = {"Image"};
inline constexpr StaticString InstanceListEntry = {"InstanceListEntry"};
inline constexpr StaticString InstanceList = {"InstanceList"};
inline constexpr StaticString IntType = {"int"};
inline constexpr StaticString Item = {"Item"};
inline constexpr StaticString JsonListModel = {"JsonListModel"};
inline constexpr StaticString KeyframeGroup = {"KeyframeGroup"};
inline constexpr StaticString Keyframe = {"Keyframe"};
inline constexpr StaticString Label = {"Label"};
inline constexpr StaticString Layout = {"Layout"};
inline constexpr StaticString Light = {"Light"};
inline constexpr StaticString ListElement = {"ListElement"};
inline constexpr StaticString ListModel = {"ListModel"};
inline constexpr StaticString ListView = {"ListView"};
inline constexpr StaticString Loader = {"Loader"};
inline constexpr StaticString Material = {"Material"};
inline constexpr StaticString Model = {"Model"};
inline constexpr StaticString MouseArea = {"MouseArea"};
inline constexpr StaticString Node = {"Node"};
inline constexpr StaticString Object3D = {"Object3D"};
inline constexpr StaticString OrthographicCamera = {"OrthographicCamera"};
inline constexpr StaticString Particle3D = {"Particle3D"};
inline constexpr StaticString ParticleEmitter3D = {"ParticleEmitter3D"};
inline constexpr StaticString Pass = {"Pass"};
inline constexpr StaticString PathView = {"PathView"};
inline constexpr StaticString Path = {"Path"};
inline constexpr StaticString PauseAnimation = {"PauseAnimation"};
inline constexpr StaticString PerspectiveCamera = {"PerspectiveCamera"};
inline constexpr StaticString Picture = {"Picture"};
inline constexpr StaticString PointLight = {"PointLight"};
inline constexpr StaticString Popup = {"Popup"};
inline constexpr StaticString Positioner = {"Positioner"};
inline constexpr StaticString PrincipledMaterial = {"PrincipledMaterial"};
inline constexpr StaticString PropertyAnimation = {"PropertyAnimation"};
inline constexpr StaticString PropertyChanges = {"PropertyChanges"};
inline constexpr StaticString QML = {"QML"};
inline constexpr StaticString QQuick3DParticleAbstractShape = {"QQuick3DParticleAbstractShape"};
inline constexpr StaticString QQuickAbstractDialog = {"QQuickAbstractDialog"};
inline constexpr StaticString QQuickStateOperation = {"QQuickStateOperation"};
inline constexpr StaticString QtMultimedia = {"QtMultimedia"};
inline constexpr StaticString QtObject = {"QtObject"};
inline constexpr StaticString QtQml = {"QtQml"};
inline constexpr StaticString QtQml_Models = {"QtQml.Models"};
inline constexpr StaticString QtQml_XmlListModel = {"QtQml.XmlListModel"};
inline constexpr StaticString QtQuick3D = {"QtQuick3D"};
inline constexpr StaticString QtQuick3D_Particles3D = {"QtQuick3D.Particles3D"};
inline constexpr StaticString QtQuick = {"QtQuick"};
inline constexpr StaticString QtQuick_Dialogs = {"QtQuick.Dialogs"};
inline constexpr StaticString QtQuick_Layouts = {"QtQuick.Layouts"};
inline constexpr StaticString QtQuick_Shapes = {"QtQuick.Shapes"};
inline constexpr StaticString QtQuick_Studio_Components = {"QtQuick.Studio.Components"};
inline constexpr StaticString QtQuick_Templates = {"QtQuick.Templates"};
inline constexpr StaticString QtQuick_Timeline = {"QtQuick.Timeline"};
inline constexpr StaticString Qt_SafeRenderer = {"Qt.SafeRenderer"};
inline constexpr StaticString Rectangle = {"Rectangle"};
inline constexpr StaticString Repeater = {"Repeater"};
inline constexpr StaticString RowLayout = {"RowLayout"};
inline constexpr StaticString Row = {"Row"};
inline constexpr StaticString SafePicture = {"SafePicture"};
inline constexpr StaticString SafeRendererPicture = {"SafeRendererPicture"};
inline constexpr StaticString SceneEnvironment = {"SceneEnvironment"};
inline constexpr StaticString Shader = {"Shader"};
inline constexpr StaticString Shape = {"Shape"};
inline constexpr StaticString SoundEffect = {"SoundEffect"};
inline constexpr StaticString SpecularGlossyMaterial = {"SpecularGlossyMaterial"};
inline constexpr StaticString SplitView = {"SplitView"};
inline constexpr StaticString SpotLight = {"SpotLight"};
inline constexpr StaticString SpriteParticle3D = {"SpriteParticle3D"};
inline constexpr StaticString StateGroup = {"StateGroup"};
inline constexpr StaticString State = {"State"};
inline constexpr StaticString SvgPathItem = {"SvgPathItem"};
inline constexpr StaticString SwipeView = {"SwipeView"};
inline constexpr StaticString TabBar = {"TabBar"};
inline constexpr StaticString TextArea = {"TextArea"};
inline constexpr StaticString TextEdit = {"TextEdit"};
inline constexpr StaticString Text = {"Text"};
inline constexpr StaticString TextureInput = {"TextureInput"};
inline constexpr StaticString Texture = {"Texture"};
inline constexpr StaticString TimelineAnimation = {"TimelineAnimation"};
inline constexpr StaticString Timeline = {"Timeline"};
inline constexpr StaticString Transition = {"Transition"};
inline constexpr StaticString UIntType = {"uint"};
inline constexpr StaticString View3D = {"View3D"};
inline constexpr StaticString Window = {"Window"};
inline constexpr StaticString XmlListModelRole = {"XmlListModelRole"};
inline constexpr StaticString color = {"color"};
inline constexpr StaticString date = {"date"};
inline constexpr StaticString font = {"font"};
inline constexpr StaticString string = {"string"};
inline constexpr StaticString url = {"url"};
inline constexpr StaticString var = {"var"};
inline constexpr StaticString vector2d = {"vector2d"};
inline constexpr StaticString vector3d = {"vector3d"};
inline constexpr StaticString vector4d = {"vector4d"};

struct BaseCacheType
{
    QmlDesigner::ModuleId moduleId;
    QmlDesigner::TypeId typeId;
};

template<StaticString moduleName, ModuleKind moduleKind, StaticString typeName>
struct CacheType : public BaseCacheType
{
};

template<typename ProjectStorage>
class CommonTypeCache
{
    using CommonTypes = std::tuple<
        CacheType<QML, ModuleKind::CppLibrary, FloatType>,
        CacheType<QML, ModuleKind::CppLibrary, UIntType>,
        CacheType<QML, ModuleKind::QmlLibrary, BoolType>,
        CacheType<QML, ModuleKind::QmlLibrary, Component>,
        CacheType<QML, ModuleKind::QmlLibrary, DoubleType>,
        CacheType<QML, ModuleKind::QmlLibrary, IntType>,
        CacheType<QML, ModuleKind::QmlLibrary, QtObject>,
        CacheType<QML, ModuleKind::QmlLibrary, date>,
        CacheType<QML, ModuleKind::QmlLibrary, string>,
        CacheType<QML, ModuleKind::QmlLibrary, url>,
        CacheType<QML, ModuleKind::QmlLibrary, var>,
        CacheType<QtMultimedia, ModuleKind::QmlLibrary, SoundEffect>,
        CacheType<QtQml, ModuleKind::QmlLibrary, Connections>,
        CacheType<QtQml_Models, ModuleKind::QmlLibrary, ListElement>,
        CacheType<QtQml_Models, ModuleKind::QmlLibrary, ListModel>,
        CacheType<QtQml_XmlListModel, ModuleKind::QmlLibrary, XmlListModelRole>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, BorderImage>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Column>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Gradient>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Grid>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, GridView>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Flow>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Image>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Item>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, ListView>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Loader>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, MouseArea>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Path>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, PathView>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, PauseAnimation>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Positioner>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, PropertyAnimation>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, PropertyChanges>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Rectangle>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Repeater>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Row>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, State>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, StateGroup>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Text>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, TextEdit>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Transition>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, color>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, font>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, vector2d>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, vector3d>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, vector4d>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, BakedLightmap>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Buffer>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Camera>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Command>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, CubeMapTexture>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, DefaultMaterial>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, DirectionalLight>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Effect>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, InstanceList>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, InstanceListEntry>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Light>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Material>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Model>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Node>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Object3D>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, OrthographicCamera>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Pass>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, PerspectiveCamera>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, PointLight>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, PrincipledMaterial>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, SceneEnvironment>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Shader>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, SpecularGlossyMaterial>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, SpotLight>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, Texture>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, TextureInput>,
        CacheType<QtQuick3D, ModuleKind::QmlLibrary, View3D>,
        CacheType<QtQuick3D_Particles3D, ModuleKind::QmlLibrary, Affector3D>,
        CacheType<QtQuick3D_Particles3D, ModuleKind::QmlLibrary, Attractor3D>,
        CacheType<QtQuick3D_Particles3D, ModuleKind::QmlLibrary, Model>,
        CacheType<QtQuick3D_Particles3D, ModuleKind::QmlLibrary, Particle3D>,
        CacheType<QtQuick3D_Particles3D, ModuleKind::QmlLibrary, ParticleEmitter3D>,
        CacheType<QtQuick3D_Particles3D, ModuleKind::QmlLibrary, SpriteParticle3D>,
        CacheType<QtQuick3D_Particles3D, ModuleKind::CppLibrary, QQuick3DParticleAbstractShape>,
        CacheType<QtQuick_Dialogs, ModuleKind::CppLibrary, QQuickAbstractDialog>,
        CacheType<QtQuick_Layouts, ModuleKind::QmlLibrary, ColumnLayout>,
        CacheType<QtQuick_Layouts, ModuleKind::QmlLibrary, GridLayout>,
        CacheType<QtQuick_Layouts, ModuleKind::QmlLibrary, Layout>,
        CacheType<QtQuick_Layouts, ModuleKind::QmlLibrary, RowLayout>,
        CacheType<QtQuick_Shapes, ModuleKind::QmlLibrary, Shape>,
        CacheType<QtQuick_Studio_Components, ModuleKind::QmlLibrary, ArcItem>,
        CacheType<QtQuick_Studio_Components, ModuleKind::QmlLibrary, GroupItem>,
        CacheType<QtQuick_Studio_Components, ModuleKind::QmlLibrary, JsonListModel>,
        CacheType<QtQuick_Studio_Components, ModuleKind::QmlLibrary, SvgPathItem>,
        CacheType<QtQuick_Templates, ModuleKind::QmlLibrary, Control>,
        CacheType<QtQuick_Templates, ModuleKind::QmlLibrary, Label>,
        CacheType<QtQuick_Templates, ModuleKind::QmlLibrary, Popup>,
        CacheType<QtQuick_Templates, ModuleKind::QmlLibrary, SplitView>,
        CacheType<QtQuick_Templates, ModuleKind::QmlLibrary, SwipeView>,
        CacheType<QtQuick_Templates, ModuleKind::QmlLibrary, TabBar>,
        CacheType<QtQuick_Templates, ModuleKind::QmlLibrary, TextArea>,
        CacheType<QtQuick_Timeline, ModuleKind::QmlLibrary, Keyframe>,
        CacheType<QtQuick_Timeline, ModuleKind::QmlLibrary, KeyframeGroup>,
        CacheType<QtQuick_Timeline, ModuleKind::QmlLibrary, Timeline>,
        CacheType<QtQuick_Timeline, ModuleKind::QmlLibrary, TimelineAnimation>,
        CacheType<QtQuick, ModuleKind::CppLibrary, QQuickStateOperation>,
        CacheType<Qt_SafeRenderer, ModuleKind::QmlLibrary, SafePicture>,
        CacheType<Qt_SafeRenderer, ModuleKind::QmlLibrary, SafeRendererPicture>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, Window>>;

public:
    CommonTypeCache(const ProjectStorage &projectStorage, ModulesStorage &modulesStorage)
        : m_projectStorage{projectStorage}
        , m_modulesStorage{modulesStorage}
    {
        NanotraceHR::Tracer tracer{"common type cache constructor", ModelTracing::category()};

        m_typesWithoutProperties.fill(TypeId{});
    }

    CommonTypeCache(const CommonTypeCache &) = delete;
    CommonTypeCache &operator=(const CommonTypeCache &) = delete;
    CommonTypeCache(CommonTypeCache &&) = default;
    CommonTypeCache &operator=(CommonTypeCache &&) = default;

    void refreshTypeIds()
    {
        NanotraceHR::Tracer tracer{"common type cache type ids refresh", ModelTracing::category()};

        std::apply([](auto &...type) { ((type.typeId = QmlDesigner::TypeId{}), ...); }, m_types);

        std::apply([&](auto &...type) { (refreshTypedId(type), ...); }, m_types);

        updateTypeIdsWithoutProperties();
    }

    void clearForTestsOnly()
    {
        NanotraceHR::Tracer tracer{"common type cache type ids clear for tests only",
                                   ModelTracing::category()};

        std::apply([](auto &...type) { ((type.typeId = QmlDesigner::TypeId{}), ...); }, m_types);
        std::fill(std::begin(m_typesWithoutProperties), std ::end(m_typesWithoutProperties), TypeId{});
    }

    template<StaticString moduleName, StaticString typeName, ModuleKind moduleKind = ModuleKind::QmlLibrary>
    TypeId typeId() const
    {
        NanotraceHR::Tracer tracer{"common type cache type id for module and type name",
                                   ModelTracing::category(),
                                   NanotraceHR::keyValue("module", moduleName),
                                   NanotraceHR::keyValue("type", typeName)};

        const auto &type = std::get<CacheType<moduleName, moduleKind, typeName>>(m_types);

        return type.typeId;
    }

    template<StaticString typeName>
    TypeId builtinTypeId() const
    {
        NanotraceHR::Tracer tracer{"common type cache type id for built-in type",
                                   ModelTracing::category()};
        return typeId<QML, typeName>();
    }

    template<typename Type>
    TypeId builtinTypeId() const
    {
        NanotraceHR::Tracer tracer{"common type cache type id for built-in type",
                                   ModelTracing::category()};

        if constexpr (std::is_same_v<Type, double>)
            return typeId<QML, DoubleType>();
        else if constexpr (std::is_same_v<Type, int>)
            return typeId<QML, IntType>();
        else if constexpr (std::is_same_v<Type, uint>)
            return typeId<QML, UIntType, ModuleKind::CppLibrary>();
        else if constexpr (std::is_same_v<Type, bool>)
            return typeId<QML, BoolType>();
        else if constexpr (std::is_same_v<Type, float>)
            return typeId<QML, FloatType, ModuleKind::CppLibrary>();
        else if constexpr (std::is_same_v<Type, QString>)
            return typeId<QML, string>();
        else if constexpr (std::is_same_v<Type, QDateTime>)
            return typeId<QML, date>();
        else if constexpr (std::is_same_v<Type, QUrl>)
            return typeId<QML, url>();
        else if constexpr (std::is_same_v<Type, QVariant>)
            return typeId<QML, var>();
        else if constexpr (std::is_same_v<Type, QColor>)
            return typeId<QtQuick, color>();
        else if constexpr (std::is_same_v<Type, QVector2D>)
            return typeId<QtQuick, vector2d>();
        else if constexpr (std::is_same_v<Type, QVector3D>)
            return typeId<QtQuick, vector3d>();
        else if constexpr (std::is_same_v<Type, QVector4D>)
            return typeId<QtQuick, vector4d>();
        else
            static_assert(!std::is_same_v<Type, Type>, "built-in type not supported");
        return TypeId{};
    }

    const TypeIdsWithoutProperties &typeIdsWithoutProperties() const
    {
        return m_typesWithoutProperties;
    }

private:
    void refreshTypedIdWithoutTransaction(BaseCacheType &type,
                                          Utils::SmallStringView moduleName,
                                          Utils::SmallStringView typeName,
                                          ModuleKind moduleKind) const
    {
        NanotraceHR::Tracer tracer{"common type cache refresh type it without transaction",
                                   ModelTracing::category()};

        if (!type.moduleId)
            type.moduleId = m_modulesStorage.moduleId(moduleName, moduleKind);

        type.typeId = m_projectStorage.fetchTypeIdByModuleIdAndExportedName(type.moduleId, typeName);
    }

    template<StaticString moduleName, ModuleKind moduleKind, StaticString typeName>
    void refreshTypedId(CacheType<moduleName, moduleKind, typeName> &type) const
    {
        refreshTypedIdWithoutTransaction(type, moduleName, typeName, moduleKind);
    }

    template<std::size_t size>
    void setupTypeIdsWithoutProperties(const TypeId (&typeIds)[size])
    {
        NanotraceHR::Tracer tracer{"common type cache setup type ids without properties",
                                   ModelTracing::category()};

        static_assert(size == std::tuple_size_v<TypeIdsWithoutProperties>,
                      "array size must match type id count!");
        std::copy(std::begin(typeIds), std::end(typeIds), std::begin(m_typesWithoutProperties));
    }

    void updateTypeIdsWithoutProperties()
    {
        NanotraceHR::Tracer tracer{"common type cache type ids without properties update",
                                   ModelTracing::category()};

        setupTypeIdsWithoutProperties({typeId<QML, BoolType>(),
                                       typeId<QML, IntType>(),
                                       typeId<QML, UIntType, ModuleKind::CppLibrary>(),
                                       typeId<QML, DoubleType>(),
                                       typeId<QML, FloatType, ModuleKind::CppLibrary>(),
                                       typeId<QML, date>(),
                                       typeId<QML, string>(),
                                       typeId<QML, url>()});
    }

private:
    const ProjectStorage &m_projectStorage;
    ModulesStorage &m_modulesStorage;
    mutable CommonTypes m_types;
    TypeIdsWithoutProperties m_typesWithoutProperties;
};

} // namespace QmlDesigner::Storage::Info
