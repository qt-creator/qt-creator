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
inline constexpr char Light[] = "Light";
inline constexpr char IntType[] = "int";
inline constexpr char Item[] = "Item";
inline constexpr char KeyframeGroup[] = "KeyframeGroup";
inline constexpr char Keyframe[] = "Keyframe";
inline constexpr char Layout[] = "Layout";
inline constexpr char ListElement[] = "ListElement";
inline constexpr char ListModel[] = "ListModel";
inline constexpr char ListView[] = "ListView";
inline constexpr char Loader[] = "Loader";
inline constexpr char Material[] = "Material";
inline constexpr char Model[] = "Model";
inline constexpr char MouseArea[] = "MouseArea";
inline constexpr char Node[] = "Node";
inline constexpr char Particle3D[] = "Particle3D";
inline constexpr char ParticleEmitter3D[] = "ParticleEmitter3D";
inline constexpr char Pass[] = "Pass";
inline constexpr char PathView[] = "PathView";
inline constexpr char Path[] = "Path";
inline constexpr char PauseAnimation[] = "PauseAnimation";
inline constexpr char Picture[] = "Picture";
inline constexpr char Popup[] = "Popup";
inline constexpr char Positioner[] = "Positioner";
inline constexpr char PrincipledMaterial[] = "PrincipledMaterial";
inline constexpr char SpecularGlossyMaterial[] = "SpecularGlossyMaterial";
inline constexpr char PropertyAnimation[] = "PropertyAnimation";
inline constexpr char PropertyChanges[] = "PropertyChanges";
inline constexpr char QML[] = "QML";
inline constexpr char QML_cppnative[] = "QML-cppnative";
inline constexpr char QQuick3DParticleAbstractShape[] = "QQuick3DParticleAbstractShape";
inline constexpr char QQuickStateOperation[] = "QQuickStateOperation";
inline constexpr char QtMultimedia[] = "QtMultimedia";
inline constexpr char QtObject[] = "QtObject";
inline constexpr char QtQml[] = "QtQml";
inline constexpr char QtQml_Models[] = "QtQml.Models";
inline constexpr char QtQuick3D[] = "QtQuick3D";
inline constexpr char QtQuick3D_Particles3D[] = "QtQuick3D.Particles3D";
inline constexpr char QtQuick3D_Particles3D_cppnative[] = "QtQuick3D.Particles3D-cppnative";
inline constexpr char QtQuick[] = "QtQuick";
inline constexpr char QtQuick_cppnative[] = "QtQuick-cppnative";
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
inline constexpr char SplitView[] = "SplitView";
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

template<const char *moduleName_, const char *typeName_>
struct CacheType : public BaseCacheType
{
};

