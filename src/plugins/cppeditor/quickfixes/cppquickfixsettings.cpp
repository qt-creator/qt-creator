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

#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcsettings.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSpacerItem>
#include <QSpinBox>
#include <QtDebug>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

CppQuickFixSettings::CppQuickFixSettings(bool loadGlobalSettings)
{
    setDefaultSettings();
    if (loadGlobalSettings)
        this->loadGlobalSettings();
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
            = Internal::CppQuickFixProjectsSettings::getQuickFixSettings(
                ProjectExplorer::ProjectTree::currentProject());
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

auto removeAndExtractTemplate(QString type)
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

auto withoutNamespace(QString type)
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

CppQuickFixProjectsSettings::CppQuickFixProjectsSettings(ProjectExplorer::Project *project)
{
    m_project = project;
    const auto settings = storeFromVariant(m_project->namedSettings(QUICK_FIX_SETTINGS_ID));
    // if no option is saved try to load settings from a file
    m_useGlobalSettings = settings.value(USE_GLOBAL_SETTINGS, false).toBool();
    if (!m_useGlobalSettings) {
        m_settingsFile = searchForCppQuickFixSettingsFile();
        if (!m_settingsFile.isEmpty()) {
            loadOwnSettingsFromFile();
            m_useGlobalSettings = false;
        } else {
            m_useGlobalSettings = true;
        }
    }
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings, this, [this] {
        auto settings = m_project->namedSettings(QUICK_FIX_SETTINGS_ID).toMap();
        settings.insert(USE_GLOBAL_SETTINGS, m_useGlobalSettings);
        m_project->setNamedSettings(QUICK_FIX_SETTINGS_ID, settings);
    });
}

CppQuickFixSettings *CppQuickFixProjectsSettings::getSettings()
{
    if (m_useGlobalSettings)
        return CppQuickFixSettings::instance();

    return &m_ownSettings;
}

bool CppQuickFixProjectsSettings::isUsingGlobalSettings() const
{
    return m_useGlobalSettings;
}

const Utils::FilePath &CppQuickFixProjectsSettings::filePathOfSettingsFile() const
{
    return m_settingsFile;
}

CppQuickFixProjectsSettings::CppQuickFixProjectsSettingsPtr CppQuickFixProjectsSettings::getSettings(
    ProjectExplorer::Project *project)
{
    const Key key = "CppQuickFixProjectsSettings";
    QVariant v = project->extraData(key);
    if (v.isNull()) {
        v = QVariant::fromValue(
            CppQuickFixProjectsSettingsPtr{new CppQuickFixProjectsSettings(project)});
        project->setExtraData(key, v);
    }
    return v.value<QSharedPointer<CppQuickFixProjectsSettings>>();
}

CppQuickFixSettings *CppQuickFixProjectsSettings::getQuickFixSettings(ProjectExplorer::Project *project)
{
    if (project)
        return getSettings(project)->getSettings();
    return CppQuickFixSettings::instance();
}

FilePath CppQuickFixProjectsSettings::searchForCppQuickFixSettingsFile()
{
    return m_project->projectDirectory().searchHereAndInParents(SETTINGS_FILE_NAME, QDir::Files);
}

void CppQuickFixProjectsSettings::useGlobalSettings()
{
    m_useGlobalSettings = true;
}

