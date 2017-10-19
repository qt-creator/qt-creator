/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "jsonfieldpage.h"

#include <utils/pathchooser.h>

#include <QWidget>
#include <QString>
#include <QVariant>

namespace ProjectExplorer {

// --------------------------------------------------------------------
// JsonFieldPage::Field::FieldPrivate:
// --------------------------------------------------------------------

class JsonFieldPage::Field::FieldPrivate
{
public:
    QString m_name;
    QString m_displayName;
    QString m_toolTip;
    bool m_isMandatory = false;
    bool m_hasSpan = false;

    QVariant m_visibleExpression;
    QVariant m_enabledExpression;
    QVariant m_isCompleteExpando;
    QString m_isCompleteExpandoMessage;

    QLabel *m_label = nullptr;
    QWidget *m_widget = nullptr;

    QString m_type;
};

// --------------------------------------------------------------------
// Field Implementations:
// --------------------------------------------------------------------

class LabelField : public JsonFieldPage::Field
{
private:
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;
    bool parseData(const QVariant &data, QString *errorMessage) override;

    bool m_wordWrap = false;
    QString m_text;
};

class SpacerField : public JsonFieldPage::Field
{
public:
    bool suppressName() const override { return true; }

private:
    bool parseData(const QVariant &data, QString *errorMessage) override;
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;

    int m_factor = 1;
};

class LineEditField : public JsonFieldPage::Field
{
private:
    bool parseData(const QVariant &data, QString *errorMessage) override;
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;

    void setup(JsonFieldPage *page, const QString &name) override;

    bool validate(Utils::MacroExpander *expander, QString *message) override;
    void initializeData(Utils::MacroExpander *expander) override;

    bool m_isModified = false;
    bool m_isValidating = false;
    bool m_restoreLastHistoryItem = false;
    bool m_isPassword = false;
    QString m_placeholderText;
    QString m_defaultText;
    QString m_disabledText;
    QString m_historyId;
    QRegularExpression m_validatorRegExp;
    QString m_fixupExpando;
    mutable QString m_currentText;
};

class TextEditField : public JsonFieldPage::Field
{
private:
    bool parseData(const QVariant &data, QString *errorMessage) override;
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;

    void setup(JsonFieldPage *page, const QString &name) override;

    bool validate(Utils::MacroExpander *expander, QString *message) override;
    void initializeData(Utils::MacroExpander *expander) override;

    QString m_defaultText;
    bool m_acceptRichText = false;
    QString m_disabledText;

    mutable QString m_currentText;
};

class PathChooserField : public JsonFieldPage::Field
{
private:
    bool parseData(const QVariant &data, QString *errorMessage) override;

    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;
    void setEnabled(bool e) override;

    void setup(JsonFieldPage *page, const QString &name) override;

    bool validate(Utils::MacroExpander *expander, QString *message) override;
    void initializeData(Utils::MacroExpander *expander) override;

    QString m_path;
    QString m_basePath;
    QString m_historyId;
    Utils::PathChooser::Kind m_kind = Utils::PathChooser::ExistingDirectory;

    QString m_currentPath;
};

class CheckBoxField : public JsonFieldPage::Field
{
public:
    bool suppressName() const override { return true; }

private:
    bool parseData(const QVariant &data, QString *errorMessage) override;

    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;

    void setup(JsonFieldPage *page, const QString &name) override;

    bool validate(Utils::MacroExpander *expander, QString *message) override;
    void initializeData(Utils::MacroExpander *expander) override;

    QString m_checkedValue = QString("0");
    QString m_uncheckedValue = QString("1");
    QVariant m_checkedExpression;

    bool m_isModified = false;
};

class ComboBoxField : public JsonFieldPage::Field
{
private:
    bool parseData(const QVariant &data, QString *errorMessage) override;

    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;

    void setup(JsonFieldPage *page, const QString &name) override;

    bool validate(Utils::MacroExpander *expander, QString *message) override;
    void initializeData(Utils::MacroExpander *expander) override;

    QStringList m_itemList;
    QStringList m_itemDataList;
    QVariantList m_itemConditionList;
    int m_index = -1;
    int m_disabledIndex = -1;

    mutable int m_savedIndex = -1;
};

} // namespace ProjectExplorer
