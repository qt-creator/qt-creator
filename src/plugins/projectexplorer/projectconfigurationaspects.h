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
#include <utils/pathchooser.h>

#include <memory>

namespace ProjectExplorer {

namespace Internal {
class BaseBoolAspectPrivate;
class BaseStringAspectPrivate;
class BaseIntegerAspectPrivate;
} // Internal

class PROJECTEXPLORER_EXPORT BaseBoolAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    explicit BaseBoolAspect(const QString &settingsKey = QString());
    ~BaseBoolAspect() override;

    void addToConfigurationLayout(QFormLayout *layout) override;

    bool value() const;
    void setValue(bool val);

    bool defaultValue() const;
    void setDefaultValue(bool defaultValue);

    void setLabel(const QString &label);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    std::unique_ptr<Internal::BaseBoolAspectPrivate> d;
};

class PROJECTEXPLORER_EXPORT BaseStringAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    BaseStringAspect();
    ~BaseStringAspect() override;

    void addToConfigurationLayout(QFormLayout *layout) override;

    QString value() const;
    void setValue(const QString &val);

    QString labelText() const;
    void setLabelText(const QString &labelText);
    void setLabelPixmap(const QPixmap &labelPixmap);

    void setDisplayFilter(const std::function<QString (const QString &)> &displayFilter);
    void setPlaceHolderText(const QString &placeHolderText);
    void setHistoryCompleter(const QString &historyCompleterKey);
    void setExpectedKind(const Utils::PathChooser::Kind expectedKind);
    void setEnvironment(const Utils::Environment &env);

    bool isChecked() const;
    void makeCheckable(const QString &optionalLabel, const QString &optionalBaseKey);

    enum DisplayStyle { LabelDisplay, LineEditDisplay, PathChooserDisplay };
    void setDisplayStyle(DisplayStyle style);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    Utils::FileName fileName() const;
    void setFileName(const Utils::FileName &val);

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

    void addToConfigurationLayout(QFormLayout *layout) override;

    int value() const;
    void setValue(int val);

    void setRange(int min, int max);
    void setLabel(const QString &label);
    void setPrefix(const QString &prefix);
    void setSuffix(const QString &suffix);
    void setDisplayIntegerBase(int base);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    std::unique_ptr<Internal::BaseIntegerAspectPrivate> d;
};

} // namespace ProjectExplorer
