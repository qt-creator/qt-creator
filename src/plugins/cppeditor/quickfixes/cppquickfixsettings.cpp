// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfixsettings.h"

#include "../cppeditorconstants.h"
#include "../cppeditortr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/jsexpander.h>

#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projecttree.h>

#include <utils/aspectlist.h>
#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcsettings.h>

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSpinBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

CppQuickFixSettings::CppQuickFixSettings(bool loadGlobalSettings)
{
    setDefaultSettings();
    if (loadGlobalSettings)
        this->loadGlobalSettings();
}

CppQuickFixSettings *globalCppQuickFixSettings()
{
    static CppQuickFixSettings settings(true);
    return &settings;
}

void CppQuickFixSettings::loadGlobalSettings()
{
    loadSettingsFrom(Core::ICore::settings());
}

void CppQuickFixSettings::loadSettingsFrom(QtcSettings *s)
{
    CppQuickFixSettings def;
    s->beginGroup(Constants::QUICK_FIX_SETTINGS_ID);
    getterOutsideClassFrom = s->value(Constants::QUICK_FIX_SETTING_GETTER_OUTSIDE_CLASS_FROM,
                                      def.getterOutsideClassFrom)
                                 .toInt();
    getterInCppFileFrom = s->value(Constants::QUICK_FIX_SETTING_GETTER_IN_CPP_FILE_FROM,
                                   def.getterInCppFileFrom)
                              .toInt();
    setterOutsideClassFrom = s->value(Constants::QUICK_FIX_SETTING_SETTER_OUTSIDE_CLASS_FROM,
                                      def.setterOutsideClassFrom)
                                 .toInt();
    setterInCppFileFrom = s->value(Constants::QUICK_FIX_SETTING_SETTER_IN_CPP_FILE_FROM,
                                   def.setterInCppFileFrom)
                              .toInt();
    getterAttributes
        = s->value(Constants::QUICK_FIX_SETTING_GETTER_ATTRIBUTES, def.getterAttributes).toString();
    getterNameTemplate = s->value(Constants::QUICK_FIX_SETTING_GETTER_NAME_TEMPLATE,
                                  def.getterNameTemplate)
                             .toString();
    setterNameTemplate = s->value(Constants::QUICK_FIX_SETTING_SETTER_NAME_TEMPLATE,
                                  def.setterNameTemplate)
                             .toString();
    setterParameterNameTemplate = s->value(Constants::QUICK_FIX_SETTING_SETTER_PARAMETER_NAME,
                                           def.setterParameterNameTemplate)
                                      .toString();
    resetNameTemplate = s->value(Constants::QUICK_FIX_SETTING_RESET_NAME_TEMPLATE,
                                 def.resetNameTemplate)
                            .toString();
    signalNameTemplate = s->value(Constants::QUICK_FIX_SETTING_SIGNAL_NAME_TEMPLATE,
                                  def.signalNameTemplate)
                             .toString();
    signalWithNewValue = s->value(Constants::QUICK_FIX_SETTING_SIGNAL_WITH_NEW_VALUE,
                                  def.signalWithNewValue)
                             .toBool();
    setterAsSlot = s->value(Constants::QUICK_FIX_SETTING_SETTER_AS_SLOT, def.setterAsSlot).toBool();
    cppFileNamespaceHandling = static_cast<MissingNamespaceHandling>(
        s->value(Constants::QUICK_FIX_SETTING_CPP_FILE_NAMESPACE_HANDLING,
                 static_cast<int>(def.cppFileNamespaceHandling))
            .toInt());
    useAuto = s->value(Constants::QUICK_FIX_SETTING_USE_AUTO, def.useAuto).toBool();

    memberVariableNameTemplate = s->value(Constants::QUICK_FIX_SETTING_MEMBER_VARIABLE_NAME_TEMPLATE,
                                          def.memberVariableNameTemplate)
                                     .toString();
    nameFromMemberVariableTemplate
        = s->value(Constants::QUICK_FIX_SETTING_REVERSE_MEMBER_VARIABLE_NAME_TEMPLATE,
                   def.nameFromMemberVariableTemplate)
              .toString();
    valueTypes = s->value(Constants::QUICK_FIX_SETTING_VALUE_TYPES, def.valueTypes).toStringList();
    returnByConstRef = s->value(Constants::QUICK_FIX_SETTING_RETURN_BY_CONST_REF,
                                def.returnByConstRef).toBool();
    customTemplates = def.customTemplates;
    int size = s->beginReadArray(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATES);
    if (size > 0)
        customTemplates.clear();

    for (int i = 0; i < size; ++i) {
        s->setArrayIndex(i);
        CustomTemplate c;
        c.types = s->value(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATE_TYPES).toStringList();
        if (c.types.isEmpty())
            continue;
        c.equalComparison = s->value(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATE_COMPARISON)
                                .toString();
        c.returnType = s->value(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATE_RETURN_TYPE).toString();
        c.returnExpression
            = s->value(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATE_RETURN_EXPRESSION).toString();
        c.assignment = s->value(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATE_ASSIGNMENT).toString();
        if (c.assignment.isEmpty() && c.returnType.isEmpty() && c.equalComparison.isEmpty())
            continue; // nothing custom here

        customTemplates.push_back(c);
    }
    s->endArray();
    s->endGroup();
}