bool CppQuickFixProjectsSettings::useCustomSettings()
{
    if (m_settingsFile.isEmpty()) {
        m_settingsFile = searchForCppQuickFixSettingsFile();
        const Utils::FilePath defaultLocation = m_project->projectDirectory() / SETTINGS_FILE_NAME;
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

    m_useGlobalSettings = false;
    return true;
}

void CppQuickFixProjectsSettings::resetOwnSettingsToGlobal()
{
    m_ownSettings = *CppQuickFixSettings::instance();
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

class CppQuickFixSettingsWidget : public Core::IOptionsPageWidget
{
    Q_OBJECT

    enum CustomDataRoles {
        Types = Qt::UserRole,
        Comparison,
        Assignment,
        ReturnExpression,
        ReturnType,
    };

public:
    CppQuickFixSettingsWidget();

    void loadSettings(CppQuickFixSettings *settings);
    void saveSettings(CppQuickFixSettings *settings);

signals:
    void settingsChanged();

private:
    void apply() final;
    void currentCustomItemChanged(QListWidgetItem *newItem, QListWidgetItem *oldItem);

    bool m_isLoadingSettings = false;
    const QRegularExpression m_typeSplitter;

    LineCountSpinBox *m_lines_getterOutsideClass;
    LineCountSpinBox *m_lines_getterInCppFile;
    LineCountSpinBox *m_lines_setterOutsideClass;
    LineCountSpinBox *m_lines_setterInCppFile;
    QLineEdit *m_lineEdit_setterParameter;
    QCheckBox *m_checkBox_setterSlots;
    QCheckBox *m_checkBox_signalWithNewValue;
    QLineEdit *m_lineEdit_getterName;
    QLineEdit *m_lineEdit_resetName;
    QLineEdit *m_lineEdit_getterAttribute;
    QLineEdit *m_lineEdit_setterName;
    QLineEdit *m_lineEdit_signalName;
    QLineEdit *m_lineEdit_memberVariableName;
    QLineEdit *m_lineEdit_nameFromMemberVariable;
    QRadioButton *m_radioButton_generateMissingNamespace;
    QRadioButton *m_radioButton_addUsingnamespace;
    QRadioButton *m_radioButton_rewriteTypes;
    QCheckBox *m_useAutoCheckBox;
    QGroupBox *m_groupBox_customTemplate;
    QLineEdit *m_lineEdit_customTemplateTypes;
    QLineEdit *m_lineEdit_customTemplateComparison;
    QLineEdit *m_lineEdit_customTemplateAssignment;
    QLineEdit *m_lineEdit_customTemplateReturnExpression;
    QLineEdit *m_lineEdit_customTemplateReturnType;
    QListWidget *m_listWidget_customTemplates;
    QPushButton *m_pushButton_removeCustomTemplate;
    QListWidget *m_valueTypes;
    QCheckBox *m_returnByConstRefCheckBox;
};

class CppQuickFixProjectSettingsWidget : public ProjectSettingsWidget
{
public:
    explicit CppQuickFixProjectSettingsWidget(Project *project);

private:
    void currentItemChanged(bool useGlobalSettings);
    void buttonCustomClicked();

    CppQuickFixSettingsWidget *m_settingsWidget;
    CppQuickFixProjectsSettings::CppQuickFixProjectsSettingsPtr m_projectSettings;

    QPushButton *m_pushButton;
};

CppQuickFixProjectSettingsWidget::CppQuickFixProjectSettingsWidget(Project *project)
{
    setGlobalSettingsId(CppEditor::Constants::QUICK_FIX_SETTINGS_ID);
    m_projectSettings = CppQuickFixProjectsSettings::getSettings(project);

    m_pushButton = new QPushButton(this);

    auto gridLayout = new QGridLayout(this);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->addWidget(m_pushButton, 1, 0, 1, 1);
    auto layout = new QVBoxLayout();
    gridLayout->addLayout(layout, 2, 0, 1, 2);

    m_settingsWidget = new CppQuickFixSettingsWidget;
    m_settingsWidget->loadSettings(m_projectSettings->getSettings());

    if (QLayout *layout = m_settingsWidget->layout())
        layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_settingsWidget);

    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
            this, &CppQuickFixProjectSettingsWidget::currentItemChanged);
    setUseGlobalSettings(m_projectSettings->isUsingGlobalSettings());
    currentItemChanged(m_projectSettings->isUsingGlobalSettings());

    connect(m_pushButton, &QAbstractButton::clicked,
            this, &CppQuickFixProjectSettingsWidget::buttonCustomClicked);
    connect(m_settingsWidget, &CppQuickFixSettingsWidget::settingsChanged, this,
            [this] {
                m_settingsWidget->saveSettings(m_projectSettings->getSettings());
                if (!useGlobalSettings())
                    m_projectSettings->saveOwnSettings();
            });
}

void CppQuickFixProjectSettingsWidget::currentItemChanged(bool useGlobalSettings)
{
    if (useGlobalSettings) {
        const auto &path = m_projectSettings->filePathOfSettingsFile();
        m_pushButton->setToolTip(Tr::tr("Custom settings are saved in a file. If you use the "
                                        "global settings, you can delete that file."));
        m_pushButton->setText(Tr::tr("Delete Custom Settings File"));
        m_pushButton->setVisible(!path.isEmpty() && path.exists());
        m_projectSettings->useGlobalSettings();
    } else /*Custom*/ {
        if (!m_projectSettings->useCustomSettings()) {
            setUseGlobalSettings(!m_projectSettings->useCustomSettings());
            return;
        }
        m_pushButton->setToolTip(Tr::tr("Resets all settings to the global settings."));
        m_pushButton->setText(Tr::tr("Reset to Global"));
        m_pushButton->setVisible(true);
        // otherwise you change the comboBox and exit and have no custom settings:
        m_projectSettings->saveOwnSettings();
    }
    m_settingsWidget->loadSettings(m_projectSettings->getSettings());
    m_settingsWidget->setEnabled(!useGlobalSettings);
}

