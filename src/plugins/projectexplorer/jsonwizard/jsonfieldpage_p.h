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

#ifndef JSONFIELDPAGE_P_H
#define JSONFIELDPAGE_P_H

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

    QWidget *m_widget = 0;
};

// --------------------------------------------------------------------
// Field Implementations:
// --------------------------------------------------------------------

class LabelField : public JsonFieldPage::Field
{
public:
    LabelField();

private:
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page);
    bool parseData(const QVariant &data, QString *errorMessage);

    bool m_wordWrap;
    QString m_text;
};

class SpacerField : public JsonFieldPage::Field
{
public:
    SpacerField();

    bool suppressName() const { return true; }

private:
    bool parseData(const QVariant &data, QString *errorMessage);
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page);

    int m_factor;
};

class LineEditField : public JsonFieldPage::Field
{
public:
    LineEditField();

private:
    bool parseData(const QVariant &data, QString *errorMessage);
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page);

    void setup(JsonFieldPage *page, const QString &name);

    bool validate(Utils::MacroExpander *expander, QString *message);
    void initializeData(Utils::MacroExpander *expander);

    bool m_isModified;
    bool m_isValidating;
    bool m_restoreLastHistoryItem;
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
public:
    TextEditField();

private:
    bool parseData(const QVariant &data, QString *errorMessage);
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page);

    void setup(JsonFieldPage *page, const QString &name);

    bool validate(Utils::MacroExpander *expander, QString *message);
    void initializeData(Utils::MacroExpander *expander);

    QString m_defaultText;
    bool m_acceptRichText;
    QString m_disabledText;

    mutable QString m_currentText;
};

class PathChooserField : public JsonFieldPage::Field
{
public:
    PathChooserField();

private:
    bool parseData(const QVariant &data, QString *errorMessage);

    QWidget *createWidget(const QString &displayName, JsonFieldPage *page);
    void setEnabled(bool e);

    void setup(JsonFieldPage *page, const QString &name);

    bool validate(Utils::MacroExpander *expander, QString *message);
    void initializeData(Utils::MacroExpander *expander);

    QString m_path;
    QString m_basePath;
    QString m_historyId;
    Utils::PathChooser::Kind m_kind;

    QString m_currentPath;
};

class CheckBoxField : public JsonFieldPage::Field
{
public:
    CheckBoxField();

    bool suppressName() const { return true; }

private:
    bool parseData(const QVariant &data, QString *errorMessage);

    QWidget *createWidget(const QString &displayName, JsonFieldPage *page);

    void setup(JsonFieldPage *page, const QString &name);

    bool validate(Utils::MacroExpander *expander, QString *message);
    void initializeData(Utils::MacroExpander *expander);

    QString m_checkedValue;
    QString m_uncheckedValue;
    QVariant m_checkedExpression;

    bool m_isModified;
};

class ComboBoxField : public JsonFieldPage::Field
{
public:
    ComboBoxField();

private:
    bool parseData(const QVariant &data, QString *errorMessage);

    QWidget *createWidget(const QString &displayName, JsonFieldPage *page);

    void setup(JsonFieldPage *page, const QString &name);

    bool validate(Utils::MacroExpander *expander, QString *message);
    void initializeData(Utils::MacroExpander *expander);

    QStringList m_itemList;
    QStringList m_itemDataList;
    QVariantList m_itemConditionList;
    int m_index;
    int m_disabledIndex;

    mutable int m_savedIndex;
};

} // namespace ProjectExplorer

#endif // JSONFIELDPAGE_P_H