template<typename ProjectStorage>
class CommonTypeCache
{
    using CommonTypes = std::tuple<CacheType<FlowView, FlowActionArea>,
                                   CacheType<FlowView, FlowDecision>,
                                   CacheType<FlowView, FlowItem>,
                                   CacheType<FlowView, FlowTransition>,
                                   CacheType<FlowView, FlowView>,
                                   CacheType<FlowView, FlowWildcard>,
                                   CacheType<QML, BoolType>,
                                   CacheType<QML, Component>,
                                   CacheType<QML, DoubleType>,
                                   CacheType<QML, IntType>,
                                   CacheType<QML, QtObject>,
                                   CacheType<QML, date>,
                                   CacheType<QML, string>,
                                   CacheType<QML, url>,
                                   CacheType<QML, var>,
                                   CacheType<QML_cppnative, FloatType>,
                                   CacheType<QML_cppnative, UIntType>,
                                   CacheType<QtMultimedia, SoundEffect>,
                                   CacheType<QtQml_Models, ListElement>,
                                   CacheType<QtQml_Models, ListModel>,
                                   CacheType<QtQuick, BorderImage>,
                                   CacheType<QtQuick, Connections>,
                                   CacheType<QtQuick, GridView>,
                                   CacheType<QtQuick, Image>,
                                   CacheType<QtQuick, Item>,
                                   CacheType<QtQuick, ListView>,
                                   CacheType<QtQuick, Loader>,
                                   CacheType<QtQuick, MouseArea>,
                                   CacheType<QtQuick, Path>,
                                   CacheType<QtQuick, PathView>,
                                   CacheType<QtQuick, PauseAnimation>,
                                   CacheType<QtQuick, Positioner>,
                                   CacheType<QtQuick, PropertyAnimation>,
                                   CacheType<QtQuick, PropertyChanges>,
                                   CacheType<QtQuick, Rectangle>,
                                   CacheType<QtQuick, Repeater>,
                                   CacheType<QtQuick, State>,
                                   CacheType<QtQuick, StateGroup>,
                                   CacheType<QtQuick, Text>,
                                   CacheType<QtQuick, TextEdit>,
                                   CacheType<QtQuick, Transition>,
                                   CacheType<QtQuick, color>,
                                   CacheType<QtQuick, font>,
                                   CacheType<QtQuick, vector2d>,
                                   CacheType<QtQuick, vector3d>,
                                   CacheType<QtQuick, vector4d>,
                                   CacheType<QtQuick3D, BakedLightmap>,
                                   CacheType<QtQuick3D, Buffer>,
                                   CacheType<QtQuick3D, Camera>,
                                   CacheType<QtQuick3D, Command>,
                                   CacheType<QtQuick3D, CubeMapTexture>,
                                   CacheType<QtQuick3D, DefaultMaterial>,
                                   CacheType<QtQuick3D, Effect>,
                                   CacheType<QtQuick3D, InstanceList>,
                                   CacheType<QtQuick3D, InstanceListEntry>,
                                   CacheType<QtQuick3D, Light>,
                                   CacheType<QtQuick3D, Material>,
                                   CacheType<QtQuick3D, Model>,
                                   CacheType<QtQuick3D, Node>,
                                   CacheType<QtQuick3D, Pass>,
                                   CacheType<QtQuick3D, PrincipledMaterial>,
                                   CacheType<QtQuick3D, SceneEnvironment>,
                                   CacheType<QtQuick3D, SpecularGlossyMaterial>,
                                   CacheType<QtQuick3D, Shader>,
                                   CacheType<QtQuick3D, Texture>,
                                   CacheType<QtQuick3D, TextureInput>,
                                   CacheType<QtQuick3D, View3D>,
                                   CacheType<QtQuick3D_Particles3D, Affector3D>,
                                   CacheType<QtQuick3D_Particles3D, Attractor3D>,
                                   CacheType<QtQuick3D_Particles3D, Model>,
                                   CacheType<QtQuick3D_Particles3D, Particle3D>,
                                   CacheType<QtQuick3D_Particles3D, ParticleEmitter3D>,
                                   CacheType<QtQuick3D_Particles3D, SpriteParticle3D>,
                                   CacheType<QtQuick3D_Particles3D_cppnative, QQuick3DParticleAbstractShape>,
                                   CacheType<QtQuick_Controls, Control>,
                                   CacheType<QtQuick_Controls, Popup>,
                                   CacheType<QtQuick_Controls, SplitView>,
                                   CacheType<QtQuick_Controls, SwipeView>,
                                   CacheType<QtQuick_Controls, TabBar>,
                                   CacheType<QtQuick_Controls, TextArea>,
                                   CacheType<QtQuick_Dialogs, Dialog>,
                                   CacheType<QtQuick_Extras, Picture>,
                                   CacheType<QtQuick_Layouts, Layout>,
                                   CacheType<QtQuick_Studio_Components, GroupItem>,
                                   CacheType<QtQuick_Templates, Control>,
                                   CacheType<QtQuick_Timeline, Keyframe>,
                                   CacheType<QtQuick_Timeline, KeyframeGroup>,
                                   CacheType<QtQuick_Timeline, Timeline>,
                                   CacheType<QtQuick_Timeline, TimelineAnimation>,
                                   CacheType<QtQuick_cppnative, QQuickStateOperation>,
                                   CacheType<Qt_SafeRenderer, SafePicture>,
                                   CacheType<Qt_SafeRenderer, SafeRendererPicture>,
                                   CacheType<QtQuick_Window, Window>>;

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

    template<const char *moduleName, const char *typeName>
    TypeId typeId() const
    {
        auto &type = std::get<CacheType<moduleName, typeName>>(m_types);
        if (type.typeId)
            return type.typeId;

        return refreshTypedId(type, moduleName, typeName);
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
            return typeId<QML_cppnative, UIntType>();
        else if constexpr (std::is_same_v<Type, bool>)
            return typeId<QML, BoolType>();
        else if constexpr (std::is_same_v<Type, float>)
            return typeId<QML_cppnative, FloatType>();
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
                          ::Utils::SmallStringView typeName) const
    {
        if (!type.moduleId)
            type.moduleId = m_projectStorage.moduleId(moduleName);

        type.typeId = m_projectStorage.typeId(type.moduleId, typeName, Storage::Version{});

        return type.typeId;
    }

    TypeId refreshTypedIdWithoutTransaction(BaseCacheType &type,
                                            ::Utils::SmallStringView moduleName,
                                            ::Utils::SmallStringView typeName) const
    {
        if (!type.moduleId)
            type.moduleId = m_projectStorage.fetchModuleIdUnguarded(moduleName);

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

    template<const char *moduleName, const char *typeName>
    TypeId typeIdWithoutTransaction() const
    {
        auto &type = std::get<CacheType<moduleName, typeName>>(m_types);
        if (type.typeId)
            return type.typeId;

        return refreshTypedIdWithoutTransaction(type, moduleName, typeName);
    }

    void updateTypeIdsWithoutProperties()
    {
        setupTypeIdsWithoutProperties({typeIdWithoutTransaction<QML, BoolType>(),
                                       typeIdWithoutTransaction<QML, IntType>(),
                                       typeIdWithoutTransaction<QML_cppnative, UIntType>(),
                                       typeIdWithoutTransaction<QML, DoubleType>(),
                                       typeIdWithoutTransaction<QML_cppnative, FloatType>(),
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