void CppQuickFixProjectSettingsWidget::buttonCustomClicked()
{
    if (useGlobalSettings()) {
        // delete file
        m_projectSettings->filePathOfSettingsFile().removeFile();
        m_pushButton->setVisible(false);
    } else /*Custom*/ {
        m_projectSettings->resetOwnSettingsToGlobal();
        m_projectSettings->saveOwnSettings();
        m_settingsWidget->loadSettings(m_projectSettings->getSettings());
    }
}

CppQuickFixSettingsWidget::CppQuickFixSettingsWidget()
    : m_typeSplitter("\\s*,\\s*")
{
    m_lines_getterOutsideClass = new LineCountSpinBox;
    m_lines_getterInCppFile = new LineCountSpinBox;
    m_lines_setterOutsideClass = new LineCountSpinBox;
    m_lines_setterInCppFile = new LineCountSpinBox;
    auto functionLocationsGrid = new QWidget;
    auto ulLabel = [] (const QString &text) {
        QLabel *label = new QLabel(text);
        QFont font = label->font();
        font.setUnderline(true);
        label->setFont(font);
        return label;
    };

    const QString description1 = Tr::tr(
        "A JavaScript expression acting as the return value of a function with two parameters "
        "<b>name</b> and <b>memberName</b>, where"
        "<ul><li><b>name</b> is the \"semantic name\" as it would be used for a Qt property</li>"
        "<li><b>memberName</b> is the name of the member variable.</li></ul>");
    const QString toolTip1 = QString("<html><body>%1</body></html>").arg(description1);
    const QString description2 = Tr::tr(
        "A JavaScript expression acting as the return value of a function with a parameter "
        "<b>name</b>, which is the \"semantic name\" as it would be used for a Qt property.");
    const QString toolTip2 = QString("<html><body>%1</body></html>").arg(description2);
    CppQuickFixSettings defaultSettings;

    const auto makeJsField = [] {
        const auto field = new QLineEdit;
        QSizePolicy sp = field->sizePolicy();
        sp.setHorizontalStretch(1);
        field->setSizePolicy(sp);
        return field;
    };
    m_lineEdit_getterAttribute = new QLineEdit;
    m_lineEdit_getterAttribute->setPlaceholderText(Tr::tr("For example, [[nodiscard]]"));
    m_lineEdit_getterName = makeJsField();
    m_lineEdit_getterName->setPlaceholderText(defaultSettings.getterNameTemplate);
    m_lineEdit_getterName->setToolTip(toolTip1);
    m_lineEdit_setterName = makeJsField();
    m_lineEdit_setterName->setPlaceholderText(defaultSettings.setterNameTemplate);
    m_lineEdit_setterName->setToolTip(toolTip1);
    m_lineEdit_setterParameter = makeJsField();
    m_lineEdit_setterParameter->setPlaceholderText(defaultSettings.setterParameterNameTemplate);
    m_lineEdit_setterParameter->setToolTip(toolTip1);
    m_checkBox_setterSlots = new QCheckBox(Tr::tr("Setters should be slots"));
    m_lineEdit_resetName = makeJsField();
    m_lineEdit_resetName->setPlaceholderText(defaultSettings.resetNameTemplate);
    m_lineEdit_resetName->setToolTip(toolTip1);
    m_lineEdit_signalName = makeJsField();
    m_lineEdit_signalName->setPlaceholderText(defaultSettings.signalNameTemplate);
    m_lineEdit_signalName->setToolTip(toolTip1);
    m_checkBox_signalWithNewValue = new QCheckBox(
                Tr::tr("Generate signals with the new value as parameter"));
    m_lineEdit_memberVariableName = makeJsField();
    m_lineEdit_memberVariableName->setPlaceholderText(defaultSettings.memberVariableNameTemplate);
    m_lineEdit_memberVariableName->setToolTip(toolTip2);
    m_lineEdit_nameFromMemberVariable = makeJsField();
    m_lineEdit_nameFromMemberVariable->setToolTip(
        Tr::tr(
            "How to get from the member variable to the semantic name.\n"
            "This is the reverse of the operation above.\n"
            "Leave empty to apply heuristics."));

    const auto jsTestButton = new QPushButton(Tr::tr("Test"));
    const auto hideJsTestResultsButton = new QPushButton(Tr::tr("Hide Test Results"));
    const auto jsTestInputField = new QLineEdit;
    jsTestInputField->setToolTip(
        Tr::tr(
            "The content of the <b>name</b> parameter in the fields above, that is, the "
            "\"semantic name\" without any prefix or suffix."));
    jsTestInputField->setText("myValue");
    const auto makeResultField = [] {
        const auto resultField = new QLineEdit;
        resultField->setReadOnly(true);
        return resultField;
    };
    QLineEdit * const getterTestResultField = makeResultField();
    QLineEdit * const setterTestResultField = makeResultField();
    QLineEdit * const setterParameterTestResultField = makeResultField();
    QLineEdit * const resetterTestResultField = makeResultField();
    QLineEdit * const signalTestResultField = makeResultField();
    QLineEdit * const memberTestResultField = makeResultField();
    QLineEdit * const nameFromMemberTestResultField = makeResultField();
    const auto runTests = [=, this] {
        const QString memberName = CppQuickFixSettings::replaceNamePlaceholders(
            m_lineEdit_memberVariableName->text(), jsTestInputField->text(), {});
        memberTestResultField->show();
        memberTestResultField->setText(memberName);
        for (const auto &[codeField, resultField] :
             {std::make_pair(m_lineEdit_getterName, getterTestResultField),
              std::make_pair(m_lineEdit_setterName, setterTestResultField),
              std::make_pair(m_lineEdit_setterParameter, setterParameterTestResultField),
              std::make_pair(m_lineEdit_resetName, resetterTestResultField),
              std::make_pair(m_lineEdit_signalName, signalTestResultField),}) {
            resultField->show();
            resultField->setText(
                CppQuickFixSettings::replaceNamePlaceholders(
                    codeField->text(), jsTestInputField->text(), memberName));
        }
        nameFromMemberTestResultField->show();
        nameFromMemberTestResultField->setText(
            CppQuickFixSettings::memberBaseName(
                memberTestResultField->text(), m_lineEdit_nameFromMemberVariable->text()));
    };
    const auto hideResultFields = [=] {
        getterTestResultField->hide();
        setterTestResultField->hide();
        setterParameterTestResultField->hide();
        resetterTestResultField->hide();
        signalTestResultField->hide();
        memberTestResultField->hide();
        nameFromMemberTestResultField->hide();
    };
    connect(jsTestButton, &QPushButton::clicked, runTests);
    connect(hideJsTestResultsButton, &QPushButton::clicked, hideResultFields);
    hideResultFields();

    m_radioButton_generateMissingNamespace = new QRadioButton(Tr::tr("Generate missing namespaces"));
    m_radioButton_addUsingnamespace = new QRadioButton(Tr::tr("Add \"using namespace ...\""));
    m_radioButton_rewriteTypes = new QRadioButton(
                Tr::tr("Rewrite types to match the existing namespaces"));

    m_useAutoCheckBox = new QCheckBox(this);
    m_useAutoCheckBox->setToolTip(Tr::tr("<html><head/><body><p>Uncheck this to make Qt Creator try to "
                                         "derive the type of expression in the &quot;Assign to Local "
                                         "Variable&quot; quickfix.</p><p>Note that this might fail for "
                                         "more complex types.</p></body></html>"));
    m_useAutoCheckBox->setText(Tr::tr("Use type \"auto\" when creating new variables"));

    m_groupBox_customTemplate = new QGroupBox(Tr::tr("Template"));
    m_groupBox_customTemplate->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    m_groupBox_customTemplate->setEnabled(false);
    m_listWidget_customTemplates = new QListWidget;
    m_listWidget_customTemplates->setMaximumWidth(200);
    m_listWidget_customTemplates->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    m_lineEdit_customTemplateTypes = new QLineEdit;
    m_lineEdit_customTemplateTypes->setToolTip(Tr::tr("Separate the types by comma."));
    m_lineEdit_customTemplateComparison = new QLineEdit;
    m_lineEdit_customTemplateAssignment = new QLineEdit;
    m_lineEdit_customTemplateReturnExpression = new QLineEdit;
    m_lineEdit_customTemplateReturnType = new QLineEdit;
    auto customTemplateLabel = new QLabel(Tr::tr("Use <new> and <cur> to access the parameter and "
                                                 "current value. Use <type> to access the type and <T> "
                                                 "for the template parameter."));
    customTemplateLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    customTemplateLabel->setWordWrap(true);

    auto pushButton_addCustomTemplate = new QPushButton;
    pushButton_addCustomTemplate->setText(Tr::tr("Add"));
    m_pushButton_removeCustomTemplate = new QPushButton(Tr::tr("Remove"));
    m_pushButton_removeCustomTemplate->setEnabled(false);

    m_valueTypes = new QListWidget(this);
    m_valueTypes->setToolTip(Tr::tr("Normally arguments get passed by const reference. If the Type is "
                                    "one of the following ones, the argument gets passed by value. "
                                    "Namespaces and template arguments are removed. The real Type must "
                                    "contain the given Type. For example, \"int\" matches \"int32_t\" "
                                    "but not \"vector<int>\". \"vector\" matches "
                                    "\"std::pmr::vector<int>\" but not "
                                    "\"std::optional<vector<int>>\""));
    auto pushButton_addValueType = new QPushButton(Tr::tr("Add"));
    auto pushButton_removeValueType = new QPushButton(Tr::tr("Remove"));

    m_returnByConstRefCheckBox = new QCheckBox(Tr::tr("Return non-value types by const reference"));
    m_returnByConstRefCheckBox->setChecked(false);

    connect(m_listWidget_customTemplates, &QListWidget::currentItemChanged,
            this, &CppQuickFixSettingsWidget::currentCustomItemChanged);

    connect(pushButton_addValueType, &QPushButton::clicked, this, [this] {
        auto item = new QListWidgetItem("<type>", m_valueTypes);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled
                       | Qt::ItemNeverHasChildren);
        m_valueTypes->scrollToItem(item);
        item->setSelected(true);
    });
    connect(pushButton_addCustomTemplate, &QPushButton::clicked, this, [this] {
        auto item = new QListWidgetItem("<type>", m_listWidget_customTemplates);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
        m_listWidget_customTemplates->scrollToItem(item);
        m_listWidget_customTemplates->setCurrentItem(item);
        m_lineEdit_customTemplateTypes->setText("<type>");
    });
    connect(m_pushButton_removeCustomTemplate, &QPushButton::clicked, this, [this] {
        delete m_listWidget_customTemplates->currentItem();
        Utils::markSettingsDirty();
    });
    connect(pushButton_removeValueType, &QPushButton::clicked, this, [this] {
        delete m_valueTypes->currentItem();
        Utils::markSettingsDirty();
    });

    setEnabled(false);

    using namespace Layouting;

    // clang-format off
    Grid {
        empty, ulLabel(Tr::tr("Generate Setters")), ulLabel(Tr::tr("Generate Getters")), br,
        Tr::tr("Inside class:"), Tr::tr("Default"), Tr::tr("Default"), br,
        Tr::tr("Outside class:"), m_lines_setterOutsideClass, m_lines_getterOutsideClass, br,
        Tr::tr("In .cpp file:"), m_lines_setterInCppFile, m_lines_getterInCppFile, br,
        noMargin,
    }.attachTo(functionLocationsGrid);

    if (QGridLayout *gl = qobject_cast<QGridLayout*>(functionLocationsGrid->layout()))
        gl->setHorizontalSpacing(48);

    Form {
        Tr::tr("Types:"), m_lineEdit_customTemplateTypes, br,
        Tr::tr("Comparison:"), m_lineEdit_customTemplateComparison, br,
        Tr::tr("Assignment:"), m_lineEdit_customTemplateAssignment, br,
        Tr::tr("Return expression:"), m_lineEdit_customTemplateReturnExpression, br,
        Tr::tr("Return type:"), m_lineEdit_customTemplateReturnType, br,
        customTemplateLabel, br,
    }.attachTo(m_groupBox_customTemplate);

    Column {
        Group {
            title(Tr::tr("Generated Function Locations")),
            Row { functionLocationsGrid, st, },
        },
        Group {
            title(Tr::tr("Getter Setter Generation Properties")),
            Form {
                Tr::tr("Getter attributes:"), m_lineEdit_getterAttribute, br,
                Tr::tr("Getter name:"), m_lineEdit_getterName, getterTestResultField, br,
                Tr::tr("Setter name:"), m_lineEdit_setterName, setterTestResultField, br,
                Tr::tr("Setter parameter name:"), m_lineEdit_setterParameter, setterParameterTestResultField, br,
                m_checkBox_setterSlots, br,
                Tr::tr("Reset name:"), m_lineEdit_resetName, resetterTestResultField, br,
                Tr::tr("Signal name:"), m_lineEdit_signalName, signalTestResultField, br,
                m_checkBox_signalWithNewValue, br,
                Tr::tr("Member variable name:"), m_lineEdit_memberVariableName, memberTestResultField, br,
                Tr::tr("Name from member variable:"), m_lineEdit_nameFromMemberVariable, nameFromMemberTestResultField, br,
                Tr::tr("Test with example name:"), jsTestInputField, jsTestButton, hideJsTestResultsButton, st, br,
            },
        },
        Group {
            title(Tr::tr("Missing Namespace Handling")),
            Form {
                m_radioButton_generateMissingNamespace, br,
                m_radioButton_addUsingnamespace, br,
                m_radioButton_rewriteTypes, br,
            },
        },
        m_useAutoCheckBox,
        Group {
            title(Tr::tr("Custom Getter Setter Templates")),
            Row {
                Column {
                    m_listWidget_customTemplates,
                    Row { pushButton_addCustomTemplate, m_pushButton_removeCustomTemplate, },
                },
                m_groupBox_customTemplate,
            },
        },
        Group {
            title(Tr::tr("Value Types")),
            Row {
                m_valueTypes,
                Column { pushButton_addValueType, pushButton_removeValueType, st, },
            },
        },
        m_returnByConstRefCheckBox,
    }.attachTo(this);
    // clang-format on

    // connect controls to settingsChanged signal
    auto then = [this] {
        if (!m_isLoadingSettings)
            emit settingsChanged();
    };

    connect(m_lines_setterOutsideClass, &LineCountSpinBox::changed, then);
    connect(m_lines_setterInCppFile, &LineCountSpinBox::changed, then);
    connect(m_lines_getterOutsideClass, &LineCountSpinBox::changed, then);
    connect(m_lines_getterInCppFile, &LineCountSpinBox::changed, then);
    connect(m_checkBox_setterSlots, &QCheckBox::clicked, then);
    connect(m_checkBox_signalWithNewValue, &QCheckBox::clicked, then);
    connect(pushButton_addCustomTemplate, &QPushButton::clicked, then);
    connect(m_pushButton_removeCustomTemplate, &QPushButton::clicked, then);
    connect(pushButton_addValueType, &QPushButton::clicked, then);
    connect(pushButton_removeValueType, &QPushButton::clicked, then);
    connect(m_useAutoCheckBox, &QCheckBox::clicked, then);
    connect(m_valueTypes, &QListWidget::itemChanged, then);
    connect(m_returnByConstRefCheckBox, &QCheckBox::clicked, then);
    connect(m_lineEdit_customTemplateAssignment, &QLineEdit::textEdited, then);
    connect(m_lineEdit_customTemplateComparison, &QLineEdit::textEdited, then);
    connect(m_lineEdit_customTemplateReturnExpression, &QLineEdit::textEdited, then);
    connect(m_lineEdit_customTemplateReturnType, &QLineEdit::textEdited, then);
    connect(m_lineEdit_customTemplateTypes, &QLineEdit::textEdited, then);
    connect(m_lineEdit_getterAttribute, &QLineEdit::textEdited, then);
    connect(m_lineEdit_getterName, &QLineEdit::textEdited, then);
    connect(m_lineEdit_memberVariableName, &QLineEdit::textEdited, then);
    connect(m_lineEdit_nameFromMemberVariable, &QLineEdit::textEdited, then);
    connect(m_lineEdit_resetName, &QLineEdit::textEdited, then);
    connect(m_lineEdit_setterName, &QLineEdit::textEdited, then);
    connect(m_lineEdit_setterParameter, &QLineEdit::textEdited, then);
    connect(m_lineEdit_signalName, &QLineEdit::textEdited, then);
    connect(m_radioButton_addUsingnamespace, &QRadioButton::clicked, then);
    connect(m_radioButton_generateMissingNamespace, &QRadioButton::clicked, then);
    connect(m_radioButton_rewriteTypes, &QRadioButton::clicked, then);

    loadSettings(CppQuickFixSettings::instance());

    Utils::installMarkSettingsDirtyTriggerRecursively(this);
}