void CppQuickFixSettings::saveSettingsTo(QtcSettings *s)
{
    CppQuickFixSettings def;
    s->beginGroup(Constants::QUICK_FIX_SETTINGS_ID);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_GETTER_OUTSIDE_CLASS_FROM,
                           getterOutsideClassFrom,
                           def.getterOutsideClassFrom);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_GETTER_IN_CPP_FILE_FROM,
                           getterInCppFileFrom,
                           def.getterInCppFileFrom);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_SETTER_OUTSIDE_CLASS_FROM,
                           setterOutsideClassFrom,
                           def.setterOutsideClassFrom);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_SETTER_IN_CPP_FILE_FROM,
                           setterInCppFileFrom,
                           def.setterInCppFileFrom);

    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_GETTER_ATTRIBUTES,
                           getterAttributes,
                           def.getterAttributes);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_GETTER_NAME_TEMPLATE,
                           getterNameTemplate,
                           def.getterNameTemplate);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_SETTER_NAME_TEMPLATE,
                           setterNameTemplate,
                           def.setterNameTemplate);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_RESET_NAME_TEMPLATE,
                           resetNameTemplate,
                           def.resetNameTemplate);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_SIGNAL_NAME_TEMPLATE,
                           signalNameTemplate,
                           def.signalNameTemplate);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_SIGNAL_WITH_NEW_VALUE,
                           signalWithNewValue,
                           def.signalWithNewValue);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_CPP_FILE_NAMESPACE_HANDLING,
                           int(cppFileNamespaceHandling),
                           int(def.cppFileNamespaceHandling));
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_MEMBER_VARIABLE_NAME_TEMPLATE,
                           memberVariableNameTemplate,
                           def.memberVariableNameTemplate);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_REVERSE_MEMBER_VARIABLE_NAME_TEMPLATE,
                           nameFromMemberVariableTemplate,
                           def.nameFromMemberVariableTemplate);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_SETTER_PARAMETER_NAME,
                           setterParameterNameTemplate,
                           def.setterParameterNameTemplate);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_SETTER_AS_SLOT,
                           setterAsSlot,
                           def.setterAsSlot);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_USE_AUTO,
                           useAuto,
                           def.useAuto);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_VALUE_TYPES,
                           valueTypes,
                           def.valueTypes);
    s->setValueWithDefault(Constants::QUICK_FIX_SETTING_RETURN_BY_CONST_REF,
                           returnByConstRef,
                           def.returnByConstRef);
    if (customTemplates == def.customTemplates) {
        s->remove(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATES);
    } else {
        s->beginWriteArray(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATES);
        for (int i = 0; i < static_cast<int>(customTemplates.size()); ++i) {
            const auto &c = customTemplates[i];
            s->setArrayIndex(i);
            s->setValue(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATE_TYPES, c.types);
            s->setValue(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATE_COMPARISON, c.equalComparison);
            s->setValue(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATE_RETURN_TYPE, c.returnType);
            s->setValue(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATE_RETURN_EXPRESSION,
                        c.returnExpression);
            s->setValue(Constants::QUICK_FIX_SETTING_CUSTOM_TEMPLATE_ASSIGNMENT, c.assignment);
        }
        s->endArray();
    }
    s->endGroup();
}

void CppQuickFixSettings::saveAsGlobalSettings()
{
    saveSettingsTo(Core::ICore::settings());
}

void CppQuickFixSettings::setDefaultSettings()
{
    valueTypes << "Pointer"     // for Q...Pointer
               << "optional"    // for ...::optional
               << "unique_ptr"; // for std::unique_ptr and boost::movelib::unique_ptr
    valueTypes << "int"
               << "long"
               << "char"
               << "real"
               << "short"
               << "unsigned"
               << "size"
               << "float"
               << "double"
               << "bool";
    CustomTemplate floatingPoint;
    floatingPoint.types << "float"
                        << "double"
                        << "qreal"
                        << "long double";
    floatingPoint.equalComparison = "qFuzzyCompare(<cur>, <new>)";
    customTemplates.push_back(floatingPoint);

    CustomTemplate unique_ptr;
    unique_ptr.types << "unique_ptr";
    unique_ptr.assignment = "<cur> = std::move(<new>)";
    unique_ptr.returnType = "<T>*";
    unique_ptr.returnExpression = "<cur>.get()";
    customTemplates.push_back(unique_ptr);
}

QString CppQuickFixSettings::memberBaseName(
    const QString &name, const std::optional<QString> &baseNameTemplate)
{
    const auto validName = [](const QString &name) {
        return !name.isEmpty() && !name.at(0).isDigit();
    };
    QString baseName = name;

    QString baseNameTemplateValue;
    if (baseNameTemplate) {
        baseNameTemplateValue = *baseNameTemplate;
    } else {
        const CppQuickFixSettings *const settings
            = Internal::cppQuickFixSettingsForProject(ProjectTree::currentProject());
        baseNameTemplateValue = settings->nameFromMemberVariableTemplate;
    }
    if (!baseNameTemplateValue.isEmpty())
        return CppQuickFixSettings::replaceNamePlaceholders(baseNameTemplateValue, name, {});

    // Remove leading and trailing "_"
    while (baseName.startsWith(QLatin1Char('_')))
        baseName.remove(0, 1);
    while (baseName.endsWith(QLatin1Char('_')))
        baseName.chop(1);
    if (baseName != name && validName(baseName))
        return baseName;

    // If no leading/trailing "_": remove "m_" and "m" prefix
    if (baseName.startsWith(QLatin1String("m_"))) {
        baseName.remove(0, 2);
    } else if (baseName.startsWith(QLatin1Char('m')) && baseName.size() > 1
               && baseName.at(1).isUpper()) {
        baseName.remove(0, 1);
        baseName[0] = baseName.at(0).toLower();
    }

    return validName(baseName) ? baseName : name;

}

QString CppQuickFixSettings::replaceNamePlaceholders(
    const QString &nameTemplate, const QString &name, const std::optional<QString> &memberName)
{
    Core::JsExpander expander;
    QString jsError;
    QString jsExpr;
    if (memberName) {
        QTC_CHECK(!memberName->isEmpty());
        jsExpr = QString("(function(name, memberName) { return %1; })(\"%2\", \"%3\")")
                     .arg(nameTemplate, name, *memberName);
    } else {
        jsExpr = QString("(function(name) { return %1; })(\"%2\")").arg(nameTemplate, name);
    }
    const QString jsRes = expander.evaluate(jsExpr, &jsError);
    if (!jsError.isEmpty())
        return jsError; // TODO: Use Utils::Result?
    return jsRes;
}

