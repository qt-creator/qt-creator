// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstoragetypes.h"

#include <modelfwd.h>

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

inline constexpr char Affector3D[] = "Affector3D";
inline constexpr char Attractor3D[] = "Attractor3D";
inline constexpr char BakedLightmap[] = "BakedLightmap";
inline constexpr char BoolType[] = "bool";
inline constexpr char BorderImage[] = "BorderImage";
inline constexpr char Buffer[] = "Buffer";
inline constexpr char Camera[] = "Camera";
inline constexpr char Command[] = "Command";
inline constexpr char Component[] = "Component";
inline constexpr char Connections[] = "Connections";
inline constexpr char Control[] = "Control";
inline constexpr char CubeMapTexture[] = "CubeMapTexture";
inline constexpr char DefaultMaterial[] = "DefaultMaterial";
inline constexpr char Dialog[] = "Dialog";
inline constexpr char DirectionalLight[] = "DirectionalLight";
inline constexpr char DoubleType[] = "double";
inline constexpr char Effect[] = "Effect";
inline constexpr char FloatType[] = "float";
inline constexpr char FlowActionArea[] = "FlowActionArea";
inline constexpr char FlowDecision[] = "FlowDecision";
inline constexpr char FlowItem[] = "FlowItem";
inline constexpr char FlowTransition[] = "FlowTransition";
inline constexpr char FlowView[] = "FlowView";
inline constexpr char FlowWildcard[] = "FlowWildcard";
inline constexpr char GridView[] = "GridView";
inline constexpr char GroupItem[] = "GroupItem";
inline constexpr char Image[] = "Image";
inline constexpr char InstanceListEntry[] = "InstanceListEntry";
inline constexpr char InstanceList[] = "InstanceList";
inline constexpr char IntType[] = "int";
inline constexpr char Item[] = "Item";
inline constexpr char JsonListModel[] = "JsonListModel";
inline constexpr char KeyframeGroup[] = "KeyframeGroup";
inline constexpr char Keyframe[] = "Keyframe";
inline constexpr char Layout[] = "Layout";
inline constexpr char Light[] = "Light";
inline constexpr char ListElement[] = "ListElement";
inline constexpr char ListModel[] = "ListModel";
inline constexpr char ListView[] = "ListView";
inline constexpr char Loader[] = "Loader";
inline constexpr char Material[] = "Material";
inline constexpr char Model[] = "Model";
inline constexpr char MouseArea[] = "MouseArea";
inline constexpr char Node[] = "Node";
inline constexpr char OrthographicCamera[] = "OrthographicCamera";
inline constexpr char Particle3D[] = "Particle3D";
inline constexpr char ParticleEmitter3D[] = "ParticleEmitter3D";
inline constexpr char Pass[] = "Pass";
inline constexpr char PathView[] = "PathView";
inline constexpr char Path[] = "Path";
inline constexpr char PauseAnimation[] = "PauseAnimation";
inline constexpr char PerspectiveCamera[] = "PerspectiveCamera";
inline constexpr char Picture[] = "Picture";
inline constexpr char PointLight[] = "PointLight";
inline constexpr char Popup[] = "Popup";
inline constexpr char Positioner[] = "Positioner";
inline constexpr char PrincipledMaterial[] = "PrincipledMaterial";
inline constexpr char PropertyAnimation[] = "PropertyAnimation";
inline constexpr char PropertyChanges[] = "PropertyChanges";
inline constexpr char QML[] = "QML";
inline constexpr char QQuick3DParticleAbstractShape[] = "QQuick3DParticleAbstractShape";
inline constexpr char QQuickStateOperation[] = "QQuickStateOperation";
inline constexpr char QtMultimedia[] = "QtMultimedia";
inline constexpr char QtObject[] = "QtObject";
inline constexpr char QtQml[] = "QtQml";
inline constexpr char QtQml_Models[] = "QtQml.Models";
inline constexpr char QtQml_XmlListModel[] = "QtQml.XmlListModel";
inline constexpr char QtQuick3D[] = "QtQuick3D";
inline constexpr char QtQuick3D_Particles3D[] = "QtQuick3D.Particles3D";
inline constexpr char QtQuick[] = "QtQuick";
inline constexpr char QtQuick_Controls[] = "QtQuick.Controls";
inline constexpr char QtQuick_Dialogs[] = "QtQuick.Dialogs";
inline constexpr char QtQuick_Extras[] = "QtQuick.Extras";
inline constexpr char QtQuick_Layouts[] = "QtQuick.Layouts";
inline constexpr char QtQuick_Studio_Components[] = "QtQuick.Studio.Components";
inline constexpr char QtQuick_Templates[] = "QtQuick.Templates";
inline constexpr char QtQuick_Timeline[] = "QtQuick.Timeline";
inline constexpr char QtQuick_Window[] = "QtQuick.Window";
inline constexpr char Qt_SafeRenderer[] = "Qt.SafeRenderer";
inline constexpr char Rectangle[] = "Rectangle";
inline constexpr char Repeater[] = "Repeater";
inline constexpr char SafePicture[] = "SafePicture";
inline constexpr char SafeRendererPicture[] = "SafeRendererPicture";
inline constexpr char SceneEnvironment[] = "SceneEnvironment";
inline constexpr char Shader[] = "Shader";
inline constexpr char SoundEffect[] = "SoundEffect";
inline constexpr char SpecularGlossyMaterial[] = "SpecularGlossyMaterial";
inline constexpr char SplitView[] = "SplitView";
inline constexpr char SpotLight[] = "SpotLight";
inline constexpr char SpriteParticle3D[] = "SpriteParticle3D";
inline constexpr char StateGroup[] = "StateGroup";
inline constexpr char State[] = "State";
inline constexpr char SwipeView[] = "SwipeView";
inline constexpr char TabBar[] = "TabBar";
inline constexpr char TextArea[] = "TextArea";
inline constexpr char TextEdit[] = "TextEdit";
inline constexpr char Text[] = "Text";
inline constexpr char TextureInput[] = "TextureInput";
inline constexpr char Texture[] = "Texture";
inline constexpr char TimelineAnimation[] = "TimelineAnimation";
inline constexpr char Timeline[] = "Timeline";
inline constexpr char Transition[] = "Transition";
inline constexpr char UIntType[] = "uint";
inline constexpr char View3D[] = "View3D";
inline constexpr char Window[] = "Window";
inline constexpr char XmlListModelRole[] = "XmlListModelRole";
inline constexpr char color[] = "color";
inline constexpr char date[] = "date";
inline constexpr char font[] = "font";
inline constexpr char string[] = "string";
inline constexpr char url[] = "url";
inline constexpr char var[] = "var";
inline constexpr char vector2d[] = "vector2d";
inline constexpr char vector3d[] = "vector3d";
inline constexpr char vector4d[] = "vector4d";