void CppQuickFixSettingsWidget::loadSettings(CppQuickFixSettings *settings)
{
    m_isLoadingSettings = true;
    m_lines_getterOutsideClass->setCount(settings->getterOutsideClassFrom);
    m_lines_getterInCppFile->setCount(settings->getterInCppFileFrom);
    m_lines_setterOutsideClass->setCount(settings->setterOutsideClassFrom);
    m_lines_setterInCppFile->setCount(settings->setterInCppFileFrom);
    m_lineEdit_getterAttribute->setText(settings->getterAttributes);
    m_lineEdit_getterName->setText(settings->getterNameTemplate);
    m_lineEdit_setterName->setText(settings->setterNameTemplate);
    m_lineEdit_setterParameter->setText(settings->setterParameterNameTemplate);
    switch (settings->cppFileNamespaceHandling) {
    case CppQuickFixSettings::MissingNamespaceHandling::RewriteType:
        m_radioButton_rewriteTypes->setChecked(true);
        break;
    case CppQuickFixSettings::MissingNamespaceHandling::CreateMissing:
        m_radioButton_generateMissingNamespace->setChecked(true);
        break;
    case CppQuickFixSettings::MissingNamespaceHandling::AddUsingDirective:
        m_radioButton_addUsingnamespace->setChecked(true);
        break;
    }
    m_lineEdit_resetName->setText(settings->resetNameTemplate);
    m_lineEdit_signalName->setText(settings->signalNameTemplate);
    m_lineEdit_memberVariableName->setText(settings->memberVariableNameTemplate);
    m_lineEdit_nameFromMemberVariable->setText(settings->nameFromMemberVariableTemplate);
    m_checkBox_setterSlots->setChecked(settings->setterAsSlot);
    m_checkBox_signalWithNewValue->setChecked(settings->signalWithNewValue);
    m_useAutoCheckBox->setChecked(settings->useAuto);
    m_valueTypes->clear();
    for (const auto &valueType : std::as_const(settings->valueTypes)) {
        auto item = new QListWidgetItem(valueType, m_valueTypes);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled
                       | Qt::ItemNeverHasChildren);
    }
    connect(m_valueTypes, &QListWidget::itemChanged, this, Utils::markSettingsDirty);
    m_returnByConstRefCheckBox->setChecked(settings->returnByConstRef);
    m_listWidget_customTemplates->clear();
    for (const auto &customTemplate : settings->customTemplates) {
        auto item = new QListWidgetItem(customTemplate.types.join(", "),
                                        m_listWidget_customTemplates);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
        item->setData(CustomDataRoles::Types, customTemplate.types.join(", "));
        item->setData(CustomDataRoles::Comparison, customTemplate.equalComparison);
        item->setData(CustomDataRoles::Assignment, customTemplate.assignment);
        item->setData(CustomDataRoles::ReturnType, customTemplate.returnType);
        item->setData(CustomDataRoles::ReturnExpression, customTemplate.returnExpression);
    }
    if (m_listWidget_customTemplates->count() > 0) {
        m_listWidget_customTemplates->setCurrentItem(m_listWidget_customTemplates->item(0));
    }
    this->setEnabled(true);
    m_isLoadingSettings = false;
}