static auto removeAndExtractTemplate(QString type)
{
    // maybe we have somethink like: myName::test<std::byte>::fancy<std::optional<int>>, then we want fancy
    QString realType;
    QString templateParameter;
    int counter = 0;
    int start = 0;
    int templateStart = 0;
    for (int i = 0; i < type.size(); ++i) {
        auto c = type[i];
        if (c == '<') {
            if (counter == 0) {
                // template start
                realType += type.mid(start, i - start);
                templateStart = i + 1;
            }
            ++counter;
        } else if (c == '>') {
            --counter;
            if (counter == 0) {
                // template ends
                start = i + 1;
                templateParameter = type.mid(templateStart, i - templateStart);
            }
        }
    }
    if (start < type.size()) // add last block if there is one
        realType += type.mid(start);

    struct _
    {
        QString type;
        QString templateParameter;
    };
    return _{realType, templateParameter};
}

static auto withoutNamespace(QString type)
{
    const auto namespaceIndex = type.lastIndexOf("::");
    if (namespaceIndex >= 0)
        return type.mid(namespaceIndex + 2);
    return type;
}

bool CppQuickFixSettings::isValueType(QString type) const
{
    // first remove template stuff
    auto realType = removeAndExtractTemplate(type).type;
    // remove namespaces: namespace_int::complex should not be matched by int
    realType = withoutNamespace(realType);
    for (const auto &valueType : valueTypes) {
        if (realType.contains(valueType))
            return true;
    }
    return false;
}

void CppQuickFixSettings::GetterSetterTemplate::replacePlaceholders(QString currentValueVariableName,
                                                                    QString newValueVariableName)
{
    equalComparison = equalComparison.replace("<new>", newValueVariableName)
                          .replace("<cur>", currentValueVariableName);
    assignment = assignment.replace("<new>", newValueVariableName)
                     .replace("<cur>", currentValueVariableName);
    returnExpression = returnExpression.replace("<new>", newValueVariableName)
                           .replace("<cur>", currentValueVariableName);
}

CppQuickFixSettings::GetterSetterTemplate CppQuickFixSettings::findGetterSetterTemplate(
    QString fullyQualifiedType) const
{
    const int index = fullyQualifiedType.lastIndexOf("::");
    const QString namespaces = index >= 0 ? fullyQualifiedType.left(index) : "";
    const QString typeOnly = index >= 0 ? fullyQualifiedType.mid(index + 2) : fullyQualifiedType;
    CustomTemplate bestMatch;
    enum MatchType { FullyExact, FullyContains, Exact, Contains, None } currentMatch = None;
    QRegularExpression regex;
    for (const auto &cTemplate : customTemplates) {
        for (const auto &t : cTemplate.types) {
            QString type = t;
            bool fully = false;
            if (t.contains("::")) {
                const int index = t.lastIndexOf("::");
                if (t.left(index) != namespaces)
                    continue;

                type = t.mid(index + 2);
                fully = true;
            } else if (currentMatch <= FullyContains) {
                continue;
            }

            MatchType match = None;
            if (t.contains("*")) {
                regex.setPattern('^' + QString(t).replace('*', ".*") + '$');
                if (regex.match(typeOnly).isValid() != QRegularExpression::NormalMatch)
                    match = fully ? FullyContains : Contains;
            } else if (t == typeOnly) {
                match = fully ? FullyExact : Exact;
            }
            if (match < currentMatch) {
                currentMatch = match;
                bestMatch = cTemplate;
            }
        }
    }

    if (currentMatch != None) {
        GetterSetterTemplate t;
        if (!bestMatch.equalComparison.isEmpty())
            t.equalComparison = bestMatch.equalComparison;
        if (!bestMatch.returnExpression.isEmpty())
            t.returnExpression = bestMatch.returnExpression;
        if (!bestMatch.assignment.isEmpty())
            t.assignment = bestMatch.assignment;
        if (!bestMatch.returnType.isEmpty())
            t.returnTypeTemplate = bestMatch.returnType;
        return t;
    }
    return GetterSetterTemplate{};
}

CppQuickFixSettings::FunctionLocation CppQuickFixSettings::determineGetterLocation(int lineCount) const
{
    int outsideDiff = getterOutsideClassFrom > 0 ? lineCount - getterOutsideClassFrom : -1;
    int cppDiff = getterInCppFileFrom > 0 ? lineCount - getterInCppFileFrom : -1;
    if (outsideDiff > cppDiff) {
        if (outsideDiff >= 0)
            return FunctionLocation::OutsideClass;
    }
    return cppDiff >= 0 ? FunctionLocation::CppFile : FunctionLocation::InsideClass;
}

CppQuickFixSettings::FunctionLocation CppQuickFixSettings::determineSetterLocation(int lineCount) const
{
    int outsideDiff = setterOutsideClassFrom > 0 ? lineCount - setterOutsideClassFrom : -1;
    int cppDiff = setterInCppFileFrom > 0 ? lineCount - setterInCppFileFrom : -1;
    if (outsideDiff > cppDiff) {
        if (outsideDiff >= 0)
            return FunctionLocation::OutsideClass;
    }
    return cppDiff >= 0 ? FunctionLocation::CppFile : FunctionLocation::InsideClass;
}