struct BaseCacheType
{
    QmlDesigner::ModuleId moduleId;
    QmlDesigner::TypeId typeId;
};

template<const char *moduleName_, ModuleKind moduleKind, const char *typeName_>
struct CacheType : public BaseCacheType
{
};

template<typename ProjectStorage>
class CommonTypeCache
{
    using CommonTypes = std::tuple<
        CacheType<FlowView, ModuleKind::QmlLibrary, FlowActionArea>,
        CacheType<FlowView, ModuleKind::QmlLibrary, FlowDecision>,
        CacheType<FlowView, ModuleKind::QmlLibrary, FlowItem>,
        CacheType<FlowView, ModuleKind::QmlLibrary, FlowTransition>,
        CacheType<FlowView, ModuleKind::QmlLibrary, FlowView>,
        CacheType<FlowView, ModuleKind::QmlLibrary, FlowWildcard>,
        CacheType<QML, ModuleKind::QmlLibrary, BoolType>,
        CacheType<QML, ModuleKind::QmlLibrary, Component>,
        CacheType<QML, ModuleKind::QmlLibrary, DoubleType>,
        CacheType<QML, ModuleKind::QmlLibrary, IntType>,
        CacheType<QML, ModuleKind::QmlLibrary, QtObject>,
        CacheType<QML, ModuleKind::QmlLibrary, date>,
        CacheType<QML, ModuleKind::QmlLibrary, string>,
        CacheType<QML, ModuleKind::QmlLibrary, url>,
        CacheType<QML, ModuleKind::QmlLibrary, var>,
        CacheType<QML, ModuleKind::CppLibrary, FloatType>,
        CacheType<QML, ModuleKind::CppLibrary, UIntType>,
        CacheType<QtQml, ModuleKind::QmlLibrary, Connections>,
        CacheType<QtMultimedia, ModuleKind::QmlLibrary, SoundEffect>,
        CacheType<QtQml_Models, ModuleKind::QmlLibrary, ListElement>,
        CacheType<QtQml_Models, ModuleKind::QmlLibrary, ListModel>,
        CacheType<QtQml_XmlListModel, ModuleKind::QmlLibrary, XmlListModelRole>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, BorderImage>,
        CacheType<QtQuick, ModuleKind::QmlLibrary, GridView>,
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
        CacheType<QtQuick_Controls, ModuleKind::QmlLibrary, Control>,
        CacheType<QtQuick_Controls, ModuleKind::QmlLibrary, Popup>,
        CacheType<QtQuick_Controls, ModuleKind::QmlLibrary, SplitView>,
        CacheType<QtQuick_Controls, ModuleKind::QmlLibrary, SwipeView>,
        CacheType<QtQuick_Controls, ModuleKind::QmlLibrary, TabBar>,
        CacheType<QtQuick_Controls, ModuleKind::QmlLibrary, TextArea>,
        CacheType<QtQuick_Dialogs, ModuleKind::QmlLibrary, Dialog>,
        CacheType<QtQuick_Extras, ModuleKind::QmlLibrary, Picture>,
        CacheType<QtQuick_Layouts, ModuleKind::QmlLibrary, Layout>,
        CacheType<QtQuick_Studio_Components, ModuleKind::QmlLibrary, GroupItem>,
        CacheType<QtQuick_Studio_Components, ModuleKind::QmlLibrary, JsonListModel>,
        CacheType<QtQuick_Templates, ModuleKind::QmlLibrary, Control>,
        CacheType<QtQuick_Timeline, ModuleKind::QmlLibrary, Keyframe>,
        CacheType<QtQuick_Timeline, ModuleKind::QmlLibrary, KeyframeGroup>,
        CacheType<QtQuick_Timeline, ModuleKind::QmlLibrary, Timeline>,
        CacheType<QtQuick_Timeline, ModuleKind::QmlLibrary, TimelineAnimation>,
        CacheType<QtQuick, ModuleKind::CppLibrary, QQuickStateOperation>,
        CacheType<Qt_SafeRenderer, ModuleKind::QmlLibrary, SafePicture>,
        CacheType<Qt_SafeRenderer, ModuleKind::QmlLibrary, SafeRendererPicture>,
        CacheType<QtQuick_Window, ModuleKind::QmlLibrary, Window>>;

public:
    CommonTypeCache(const ProjectStorage &projectStorage)
        : m_projectStorage{projectStorage}
    {
        m_typesWithoutProperties.fill(TypeId{});
    }