void CppQuickFixSettingsWidget::saveSettings(CppQuickFixSettings *settings)
{
    // first write the current selected custom template back to the model
    if (m_listWidget_customTemplates->currentItem() != nullptr) {
        auto item = m_listWidget_customTemplates->currentItem();
        auto list = m_lineEdit_customTemplateTypes->text().split(m_typeSplitter, Qt::SkipEmptyParts);
        item->setData(CustomDataRoles::Types, list);
        item->setData(CustomDataRoles::Comparison, m_lineEdit_customTemplateComparison->text());
        item->setData(CustomDataRoles::Assignment, m_lineEdit_customTemplateAssignment->text());
        item->setData(CustomDataRoles::ReturnType, m_lineEdit_customTemplateReturnType->text());
        item->setData(CustomDataRoles::ReturnExpression,
                      m_lineEdit_customTemplateReturnExpression->text());
    }
    settings->getterOutsideClassFrom = m_lines_getterOutsideClass->count();
    settings->getterInCppFileFrom = m_lines_getterInCppFile->count();
    settings->setterOutsideClassFrom = m_lines_setterOutsideClass->count();
    settings->setterInCppFileFrom = m_lines_setterInCppFile->count();
    settings->setterParameterNameTemplate = m_lineEdit_setterParameter->text();
    settings->setterAsSlot = m_checkBox_setterSlots->isChecked();
    settings->signalWithNewValue = m_checkBox_signalWithNewValue->isChecked();
    settings->getterAttributes = m_lineEdit_getterAttribute->text();
    settings->getterNameTemplate = m_lineEdit_getterName->text();
    settings->setterNameTemplate = m_lineEdit_setterName->text();
    settings->resetNameTemplate = m_lineEdit_resetName->text();
    settings->signalNameTemplate = m_lineEdit_signalName->text();
    settings->memberVariableNameTemplate = m_lineEdit_memberVariableName->text();
    settings->nameFromMemberVariableTemplate = m_lineEdit_nameFromMemberVariable->text();
    if (m_radioButton_rewriteTypes->isChecked()) {
        settings->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::RewriteType;
    } else if (m_radioButton_addUsingnamespace->isChecked()) {
        settings->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::AddUsingDirective;
    } else if (m_radioButton_generateMissingNamespace->isChecked()) {
        settings->cppFileNamespaceHandling = CppQuickFixSettings::MissingNamespaceHandling::CreateMissing;
    }
    settings->useAuto = m_useAutoCheckBox->isChecked();
    settings->valueTypes.clear();
    for (int i = 0; i < m_valueTypes->count(); ++i) {
        settings->valueTypes << m_valueTypes->item(i)->text();
    }
    settings->returnByConstRef = m_returnByConstRefCheckBox->isChecked();
    settings->customTemplates.clear();
    for (int i = 0; i < m_listWidget_customTemplates->count(); ++i) {
        auto item = m_listWidget_customTemplates->item(i);
        CppQuickFixSettings::CustomTemplate t;
        t.types = item->data(CustomDataRoles::Types).toStringList();
        t.equalComparison = item->data(CustomDataRoles::Comparison).toString();
        t.assignment = item->data(CustomDataRoles::Assignment).toString();
        t.returnExpression = item->data(CustomDataRoles::ReturnExpression).toString();
        t.returnType = item->data(CustomDataRoles::ReturnType).toString();
        settings->customTemplates.push_back(t);
    }
}

