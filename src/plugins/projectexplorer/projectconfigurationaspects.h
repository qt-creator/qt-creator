/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "projectconfiguration.h"
#include "environmentaspect.h"

#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>

#include <memory>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace ProjectExplorer {

namespace Internal {
class BaseBoolAspectPrivate;
class BaseIntegerAspectPrivate;
class BaseSelectionAspectPrivate;
class BaseStringAspectPrivate;
class BaseStringListAspectPrivate;
} // Internal

class PROJECTEXPLORER_EXPORT BaseBoolAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    explicit BaseBoolAspect(const QString &settingsKey = QString());
    ~BaseBoolAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    bool value() const;
    void setValue(bool val);

    bool defaultValue() const;
    void setDefaultValue(bool defaultValue);

    enum class LabelPlacement { AtCheckBox, InExtraLabel };
    void setLabel(const QString &label, LabelPlacement labelPlacement);
    void setToolTip(const QString &tooltip);
    void setEnabled(bool enabled);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    std::unique_ptr<Internal::BaseBoolAspectPrivate> d;
};

class PROJECTEXPLORER_EXPORT BaseSelectionAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    BaseSelectionAspect();
    ~BaseSelectionAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    int value() const;
    void setValue(int val);

    int defaultValue() const;
    void setDefaultValue(int defaultValue);

    enum class DisplayStyle { RadioButtons, ComboBox };
    void setDisplayStyle(DisplayStyle style);

    void addOption(const QString &displayName, const QString &toolTip = {});

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

protected:
    void setVisibleDynamic(bool visible) override;

private:
    std::unique_ptr<Internal::BaseSelectionAspectPrivate> d;
};

class PROJECTEXPLORER_EXPORT BaseStringAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    BaseStringAspect();
    ~BaseStringAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    // Hook between UI and BaseStringAspect:
    using ValueAcceptor = std::function<Utils::optional<QString>(const QString &, const QString &)>;
    void setValueAcceptor(ValueAcceptor &&acceptor);
    QString value() const;
    void setValue(const QString &val);

    QString labelText() const;
    void setLabelText(const QString &labelText);
    void setLabelPixmap(const QPixmap &labelPixmap);
    void setShowToolTipOnLabel(bool show);

    void setDisplayFilter(const std::function<QString (const QString &)> &displayFilter);
    void setPlaceHolderText(const QString &placeHolderText);
    void setHistoryCompleter(const QString &historyCompleterKey);
    void setExpectedKind(const Utils::PathChooser::Kind expectedKind);
    void setFileDialogOnly(bool requireFileDialog);
    void setEnvironment(const Utils::Environment &env);
    void setBaseFileName(const Utils::FilePath &baseFileName);
    void setReadOnly(bool readOnly);
    void setMacroExpanderProvider(const Utils::MacroExpanderProvider &expanderProvider);

    void validateInput();

    enum class UncheckedSemantics { Disabled, ReadOnly };
    enum class CheckBoxPlacement { Top, Right };
    void setUncheckedSemantics(UncheckedSemantics semantics);
    bool isChecked() const;
    void setChecked(bool checked);
    void makeCheckable(CheckBoxPlacement checkBoxPlacement, const QString &optionalLabel,
                       const QString &optionalBaseKey);

    enum DisplayStyle {
        LabelDisplay,
        LineEditDisplay,
        TextEditDisplay,
        PathChooserDisplay
    };
    void setDisplayStyle(DisplayStyle style);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    Utils::FilePath filePath() const;
    void setFilePath(const Utils::FilePath &val);

signals:
    void checkedChanged();

private:
    void update();

    std::unique_ptr<Internal::BaseStringAspectPrivate> d;
};

class PROJECTEXPLORER_EXPORT BaseIntegerAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    BaseIntegerAspect();
    ~BaseIntegerAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    qint64 value() const;
    void setValue(qint64 val);

    void setRange(qint64 min, qint64 max);
    void setLabel(const QString &label);
    void setPrefix(const QString &prefix);
    void setSuffix(const QString &suffix);
    void setDisplayIntegerBase(int base);
    void setDisplayScaleFactor(qint64 factor);
    void setEnabled(bool enabled);
    void setDefaultValue(qint64 defaultValue);
    void setToolTip(const QString &tooltip);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    std::unique_ptr<Internal::BaseIntegerAspectPrivate> d;
};

class PROJECTEXPLORER_EXPORT TriState
{
    enum Value { EnabledValue, DisabledValue, DefaultValue };
    explicit TriState(Value v) : m_value(v) {}

public:
    TriState() = default;

    QVariant toVariant() const { return int(m_value); }
    static TriState fromVariant(const QVariant &variant);

    static const TriState Enabled;
    static const TriState Disabled;
    static const TriState Default;

    friend bool operator==(TriState a, TriState b) { return a.m_value == b.m_value; }
    friend bool operator!=(TriState a, TriState b) { return a.m_value != b.m_value; }

private:
    Value m_value = DefaultValue;
};

class PROJECTEXPLORER_EXPORT BaseTriStateAspect : public BaseSelectionAspect
{
    Q_OBJECT
public:
    BaseTriStateAspect();

    TriState setting() const;
    void setSetting(TriState setting);
};

class PROJECTEXPLORER_EXPORT BaseStringListAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    BaseStringListAspect();
    ~BaseStringListAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    QStringList value() const;
    void setValue(const QStringList &val);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    std::unique_ptr<Internal::BaseStringListAspectPrivate> d;
};

} // namespace ProjectExplorer