    CommonTypeCache(const CommonTypeCache &) = delete;
    CommonTypeCache &operator=(const CommonTypeCache &) = delete;
    CommonTypeCache(CommonTypeCache &&) = default;
    CommonTypeCache &operator=(CommonTypeCache &&) = default;

    void resetTypeIds()
    {
        std::apply([](auto &...type) { ((type.typeId = QmlDesigner::TypeId{}), ...); }, m_types);

        updateTypeIdsWithoutProperties();
    }

    void clearForTestsOnly()
    {
        std::apply([](auto &...type) { ((type.typeId = QmlDesigner::TypeId{}), ...); }, m_types);
        std::fill(std::begin(m_typesWithoutProperties), std ::end(m_typesWithoutProperties), TypeId{});
    }

    template<const char *moduleName, const char *typeName, ModuleKind moduleKind = ModuleKind::QmlLibrary>
    TypeId typeId() const
    {
        auto &type = std::get<CacheType<moduleName, moduleKind, typeName>>(m_types);
        if (type.typeId)
            return type.typeId;

        return refreshTypedId(type, moduleName, moduleKind, typeName);
    }

    template<const char *typeName>
    TypeId builtinTypeId() const
    {
        return typeId<QML, typeName>();
    }

    template<typename Type>
    TypeId builtinTypeId() const
    {
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
    TypeId refreshTypedId(BaseCacheType &type,
                          ::Utils::SmallStringView moduleName,
                          ModuleKind moduleKind,
                          ::Utils::SmallStringView typeName) const
    {
        if (!type.moduleId)
            type.moduleId = m_projectStorage.moduleId(moduleName, moduleKind);

        type.typeId = m_projectStorage.typeId(type.moduleId, typeName, Storage::Version{});

        return type.typeId;
    }

    TypeId refreshTypedIdWithoutTransaction(BaseCacheType &type,
                                            ::Utils::SmallStringView moduleName,
                                            ::Utils::SmallStringView typeName,
                                            ModuleKind moduleKind) const
    {
        if (!type.moduleId)
            type.moduleId = m_projectStorage.fetchModuleIdUnguarded(moduleName, moduleKind);

        type.typeId = m_projectStorage.fetchTypeIdByModuleIdAndExportedName(type.moduleId, typeName);

        return type.typeId;
    }

    template<std::size_t size>
    void setupTypeIdsWithoutProperties(const TypeId (&typeIds)[size])
    {
        static_assert(size == std::tuple_size_v<TypeIdsWithoutProperties>,
                      "array size must match type id count!");
        std::copy(std::begin(typeIds), std::end(typeIds), std::begin(m_typesWithoutProperties));
    }

    template<const char *moduleName, const char *typeName, ModuleKind moduleKind = ModuleKind::QmlLibrary>
    TypeId typeIdWithoutTransaction() const
    {
        auto &type = std::get<CacheType<moduleName, moduleKind, typeName>>(m_types);
        if (type.typeId)
            return type.typeId;

        return refreshTypedIdWithoutTransaction(type, moduleName, typeName, moduleKind);
    }

    void updateTypeIdsWithoutProperties()
    {
        setupTypeIdsWithoutProperties(
            {typeIdWithoutTransaction<QML, BoolType>(),
             typeIdWithoutTransaction<QML, IntType>(),
             typeIdWithoutTransaction<QML, UIntType, ModuleKind::CppLibrary>(),
             typeIdWithoutTransaction<QML, DoubleType>(),
             typeIdWithoutTransaction<QML, FloatType, ModuleKind::CppLibrary>(),
             typeIdWithoutTransaction<QML, date>(),
             typeIdWithoutTransaction<QML, string>(),
             typeIdWithoutTransaction<QML, url>()});
    }

private:
    const ProjectStorage &m_projectStorage;
    mutable CommonTypes m_types;
    TypeIdsWithoutProperties m_typesWithoutProperties;
};

} // namespace QmlDesigner::Storage::Info