void CppQuickFixSettingsWidget::apply()
{
    const auto s = CppQuickFixSettings::instance();
    saveSettings(s);
    s->saveAsGlobalSettings();
}

void CppQuickFixSettingsWidget::currentCustomItemChanged(QListWidgetItem *newItem,
                                                         QListWidgetItem *oldItem)
{
    if (oldItem) {
        auto list = m_lineEdit_customTemplateTypes->text().split(m_typeSplitter, Qt::SkipEmptyParts);
        oldItem->setData(CustomDataRoles::Types, list);
        oldItem->setData(Qt::DisplayRole, list.join(", "));
        oldItem->setData(CustomDataRoles::Comparison, m_lineEdit_customTemplateComparison->text());
        oldItem->setData(CustomDataRoles::Assignment, m_lineEdit_customTemplateAssignment->text());
        oldItem->setData(CustomDataRoles::ReturnType, m_lineEdit_customTemplateReturnType->text());
        oldItem->setData(CustomDataRoles::ReturnExpression,
                         m_lineEdit_customTemplateReturnExpression->text());
    }
    m_pushButton_removeCustomTemplate->setEnabled(newItem != nullptr);
    m_groupBox_customTemplate->setEnabled(newItem != nullptr);
    if (newItem) {
        m_lineEdit_customTemplateTypes->setText(
            newItem->data(CustomDataRoles::Types).toStringList().join(", "));
        m_lineEdit_customTemplateComparison->setText(
            newItem->data(CustomDataRoles::Comparison).toString());
        m_lineEdit_customTemplateAssignment->setText(
            newItem->data(CustomDataRoles::Assignment).toString());
        m_lineEdit_customTemplateReturnType->setText(
            newItem->data(CustomDataRoles::ReturnType).toString());
        m_lineEdit_customTemplateReturnExpression->setText(
            newItem->data(CustomDataRoles::ReturnExpression).toString());
    } else {
        m_lineEdit_customTemplateTypes->setText("");
        m_lineEdit_customTemplateComparison->setText("");
        m_lineEdit_customTemplateAssignment->setText("");
        m_lineEdit_customTemplateReturnType->setText("");
        m_lineEdit_customTemplateReturnExpression->setText("");
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
        setWidgetCreator([] { return new CppQuickFixSettingsWidget; });
    }
};

void setupCppQuickFixSettings()
{
    static CppQuickFixSettingsPage theCppQuickFixSettingsPage;
}

} // Internal
} // CppEditor

 #include "cppquickfixsettings.moc"