namespace Internal {

using namespace Constants;

const char SETTINGS_FILE_NAME[] = ".cppQuickFix";
const char USE_GLOBAL_SETTINGS[] = "UseGlobalSettings";

CppQuickFixProjectsSettings::CppQuickFixProjectsSettings(Project *project)
{
    m_project = project;
    useGlobalSettings.setSettingsPageId(Constants::QUICK_FIX_SETTINGS_ID);
    const auto settings = storeFromVariant(m_project->namedSettings(QUICK_FIX_SETTINGS_ID));
    // if no option is saved try to load settings from a file
    const bool global = settings.value(USE_GLOBAL_SETTINGS, false).toBool();
    if (!global) {
        m_settingsFile = searchForCppQuickFixSettingsFile();
        if (!m_settingsFile.isEmpty()) {
            loadOwnSettingsFromFile();
            useGlobalSettings.setValue(false);
        } else {
            useGlobalSettings.setValue(true);
        }
    } else {
        useGlobalSettings.setValue(true);
    }
    connect(project, &Project::aboutToSaveSettings, this, [this] {
        auto settings = m_project->namedSettings(QUICK_FIX_SETTINGS_ID).toMap();
        settings.insert(USE_GLOBAL_SETTINGS, useGlobalSettings());
        m_project->setNamedSettings(QUICK_FIX_SETTINGS_ID, settings);
    });
}

CppQuickFixSettings *CppQuickFixProjectsSettings::getSettings()
{
    if (useGlobalSettings())
        return globalCppQuickFixSettings();

    return &m_ownSettings;
}

const FilePath &CppQuickFixProjectsSettings::filePathOfSettingsFile() const
{
    return m_settingsFile;
}

CppQuickFixProjectsSettings::CppQuickFixProjectsSettingsPtr cppQuickFixProjectSettings(
    Project *project)
{
    const Key key = "CppQuickFixProjectsSettings";
    QVariant v = project->extraData(key);
    if (v.isNull()) {
        v = QVariant::fromValue(
            CppQuickFixProjectsSettings::CppQuickFixProjectsSettingsPtr{
                new CppQuickFixProjectsSettings(project)});
        project->setExtraData(key, v);
    }
    return v.value<QSharedPointer<CppQuickFixProjectsSettings>>();
}

CppQuickFixSettings *cppQuickFixSettingsForProject(Project *project)
{
    if (project)
        return cppQuickFixProjectSettings(project)->getSettings();
    return globalCppQuickFixSettings();
}

FilePath CppQuickFixProjectsSettings::searchForCppQuickFixSettingsFile()
{
    return m_project->projectDirectory().searchHereAndInParents(SETTINGS_FILE_NAME, DirFilterFlag::Files);
}

bool CppQuickFixProjectsSettings::useCustomSettings()
{
    if (m_settingsFile.isEmpty()) {
        m_settingsFile = searchForCppQuickFixSettingsFile();
        const FilePath defaultLocation = m_project->projectDirectory() / SETTINGS_FILE_NAME;
        if (m_settingsFile.isEmpty()) {
            m_settingsFile = defaultLocation;
        } else if (m_settingsFile != defaultLocation) {
            QMessageBox msgBox(Core::ICore::dialogParent());
            msgBox.setText(Tr::tr("Quick Fix settings are saved in a file. Existing settings file "
                                  "\"%1\" found. Should this file be used or a "
                                  "new one be created?")
                               .arg(m_settingsFile.toUrlishString()));
            QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
            cancel->setToolTip(Tr::tr("Switch Back to Global Settings"));
            QPushButton *useExisting = msgBox.addButton(Tr::tr("Use Existing"), QMessageBox::AcceptRole);
            useExisting->setToolTip(m_settingsFile.toUrlishString());
            QPushButton *createNew = msgBox.addButton(Tr::tr("Create New"), QMessageBox::ActionRole);
            createNew->setToolTip(defaultLocation.toUrlishString());
            msgBox.exec();
            if (msgBox.clickedButton() == createNew) {
                m_settingsFile = defaultLocation;
            } else if (msgBox.clickedButton() != useExisting) {
                m_settingsFile.clear();
                return false;
            }
        }

        resetOwnSettingsToGlobal();
    }
    if (m_settingsFile.exists())
        loadOwnSettingsFromFile();

    return true;
}

void CppQuickFixProjectsSettings::resetOwnSettingsToGlobal()
{
    m_ownSettings = *globalCppQuickFixSettings();
}

bool CppQuickFixProjectsSettings::saveOwnSettings()
{
    if (m_settingsFile.isEmpty())
        return false;

    QtcSettings settings(m_settingsFile.toUrlishString(), QSettings::IniFormat);
    if (settings.status() == QSettings::NoError) {
        m_ownSettings.saveSettingsTo(&settings);
        settings.sync();
        return settings.status() == QSettings::NoError;
    }
    m_settingsFile.clear();
    return false;
}

void CppQuickFixProjectsSettings::loadOwnSettingsFromFile()
{
    QtcSettings settings(m_settingsFile.toUrlishString(), QSettings::IniFormat);
    if (settings.status() == QSettings::NoError) {
        m_ownSettings.loadSettingsFrom(&settings);
        return;
    }
    m_settingsFile.clear();
}

// SettingsWidgets

class LineCountSpinBox : public QWidget
{
    Q_OBJECT

public:
    LineCountSpinBox(QWidget *parent = nullptr);

    int count() const;
    void setCount(int count);

signals:
    void changed();

private:
    void updateFields();

