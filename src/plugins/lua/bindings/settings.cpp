// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/layoutbuilder.h>

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

using namespace Utils;

namespace Lua::Internal {

class LuaAspectContainer : public AspectContainer
{
public:
    using AspectContainer::AspectContainer;

    sol::object dynamic_get(const std::string &key)
    {
        auto it = m_entries.find(key);
        if (it == m_entries.cend()) {
            return sol::lua_nil;
        }
        return it->second;
    }

    void dynamic_set(const std::string &key, const sol::stack_object &value)
    {
        if (!value.is<BaseAspect>())
            throw std::runtime_error("AspectContainer can only contain BaseAspect instances");

        registerAspect(value.as<BaseAspect *>(), false);

        auto it = m_entries.find(key);
        if (it == m_entries.cend()) {
            m_entries.insert(it, {std::move(key), std::move(value)});
        } else {
            std::pair<const std::string, sol::object> &kvp = *it;
            sol::object &entry = kvp.second;
            entry = sol::object(std::move(value));
        }
    }

    size_t size() const { return m_entries.size(); }

public:
    std::unordered_map<std::string, sol::object> m_entries;
};

std::unique_ptr<LuaAspectContainer> aspectContainerCreate(const sol::table &options)
{
    auto container = std::make_unique<LuaAspectContainer>();

    for (const auto &[k, v] : options) {
        if (k.is<std::string>()) {
            std::string key = k.as<std::string>();
            if (key == "autoApply") {
                container->setAutoApply(v.as<bool>());
            } else if (key == "layouter") {
                if (v.is<sol::function>())
                    container->setLayouter(
                        [func = v.as<sol::function>()]() -> Layouting::Layout {
                            auto res = safe_call<Layouting::Layout>(func);
                            QTC_ASSERT_EXPECTED(res, return {});
                            return *res;
                        });
            } else if (key == "onApplied") {
                QObject::connect(
                    container.get(),
                    &AspectContainer::applied,
                    container.get(),
                    [func = v.as<sol::function>()] { void_safe_call(func); });
            } else if (key == "settingsGroup") {
                container->setSettingsGroup(v.as<QString>());
            } else {
                container->m_entries[key] = v;
                if (v.is<BaseAspect>()) {
                    container->registerAspect(v.as<BaseAspect *>());
                } else {
                    qWarning() << "Unknown key:" << key.c_str();
                }
            }
        }
    }

    container->readSettings();

    return container;
}

void baseAspectCreate(BaseAspect *aspect, const std::string &key, const sol::object &value)
{
    if (key == "settingsKey")
        aspect->setSettingsKey(keyFromString(value.as<QString>()));
    else if (key == "displayName")
        aspect->setDisplayName(value.as<QString>());
    else if (key == "labelText")
        aspect->setLabelText(value.as<QString>());
    else if (key == "toolTip")
        aspect->setToolTip(value.as<QString>());
    else if (key == "onValueChanged") {
        QObject::connect(aspect, &BaseAspect::changed, aspect, [func = value.as<sol::function>()]() {
            void_safe_call(func);
        });
    } else if (key == "onVolatileValueChanged") {
        QObject::connect(aspect,
                         &BaseAspect::volatileValueChanged,
                         aspect,
                         [func = value.as<sol::function>()] { void_safe_call(func); });
    } else if (key == "enabler")
        aspect->setEnabler(value.as<BoolAspect *>());
    else if (key == "macroExpander") {
        if (value.is<Null>())
            aspect->setMacroExpander(nullptr);
        else
            aspect->setMacroExpander(value.as<MacroExpander *>());
    } else
        qWarning() << "Unknown key:" << key.c_str();
}

template<class T>
void typedAspectCreate(T *aspect, const std::string &key, const sol::object &value)
{
    if (key == "defaultValue")
        aspect->setDefaultValue(value.as<typename T::valueType>());
    else if (key == "value")
        aspect->setValue(value.as<typename T::valueType>());
    else
        baseAspectCreate(aspect, key, value);
}

template<>
void typedAspectCreate(StringAspect *aspect, const std::string &key, const sol::object &value)
{
    if (key == "displayStyle")
        aspect->setDisplayStyle((StringAspect::DisplayStyle) value.as<int>());
    else if (key == "historyId")
        aspect->setHistoryCompleter(value.as<QString>().toLocal8Bit());
    else if (key == "valueAcceptor")
        aspect->setValueAcceptor([func = value.as<sol::function>()](const QString &oldValue,
                                                                    const QString &newValue)
                                     -> std::optional<QString> {
            auto res = safe_call<std::optional<QString>>(func, oldValue, newValue);
            QTC_ASSERT_EXPECTED(res, return std::nullopt);
            return *res;
        });
    else if (key == "showToolTipOnLabel")
        aspect->setShowToolTipOnLabel(value.as<bool>());
    else if (key == "displayFilter")
        aspect->setDisplayFilter([func = value.as<sol::function>()](const QString &value) {
            auto res = safe_call<QString>(func, value);
            QTC_ASSERT_EXPECTED(res, return value);
            return *res;
        });
    else if (key == "placeHolderText")
        aspect->setPlaceHolderText(value.as<QString>());
    else if (key == "acceptRichText")
        aspect->setAcceptRichText(value.as<bool>());
    else if (key == "autoApplyOnEditingFinished")
        aspect->setAutoApplyOnEditingFinished(value.as<bool>());
    else if (key == "elideMode")
        aspect->setElideMode((Qt::TextElideMode) value.as<int>());
    else
        typedAspectCreate(static_cast<TypedAspect<QString> *>(aspect), key, value);
}

template<>
void typedAspectCreate(FilePathAspect *aspect, const std::string &key, const sol::object &value)
{
    if (key == "defaultPath")
        aspect->setDefaultPathValue(value.as<FilePath>());
    else if (key == "historyId")
        aspect->setHistoryCompleter(value.as<QString>().toLocal8Bit());
    else if (key == "promptDialogFilter")
        aspect->setPromptDialogFilter(value.as<QString>());
    else if (key == "promptDialogTitle")
        aspect->setPromptDialogTitle(value.as<QString>());
    else if (key == "commandVersionArguments")
        aspect->setCommandVersionArguments(value.as<QStringList>());
    else if (key == "allowPathFromDevice")
        aspect->setAllowPathFromDevice(value.as<bool>());
    else if (key == "validatePlaceHolder")
        aspect->setValidatePlaceHolder(value.as<bool>());
    else if (key == "openTerminalHandler")
        aspect->setOpenTerminalHandler([func = value.as<sol::function>()]() {
            auto res = void_safe_call(func);
            QTC_CHECK_EXPECTED(res);
        });
    else if (key == "expectedKind")
        aspect->setExpectedKind((PathChooser::Kind) value.as<int>());
    else if (key == "environment")
        aspect->setEnvironment(value.as<Environment>());
    else if (key == "baseFileName")
        aspect->setBaseFileName(value.as<FilePath>());
    else if (key == "valueAcceptor")
        aspect->setValueAcceptor([func = value.as<sol::function>()](const QString &oldValue,
                                                                    const QString &newValue)
                                     -> std::optional<QString> {
            auto res = safe_call<std::optional<QString>>(func, oldValue, newValue);
            QTC_ASSERT_EXPECTED(res, return std::nullopt);
            return *res;
        });
    else if (key == "showToolTipOnLabel")
        aspect->setShowToolTipOnLabel(value.as<bool>());
    else if (key == "autoApplyOnEditingFinished")
        aspect->setAutoApplyOnEditingFinished(value.as<bool>());
    /*else if (key == "validationFunction")
        aspect->setValidationFunction(
            [func = value.as<sol::function>()](const QString &path) {
                return func.call<std::optional<QString>>(path);
            });
    */
    else if (key == "displayFilter")
        aspect->setDisplayFilter([func = value.as<sol::function>()](const QString &path) {
            auto res = safe_call<QString>(func, path);
            QTC_ASSERT_EXPECTED(res, return path);
            return *res;
        });
    else if (key == "placeHolderText")
        aspect->setPlaceHolderText(value.as<QString>());
    else
        typedAspectCreate(static_cast<TypedAspect<QString> *>(aspect), key, value);
}

template<>
void typedAspectCreate(BoolAspect *aspect, const std::string &key, const sol::object &value)
{
    if (key == "labelPlacement") {
        aspect->setLabelPlacement((BoolAspect::LabelPlacement) value.as<int>());
    } else {
        typedAspectCreate(static_cast<TypedAspect<bool> *>(aspect), key, value);
    }
}

template<class T>
std::unique_ptr<T> createAspectFromTable(
    const sol::table &options, const std::function<void(T *, const std::string &, sol::object)> &f)
{
    auto aspect = std::make_unique<T>();

    for (const auto &[k, v] : options) {
        if (k.template is<std::string>()) {
            f(aspect.get(), k.template as<std::string>(), v);
        }
    }

    return aspect;
}

template<class T>
void addTypedAspectBaseBindings(sol::table &lua)
{
    lua.new_usertype<TypedAspect<T>>("TypedAspect<bool>",
                                     "value",
                                     sol::property(&TypedAspect<T>::value,
                                                   [](TypedAspect<T> *a, const T &v) {
                                                       a->setValue(v);
                                                   }),
                                     "volatileValue",
                                     sol::property(&TypedAspect<T>::volatileValue,
                                                   [](TypedAspect<T> *a, const T &v) {
                                                       a->setVolatileValue(v);
                                                   }),
                                     "defaultValue",
                                     sol::property(&TypedAspect<T>::defaultValue),
                                     sol::base_classes,
                                     sol::bases<BaseAspect>());
}

template<class T>
sol::usertype<T> addTypedAspect(sol::table &lua, const QString &name)
{
    addTypedAspectBaseBindings<typename T::valueType>(lua);

    return lua.new_usertype<T>(
        name,
        "create",
        [](const sol::table &options) {
            return createAspectFromTable<T>(options, &typedAspectCreate<T>);
        },
        sol::base_classes,
        sol::bases<TypedAspect<typename T::valueType>, BaseAspect>());
}

void setupSettingsModule()
{
    registerProvider("Settings", [](sol::state_view lua) -> sol::object {
        const ScriptPluginSpec *pluginSpec = lua.get<ScriptPluginSpec *>("PluginSpec");

        sol::table settings = lua.create_table();

        settings.new_usertype<BaseAspect>("Aspect", "apply", &BaseAspect::apply);

        settings.new_usertype<LuaAspectContainer>(
            "AspectContainer",
            "create",
            &aspectContainerCreate,
            "apply",
            &LuaAspectContainer::apply,
            sol::meta_function::index,
            &LuaAspectContainer::dynamic_get,
            sol::meta_function::new_index,
            &LuaAspectContainer::dynamic_set,
            sol::meta_function::length,
            &LuaAspectContainer::size,
            sol::base_classes,
            sol::bases<AspectContainer, BaseAspect>());

        addTypedAspect<BoolAspect>(settings, "BoolAspect");
        addTypedAspect<ColorAspect>(settings, "ColorAspect");
        addTypedAspect<MultiSelectionAspect>(settings, "MultiSelectionAspect");
        addTypedAspect<StringAspect>(settings, "StringAspect");

        settings.new_usertype<SelectionAspect>(
            "SelectionAspect",
            "create",
            [](const sol::table &options) {
                return createAspectFromTable<SelectionAspect>(
                    options,
                    [](SelectionAspect *aspect, const std::string &key, const sol::object &value) {
                        if (key == "options") {
                            sol::table options = value.as<sol::table>();
                            for (size_t i = 1; i <= options.size(); ++i) {
                                sol::optional<sol::table> optiontable
                                    = options[i].get<sol::optional<sol::table>>();
                                if (optiontable) {
                                    sol::table option = *optiontable;
                                    sol::optional<sol::object> data = option["data"];
                                    if (data) {
                                        aspect->addOption(
                                            {option["name"],
                                             option["tooltip"].get_or(QString()),
                                             QVariant::fromValue(*data)});
                                    } else {
                                        aspect->addOption(
                                            option["name"], option["tooltip"].get_or(QString()));
                                    }
                                } else if (
                                    sol::optional<QString> name
                                    = options[i].get<sol::optional<QString>>()) {
                                    aspect->addOption(*name);
                                } else {
                                    throw sol::error("Invalid option type");
                                }
                            }
                        } else if (key == "displayStyle") {
                            aspect->setDisplayStyle((SelectionAspect::DisplayStyle) value.as<int>());
                        } else
                            typedAspectCreate(aspect, key, value);
                    });
            },
            "stringValue",
            sol::property(&SelectionAspect::stringValue, &SelectionAspect::setStringValue),
            "dataValue",
            sol::property([](SelectionAspect *aspect) {
                return qvariant_cast<sol::object>(aspect->itemValue());
            }),
            "addOption",
            sol::overload(
                [](SelectionAspect &self, const QString &name) { self.addOption(name); },
                [](SelectionAspect &self, const QString &name, const QString &tooltip) {
                    self.addOption(name, tooltip);
                },
                [](SelectionAspect &self,
                   const QString &name,
                   const QString &tooltip,
                   const sol::object &data) {
                    self.addOption({name, tooltip, QVariant::fromValue(data)});
                }),
            sol::base_classes,
            sol::bases<TypedAspect<int>, BaseAspect>());

        auto filePathAspectType = addTypedAspect<FilePathAspect>(settings, "FilePathAspect");
        filePathAspectType.set(
            "setValue",
            sol::overload(
                [](FilePathAspect &self, const QString &value) {
                    self.setValue(FilePath::fromUserInput(value));
                },
                [](FilePathAspect &self, const FilePath &value) { self.setValue(value); }));
        filePathAspectType.set("expandedValue", sol::property(&FilePathAspect::expandedValue));
        filePathAspectType.set(
            "defaultPath",
            sol::property(
                [](FilePathAspect &self) { return FilePath::fromUserInput(self.defaultValue()); },
                &FilePathAspect::setDefaultPathValue));

        addTypedAspect<IntegerAspect>(settings, "IntegerAspect");
        addTypedAspect<DoubleAspect>(settings, "DoubleAspect");
        addTypedAspect<StringListAspect>(settings, "StringListAspect");
        addTypedAspect<FilePathListAspect>(settings, "FilePathListAspect");
        addTypedAspect<IntegersAspect>(settings, "IntegersAspect");
        addTypedAspect<StringSelectionAspect>(settings, "StringSelectionAspect");

        settings.new_usertype<ToggleAspect>(
            "ToggleAspect",
            "create",
            [](const sol::table &options) {
                return createAspectFromTable<ToggleAspect>(
                    options,
                    [](ToggleAspect *aspect, const std::string &key, const sol::object &value) {
                        if (key == "offIcon")
                            aspect->setOffIcon(QIcon(value.as<QString>()));
                        else if (key == "offTooltip")
                            aspect->setOffTooltip(value.as<QString>());
                        else if (key == "onIcon")
                            aspect->setOnIcon(QIcon(value.as<QString>()));
                        else if (key == "onTooltip")
                            aspect->setOnTooltip(value.as<QString>());
                        else if (key == "onText")
                            aspect->setOnText(value.as<QString>());
                        else if (key == "offText")
                            aspect->setOffText(value.as<QString>());
                        else
                            typedAspectCreate(aspect, key, value);
                    });
            },
            "action",
            &ToggleAspect::action,
            sol::base_classes,
            sol::bases<BoolAspect, TypedAspect<bool>, BaseAspect>());

        static auto triStateFromString = [](const QString &str) -> TriState {
            const QString l = str.toLower();
            if (l == "enabled")
                return TriState::Enabled;
            else if (l == "disabled")
                return TriState::Disabled;
            else if (l == "default")
                return TriState::Default;
            else
                return TriState::Default;
        };

        static auto triStateToString = [](TriState state) -> QString {
            if (state == TriState::Enabled)
                return "enabled";
            else if (state == TriState::Disabled)
                return "disabled";
            return "default";
        };

        settings.new_usertype<TriStateAspect>(
            "TriStateAspect",
            "create",
            [](const sol::table &options) {
                return createAspectFromTable<TriStateAspect>(
                    options,
                    [](TriStateAspect *aspect, const std::string &key, const sol::object &value) {
                        if (key == "defaultValue")
                            aspect->setDefaultValue(triStateFromString(value.as<QString>()));
                        else if (key == "value")
                            aspect->setValue(triStateFromString(value.as<QString>()));
                        else
                            baseAspectCreate(aspect, key, value);
                    });
            },
            "value",
            sol::property(
                [](TriStateAspect *a) { return triStateToString(a->value()); },
                [](TriStateAspect *a, const QString &v) { a->setValue(triStateFromString(v)); }),
            "volatileValue",
            sol::property(
                [](TriStateAspect *a) {
                    return triStateToString(TriState::fromInt(a->volatileValue()));
                },
                [](TriStateAspect *a, const QString &v) {
                    a->setVolatileValue(triStateFromString(v).toInt());
                }),
            "defaultValue",
            sol::property([](TriStateAspect *a) { return triStateToString(a->defaultValue()); }),
            sol::base_classes,
            sol::bases<SelectionAspect, TypedAspect<int>, BaseAspect>());

        settings.new_usertype<TextDisplay>(
            "TextDisplay",
            "create",
            [](const sol::table &options) {
                return createAspectFromTable<TextDisplay>(
                    options,
                    [](TextDisplay *aspect, const std::string &key, const sol::object &value) {
                        if (key == "text") {
                            aspect->setText(value.as<QString>());
                        } else if (key == "iconType") {
                            const QString type = value.as<QString>().toLower();

                            if (type.isEmpty() || type == "None")
                                aspect->setIconType(InfoLabel::InfoType::None);
                            else if (type == "information")
                                aspect->setIconType(InfoLabel::InfoType::Information);
                            else if (type == "warning")
                                aspect->setIconType(InfoLabel::InfoType::Warning);
                            else if (type == "error")
                                aspect->setIconType(InfoLabel::InfoType::Error);
                            else if (type == "ok")
                                aspect->setIconType(InfoLabel::InfoType::Ok);
                            else if (type == "notok")
                                aspect->setIconType(InfoLabel::InfoType::NotOk);
                            else
                                aspect->setIconType(InfoLabel::InfoType::None);
                        } else {
                            baseAspectCreate(aspect, key, value);
                        }
                    });
            },
            sol::base_classes,
            sol::bases<BaseAspect>());

        settings.new_usertype<AspectList>(
            "AspectList",
            "create",
            [](const sol::table &options) {
                return createAspectFromTable<AspectList>(
                    options,
                    [](AspectList *aspect, const std::string &key, const sol::object &value) {
                        if (key == "createItemFunction") {
                            aspect->setCreateItemFunction(
                                [func = value.as<sol::function>()]() -> std::shared_ptr<BaseAspect> {
                                    auto res = safe_call<std::shared_ptr<BaseAspect>>(func);
                                    QTC_ASSERT_EXPECTED(res, return nullptr);
                                    return *res;
                                });
                        } else if (key == "onItemAdded") {
                            aspect->setItemAddedCallback([func = value.as<sol::function>()](
                                                             std::shared_ptr<BaseAspect> item) {
                                auto res = void_safe_call(func, item);
                                QTC_CHECK_EXPECTED(res);
                            });
                        } else if (key == "onItemRemoved") {
                            aspect->setItemRemovedCallback([func = value.as<sol::function>()](
                                                               std::shared_ptr<BaseAspect> item) {
                                auto res = void_safe_call(func, item);
                                QTC_CHECK_EXPECTED(res);
                            });
                        } else {
                            baseAspectCreate(aspect, key, value);
                        }
                    });
            },
            "createAndAddItem",
            &AspectList::createAndAddItem,
            "foreach",
            [](AspectList *a, const sol::function &clbk) {
                a->forEachItem<BaseAspect>([clbk](std::shared_ptr<BaseAspect> item) {
                    auto res = void_safe_call(clbk, item);
                    QTC_CHECK_EXPECTED(res);
                });
            },
            "enumerate",
            [](AspectList *a, const sol::function &clbk) {
                a->forEachItem<BaseAspect>([clbk](std::shared_ptr<BaseAspect> item, int idx) {
                    auto res = void_safe_call(clbk, item, idx);
                    QTC_CHECK_EXPECTED(res);
                });
            },
            sol::base_classes,
            sol::bases<BaseAspect>());

        class ExtensionOptionsPage : public Core::IOptionsPage
        {
        public:
            ExtensionOptionsPage(const ScriptPluginSpec *spec, AspectContainer *container)
            {
                setId(Id::fromString(QString("Extension.%2").arg(spec->id)));
                setCategory(Id("ExtensionManager"));

                setDisplayName(spec->name);

                if (container->isAutoApply())
                    throw sol::error("AspectContainer must have autoApply set to false");

                setSettingsProvider([container]() { return container; });
            }
        };

        class OptionsPage : public Core::IOptionsPage
        {
        public:
            OptionsPage(const ScriptPluginSpec *spec, const sol::table &options)
            {
                setId(
                    Id::fromString(QString("%1.%2").arg(spec->id).arg(options.get<QString>("id"))));
                setCategory(Id::fromString(
                    QString("%1.%2").arg(spec->id).arg(options.get<QString>("categoryId"))));

                setDisplayName(options.get<QString>("displayName"));
                setDisplayCategory(options.get<QString>("displayCategory"));

                const FilePath catIcon = options.get<std::optional<FilePath>>("categoryIconPath")
                                             .value_or(FilePath::fromUserInput(
                                                 options.get_or<QString>("categoryIconPath", {})));

                setCategoryIconPath(catIcon);

                AspectContainer *container = options.get<AspectContainer *>("aspectContainer");
                if (container->isAutoApply())
                    throw sol::error("AspectContainer must have autoApply set to false");

                setSettingsProvider([container]() { return container; });
            }
        };

        settings.new_usertype<OptionsPage>(
            "OptionsPage",
            "create",
            [pluginSpec](const sol::table &options) {
                return std::make_unique<OptionsPage>(pluginSpec, options);
            },
            "show",
            [](OptionsPage *page) { Core::ICore::showOptionsDialog(page->id()); });

        settings.new_usertype<ExtensionOptionsPage>(
            "ExtensionOptionsPage",
            "create",
            [pluginSpec](AspectContainer *container) {
                return std::make_unique<ExtensionOptionsPage>(pluginSpec, container);
            },
            "show",
            [](ExtensionOptionsPage *page) { Core::ICore::showOptionsDialog(page->id()); });

        // clang-format off
        settings["StringDisplayStyle"] = lua.create_table_with(
            "Label", StringAspect::DisplayStyle::LabelDisplay,
            "LineEdit", StringAspect::DisplayStyle::LineEditDisplay,
            "TextEdit", StringAspect::DisplayStyle::TextEditDisplay,
            "PasswordLineEdit", StringAspect::DisplayStyle::PasswordLineEditDisplay
        );

        settings["SelectionDisplayStyle"] = lua.create_table_with(
            "RadioButtons", SelectionAspect::DisplayStyle::RadioButtons,
            "ComboBox", SelectionAspect::DisplayStyle::ComboBox
        );

        settings["CheckBoxPlacement"] = lua.create_table_with(
            "Top", CheckBoxPlacement::Top,
            "Right", CheckBoxPlacement::Right
        );
        settings["Kind"] = lua.create_table_with(
            "ExistingDirectory", PathChooser::Kind::ExistingDirectory,
            "Directory", PathChooser::Kind::Directory,
            "File", PathChooser::Kind::File,
            "SaveFile", PathChooser::Kind::SaveFile,
            "ExistingCommand", PathChooser::Kind::ExistingCommand,
            "Command", PathChooser::Kind::Command,
            "Any", PathChooser::Kind::Any
        );
        settings["LabelPlacement"] = lua.create_table_with(
            "AtCheckBox", BoolAspect::LabelPlacement::AtCheckBox,
            "Compact", BoolAspect::LabelPlacement::Compact,
            "InExtraLabel", BoolAspect::LabelPlacement::InExtraLabel
        );
        // clang-format on

        return settings;
    });
}

} // namespace Lua::Internal