    QCheckBox *m_checkBox;
    QLabel *m_opLabel;
    QSpinBox *m_spinBox;
    QLabel *m_unitLabel;
};

LineCountSpinBox::LineCountSpinBox(QWidget *parent)
    : QWidget(parent)
{
    m_checkBox = new QCheckBox;
    m_opLabel = new QLabel(Tr::tr("\342\211\245"));
    m_spinBox = new QSpinBox;
    m_spinBox->setMinimum(1);
    m_unitLabel = new QLabel(Tr::tr("lines"));

    using namespace Layouting;
    Row { m_checkBox, m_opLabel, m_spinBox, m_unitLabel, noMargin }.attachTo(this);

    auto handleChange = [this] {
        updateFields();
        emit changed();
    };
    connect(m_checkBox, &QCheckBox::toggled, handleChange);
    connect(m_spinBox, &QSpinBox::valueChanged, handleChange);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

int LineCountSpinBox::count() const
{
    return m_spinBox->value() * (m_checkBox->isChecked() ? 1 : -1);
}

void LineCountSpinBox::setCount(int count)
{
    m_spinBox->setValue(std::abs(count));
    m_checkBox->setChecked(count > 0);
    updateFields();
}

void LineCountSpinBox::updateFields()
{
    const bool enabled = m_checkBox->isChecked();
    m_opLabel->setEnabled(enabled);
    m_spinBox->setEnabled(enabled);
    m_unitLabel->setEnabled(enabled);
}

// Signed line count edited through a LineCountSpinBox (checkbox + spin box).
class LineCountAspect final : public TypedAspect<int>
{
public:
    using TypedAspect::TypedAspect;

protected:
    void addToLayoutImpl(Layouting::Layout &parent) override
    {
        m_spinBox = createSubWidget<LineCountSpinBox>();
        m_spinBox->setCount(volatileValue());
        connect(m_spinBox, &LineCountSpinBox::changed, this, [this] { handleGuiChanged(); });
        parent.addItem(m_spinBox.data());
    }

    void volatileValueToGui() override
    {
        if (m_spinBox)
            m_spinBox->setCount(m_volatileValue);
    }

    bool guiToVolatileValue() override
    {
        if (!m_spinBox)
            return false;
        const int newValue = m_spinBox->count();
        const bool changed = newValue != m_volatileValue;
        m_volatileValue = newValue;
        return changed;
    }

private:
    QPointer<LineCountSpinBox> m_spinBox;
};

// One entry of the "Custom Getter Setter Templates" master/detail list.
class CustomTemplateAspect final : public AspectContainer
{
public:
    CustomTemplateAspect()
    {
        types.setToolTip(Tr::tr("Separate the types by comma."));
        types.setLabelText(Tr::tr("Types:"));
        comparison.setLabelText(Tr::tr("Comparison:"));
        assignment.setLabelText(Tr::tr("Assignment:"));
        returnExpression.setLabelText(Tr::tr("Return expression:"));
        returnType.setLabelText(Tr::tr("Return type:"));
        for (StringAspect *a : {&types, &comparison, &assignment, &returnExpression, &returnType})
            a->setDisplayStyle(StringAspect::LineEditDisplay);

        using namespace Layouting;
        setLayouter([this] {
            return Form {
                types, br,
                comparison, br,
                assignment, br,
                returnExpression, br,
                returnType, br,
                Label {
                    wordWrap(true),
                    text(Tr::tr("Use <new> and <cur> to access the parameter and current value. "
                                "Use <type> to access the type and <T> for the template "
                                "parameter.")),
                }, br,
            };
        });
    }

    void setTemplate(const CppQuickFixSettings::CustomTemplate &t)
    {
        types.setValue(t.types.join(", "));
        comparison.setValue(t.equalComparison);
        assignment.setValue(t.assignment);
        returnExpression.setValue(t.returnExpression);
        returnType.setValue(t.returnType);
    }

    CppQuickFixSettings::CustomTemplate toTemplate() const
    {
        static const QRegularExpression splitter("\\s*,\\s*");
        CppQuickFixSettings::CustomTemplate t;
        t.types = types.volatileValue().split(splitter, Qt::SkipEmptyParts);
        t.equalComparison = comparison.volatileValue();
        t.assignment = assignment.volatileValue();
        t.returnExpression = returnExpression.volatileValue();
        t.returnType = returnType.volatileValue();
        return t;
    }

    StringAspect types{this};
    StringAspect comparison{this};
    StringAspect assignment{this};
    StringAspect returnExpression{this};
    StringAspect returnType{this};
};

// Editor for CppQuickFixSettings, shared by the global options page and the
// per-project panel. CppQuickFixSettings stays the persistence format.
class CppQuickFixSettingsAspects final : public AspectContainer
{
public:
    CppQuickFixSettingsAspects();

    void loadFrom(const CppQuickFixSettings &s);
    void saveTo(CppQuickFixSettings &s) const;

    void apply() override;

    std::function<void()> committer;

    LineCountAspect getterOutsideClass{this};
    LineCountAspect getterInCppFile{this};
    LineCountAspect setterOutsideClass{this};
    LineCountAspect setterInCppFile{this};

    StringAspect getterAttributes{this};
    StringAspect getterName{this};
    StringAspect setterName{this};
    StringAspect setterParameter{this};
    StringAspect resetName{this};
    StringAspect signalName{this};
    StringAspect memberVariableName{this};
    StringAspect nameFromMemberVariable{this};

    BoolAspect setterAsSlot{this};
    BoolAspect signalWithNewValue{this};
    BoolAspect useAuto{this};
    BoolAspect returnByConstRef{this};

    SelectionAspect namespaceHandling{this};

    StringListAspect valueTypes{this};
    AspectList customTemplates{this};
};

CppQuickFixSettingsAspects::CppQuickFixSettingsAspects()
{
    setAutoApply(false);

    const CppQuickFixSettings def;
    const QString toolTip1 = "<html><body>"
        + Tr::tr("A JavaScript expression acting as the return value of a function with two "
                 "parameters <b>name</b> and <b>memberName</b>, where"
                 "<ul><li><b>name</b> is the \"semantic name\" as it would be used for a Qt "
                 "property</li>"
                 "<li><b>memberName</b> is the name of the member variable.</li></ul>")
        + "</body></html>";
    const QString toolTip2 = "<html><body>"
        + Tr::tr("A JavaScript expression acting as the return value of a function with a "
                 "parameter <b>name</b>, which is the \"semantic name\" as it would be used for a "
                 "Qt property.")
        + "</body></html>";

    getterAttributes.setDisplayStyle(StringAspect::LineEditDisplay);
    getterAttributes.setLabelText(Tr::tr("Getter attributes:"));
    getterAttributes.setPlaceHolderText(Tr::tr("For example, [[nodiscard]]"));

    const auto setupJsField = [](StringAspect &a, const QString &label, const QString &placeholder,
                                 const QString &toolTip) {
        a.setDisplayStyle(StringAspect::LineEditDisplay);
        a.setLabelText(label);
        if (!placeholder.isEmpty())
            a.setPlaceHolderText(placeholder);
        a.setToolTip(toolTip);
    };
    setupJsField(getterName, Tr::tr("Getter name:"), def.getterNameTemplate, toolTip1);
    setupJsField(setterName, Tr::tr("Setter name:"), def.setterNameTemplate, toolTip1);
    setupJsField(setterParameter, Tr::tr("Setter parameter name:"), def.setterParameterNameTemplate,
                 toolTip1);
    setupJsField(resetName, Tr::tr("Reset name:"), def.resetNameTemplate, toolTip1);
    setupJsField(signalName, Tr::tr("Signal name:"), def.signalNameTemplate, toolTip1);
    setupJsField(memberVariableName, Tr::tr("Member variable name:"),
                 def.memberVariableNameTemplate, toolTip2);
    setupJsField(nameFromMemberVariable, Tr::tr("Name from member variable:"), {},
                 Tr::tr("How to get from the member variable to the semantic name.\n"
                        "This is the reverse of the operation above.\n"
                        "Leave empty to apply heuristics."));

    setterAsSlot.setLabelText(Tr::tr("Setters should be slots"));
    signalWithNewValue.setLabelText(Tr::tr("Generate signals with the new value as parameter"));
    useAuto.setLabelText(Tr::tr("Use type \"auto\" when creating new variables"));
    useAuto.setToolTip(Tr::tr("Declares new variables with the type \"auto\". When "
                              "unchecked, the \"Assign to Local Variable\" quick fix "
                              "tries to derive the type of the expression. This might "
                              "fail for more complex types."));
    returnByConstRef.setLabelText(Tr::tr("Return non-value types by const reference"));

    namespaceHandling.setDisplayStyle(SelectionAspect::DisplayStyle::RadioButtons);
    namespaceHandling.addOption(Tr::tr("Generate missing namespaces"));
    namespaceHandling.addOption(Tr::tr("Add \"using namespace ...\""));
    namespaceHandling.addOption(Tr::tr("Rewrite types to match the existing namespaces"));

    valueTypes.setDisplayStyle(StringListAspect::DisplayStyle::ListView);
    valueTypes.setUiAllowAdding(true);
    valueTypes.setUiAllowRemoving(true);
    valueTypes.setUiAllowEditing(true);
    valueTypes.setToolTip(
        Tr::tr("Normally arguments get passed by const reference. If the Type is one of the "
               "following ones, the argument gets passed by value. Namespaces and template "
               "arguments are removed. The real Type must contain the given Type. For example, "
               "\"int\" matches \"int32_t\" but not \"vector<int>\". \"vector\" matches "
               "\"std::pmr::vector<int>\" but not \"std::optional<vector<int>>\""));

    customTemplates.setCreateItemFunction([] {
        auto item = std::make_shared<CustomTemplateAspect>();
        item->types.setValue("<type>");
        return item;
    });
    customTemplates.setDisplayStyle(AspectList::DisplayStyle::ListViewWithDetailsBeside);
    customTemplates.listViewDataCallback = [](CustomTemplateAspect *aspect, int role) -> QVariant {
        if (role == Qt::DisplayRole)
            return aspect->types.volatileValue();
        return {};
    };

    setLayouter([this] {
        using namespace Layouting;

        const auto ulLabel = [](const QString &text) {
            QLabel *label = new QLabel(text);
            QFont font = label->font();
            font.setUnderline(true);
            label->setFont(font);
            return label;
        };

        auto functionLocationsGrid = new QWidget;
        Grid {
            empty, ulLabel(Tr::tr("Generate Setters")), ulLabel(Tr::tr("Generate Getters")), br,
            Tr::tr("Inside class:"), Tr::tr("Default"), Tr::tr("Default"), br,
            Tr::tr("Outside class:"), setterOutsideClass, getterOutsideClass, br,
            Tr::tr("In .cpp file:"), setterInCppFile, getterInCppFile, br,
            noMargin,
        }.attachTo(functionLocationsGrid);
        if (QGridLayout *gl = qobject_cast<QGridLayout *>(functionLocationsGrid->layout()))
            gl->setHorizontalSpacing(48);

        const auto makeResultField = [] {
            auto field = new QLineEdit;
            field->setReadOnly(true);
            return field;
        };
        QLineEdit * const getterResult = makeResultField();
        QLineEdit * const setterResult = makeResultField();
        QLineEdit * const setterParameterResult = makeResultField();
        QLineEdit * const resetResult = makeResultField();
        QLineEdit * const signalResult = makeResultField();
        QLineEdit * const memberResult = makeResultField();
        QLineEdit * const nameFromMemberResult = makeResultField();
        auto jsTestInput = new QLineEdit;
        jsTestInput->setText("myValue");
        jsTestInput->setToolTip(
            Tr::tr("The content of the <b>name</b> parameter in the fields above, that is, the "
                   "\"semantic name\" without any prefix or suffix."));
        auto jsTestButton = new QPushButton(Tr::tr("Test"));
        auto hideJsTestButton = new QPushButton(Tr::tr("Hide Test Results"));

        const auto runTests = [=, this] {
            const QString memberName = CppQuickFixSettings::replaceNamePlaceholders(
                memberVariableName.volatileValue(), jsTestInput->text(), {});
            memberResult->show();
            memberResult->setText(memberName);
            const QList<std::pair<StringAspect *, QLineEdit *>> nameFields = {
                {&getterName, getterResult},
                {&setterName, setterResult},
                {&setterParameter, setterParameterResult},
                {&resetName, resetResult},
                {&signalName, signalResult}};
            for (const auto &[aspect, resultField] : nameFields) {
                resultField->show();
                resultField->setText(CppQuickFixSettings::replaceNamePlaceholders(
                    aspect->volatileValue(), jsTestInput->text(), memberName));
            }
            nameFromMemberResult->show();
            nameFromMemberResult->setText(CppQuickFixSettings::memberBaseName(
                memberResult->text(), nameFromMemberVariable.volatileValue()));
        };
        const auto hideResults = [=] {
            for (QLineEdit *field : {getterResult, setterResult, setterParameterResult, resetResult,
                                     signalResult, memberResult, nameFromMemberResult}) {
                field->hide();
            }
        };
        connect(jsTestButton, &QPushButton::clicked, this, runTests);
        connect(hideJsTestButton, &QPushButton::clicked, this, hideResults);
        hideResults();

        return Column {
            Group {
                title(Tr::tr("Generated Function Locations")),
                Row { functionLocationsGrid, st },
            },
            Group {
                title(Tr::tr("Getter Setter Generation Properties")),
                Form {
                    getterAttributes, br,
                    getterName, getterResult, br,
                    setterName, setterResult, br,
                    setterParameter, setterParameterResult, br,
                    setterAsSlot, br,
                    resetName, resetResult, br,
                    signalName, signalResult, br,
                    signalWithNewValue, br,
                    memberVariableName, memberResult, br,
                    nameFromMemberVariable, nameFromMemberResult, br,
                    Tr::tr("Test with example name:"), jsTestInput, jsTestButton, hideJsTestButton,
                        st, br,
                },
            },
            Group {
                title(Tr::tr("Missing Namespace Handling")),
                Column { namespaceHandling },
            },
            useAuto,
            Group {
                title(Tr::tr("Custom Getter Setter Templates")),
                Column { customTemplates },
            },
            Group {
                title(Tr::tr("Value Types")),
                Column { valueTypes },
            },
            returnByConstRef,
        };
    });
}

void CppQuickFixSettingsAspects::apply()
{
    AspectContainer::apply();
    if (committer)
        committer();
}

// The spin box cannot show 0 and would read it back as -1.
static int guiLineCount(int count)
{
    return count != 0 ? count : -1;
}

void CppQuickFixSettingsAspects::loadFrom(const CppQuickFixSettings &s)
{
    getterOutsideClass.setValue(guiLineCount(s.getterOutsideClassFrom));
    getterInCppFile.setValue(guiLineCount(s.getterInCppFileFrom));
    setterOutsideClass.setValue(guiLineCount(s.setterOutsideClassFrom));
    setterInCppFile.setValue(guiLineCount(s.setterInCppFileFrom));
    getterAttributes.setValue(s.getterAttributes);
    getterName.setValue(s.getterNameTemplate);
    setterName.setValue(s.setterNameTemplate);
    setterParameter.setValue(s.setterParameterNameTemplate);
    resetName.setValue(s.resetNameTemplate);
    signalName.setValue(s.signalNameTemplate);
    memberVariableName.setValue(s.memberVariableNameTemplate);
    nameFromMemberVariable.setValue(s.nameFromMemberVariableTemplate);
    setterAsSlot.setValue(s.setterAsSlot);
    signalWithNewValue.setValue(s.signalWithNewValue);
    useAuto.setValue(s.useAuto);
    returnByConstRef.setValue(s.returnByConstRef);
    namespaceHandling.setValue(int(s.cppFileNamespaceHandling));
    valueTypes.setValue(s.valueTypes);

    customTemplates.clear();
    for (const CppQuickFixSettings::CustomTemplate &t : s.customTemplates) {
        auto item = std::static_pointer_cast<CustomTemplateAspect>(
            customTemplates.createAndAddItem());
        item->setTemplate(t);
    }
    customTemplates.apply(); // commit the loaded list as the clean baseline
}

void CppQuickFixSettingsAspects::saveTo(CppQuickFixSettings &s) const
{
    s.getterOutsideClassFrom = getterOutsideClass.volatileValue();
    s.getterInCppFileFrom = getterInCppFile.volatileValue();
    s.setterOutsideClassFrom = setterOutsideClass.volatileValue();
    s.setterInCppFileFrom = setterInCppFile.volatileValue();
    s.getterAttributes = getterAttributes.volatileValue();
    s.getterNameTemplate = getterName.volatileValue();
    s.setterNameTemplate = setterName.volatileValue();
    s.setterParameterNameTemplate = setterParameter.volatileValue();
    s.resetNameTemplate = resetName.volatileValue();
    s.signalNameTemplate = signalName.volatileValue();
    s.memberVariableNameTemplate = memberVariableName.volatileValue();
    s.nameFromMemberVariableTemplate = nameFromMemberVariable.volatileValue();
    s.setterAsSlot = setterAsSlot.volatileValue();
    s.signalWithNewValue = signalWithNewValue.volatileValue();
    s.useAuto = useAuto.volatileValue();
    s.returnByConstRef = returnByConstRef.volatileValue();
    s.cppFileNamespaceHandling
        = CppQuickFixSettings::MissingNamespaceHandling(namespaceHandling.volatileValue());
    s.valueTypes = valueTypes.volatileValue();
    s.valueTypes.removeAll(QString());

    s.customTemplates.clear();
    for (const std::shared_ptr<BaseAspect> &item : customTemplates.volatileItems()) {
        const CppQuickFixSettings::CustomTemplate t
            = static_cast<CustomTemplateAspect *>(item.get())->toTemplate();
        // Same filter as loadSettingsFrom(), which would drop these on the next load.
        if (t.types.isEmpty()
            || (t.assignment.isEmpty() && t.returnType.isEmpty() && t.equalComparison.isEmpty())) {
            continue;
        }
        s.customTemplates.push_back(t);
    }
}

static CppQuickFixSettingsAspects &globalCppQuickFixSettingsAspects()
{
    static CppQuickFixSettingsAspects theAspects;
    static const bool initialized = [] {
        theAspects.loadFrom(*globalCppQuickFixSettings());
        theAspects.committer = [] {
            theAspects.saveTo(*globalCppQuickFixSettings());
            globalCppQuickFixSettings()->saveAsGlobalSettings();
        };
        return true;
    }();
    Q_UNUSED(initialized)
    return theAspects;
}

class CppQuickFixProjectSettingsWidget : public QWidget
{
public:
    explicit CppQuickFixProjectSettingsWidget(Project *project);

private:
    void currentItemChanged(bool useGlobal);
    void buttonCustomClicked();

    void reload();

    QWidget *m_aspectsWidget;
    CppQuickFixSettingsAspects *m_aspects;
    CppQuickFixProjectsSettings::CppQuickFixProjectsSettingsPtr m_projectSettings;
    QPushButton *m_pushButton;
    bool m_ignoreChanges = false;
};

CppQuickFixProjectSettingsWidget::CppQuickFixProjectSettingsWidget(Project *project)
{
    m_projectSettings = cppQuickFixProjectSettings(project);

    m_pushButton = new QPushButton(this);
    m_aspectsWidget = new QWidget;
    m_aspects = new CppQuickFixSettingsAspects;
    m_aspects->setParent(this);
    reload();

    using namespace Layouting;
    Column { *m_aspects, noMargin }.attachTo(m_aspectsWidget);
    Column {
        m_projectSettings->useGlobalSettings,
        Row { m_pushButton, st },
        m_aspectsWidget,
        noMargin,
    }.attachTo(this);

    currentItemChanged(m_projectSettings->useGlobalSettings());

    m_projectSettings->useGlobalSettings.addOnChanged(this, [this] {
        currentItemChanged(m_projectSettings->useGlobalSettings());
    });

    connect(m_pushButton, &QAbstractButton::clicked,
            this, &CppQuickFixProjectSettingsWidget::buttonCustomClicked);
    connect(m_aspects, &AspectContainer::volatileValueChanged, this, [this] {
        if (m_ignoreChanges || m_projectSettings->useGlobalSettings())
            return;
        m_aspects->saveTo(*m_projectSettings->getSettings());
        m_projectSettings->saveOwnSettings();
        m_ignoreChanges = true;
        m_aspects->customTemplates.apply(); // saved: make the list clean
        m_ignoreChanges = false;
    });
}

void CppQuickFixProjectSettingsWidget::reload()
{
    m_ignoreChanges = true;
    m_aspects->loadFrom(*m_projectSettings->getSettings());
    m_ignoreChanges = false;
}

void CppQuickFixProjectSettingsWidget::currentItemChanged(bool useGlobal)
{
    if (useGlobal) {
        const auto &path = m_projectSettings->filePathOfSettingsFile();
        m_pushButton->setToolTip(Tr::tr("Custom settings are saved in a file. If you use the "
                                        "global settings, you can delete that file."));
        m_pushButton->setText(Tr::tr("Delete Custom Settings File"));
        m_pushButton->setVisible(!path.isEmpty() && path.exists());
    } else /*Custom*/ {
        if (!m_projectSettings->useCustomSettings()) {
            m_projectSettings->useGlobalSettings.setValue(true, BaseAspect::BeQuiet);
            return;
        }
        m_pushButton->setToolTip(Tr::tr("Resets all settings to the global settings."));
        m_pushButton->setText(Tr::tr("Reset to Global"));
        m_pushButton->setVisible(true);
        // otherwise you change the comboBox and exit and have no custom settings:
        m_projectSettings->saveOwnSettings();
    }
    reload();
    m_aspectsWidget->setEnabled(!useGlobal);
}

void CppQuickFixProjectSettingsWidget::buttonCustomClicked()
{
    if (m_projectSettings->useGlobalSettings()) {
        // delete file
        m_projectSettings->filePathOfSettingsFile().removeFile();
        m_pushButton->setVisible(false);
    } else /*Custom*/ {
        m_projectSettings->resetOwnSettingsToGlobal();
        m_projectSettings->saveOwnSettings();
        reload();
    }
}

// Factories

class CppQuickFixProjectPanelFactory final : public ProjectPanelFactory
{
public:
    CppQuickFixProjectPanelFactory()
    {
        setPriority(100);
        setId(Constants::QUICK_FIX_PROJECT_PANEL_ID);
        setDisplayName(Tr::tr(Constants::QUICK_FIX_SETTINGS_DISPLAY_NAME));
        setCreateWidgetFunction([](Project *project) {
            return new CppQuickFixProjectSettingsWidget(project);
        });
    }
};

void setupCppQuickFixProjectPanel()
{
    static CppQuickFixProjectPanelFactory theCppQuickFixProjectPanelFactory;
}

class CppQuickFixSettingsPage : public Core::IOptionsPage
{
public:
    CppQuickFixSettingsPage()
    {
        setId(Constants::QUICK_FIX_SETTINGS_ID);
        setDisplayName(Tr::tr(Constants::QUICK_FIX_SETTINGS_DISPLAY_NAME));
        setCategory(Constants::CPP_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &globalCppQuickFixSettingsAspects(); });
    }
};

void setupCppQuickFixSettings()
{
    static CppQuickFixSettingsPage theCppQuickFixSettingsPage;
}

} // Internal
} // CppEditor

 #include "cppquickfixsettings.moc"
