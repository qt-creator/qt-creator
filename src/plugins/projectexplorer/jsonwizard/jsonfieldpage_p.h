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
#include <QDir>

#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE
class QStandardItem;
class QStandardItemModel;
class QItemSelectionModel;
QT_END_NAMESPACE

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
    bool m_hasUserChanges = false;

    QVariant m_visibleExpression;
    QVariant m_enabledExpression;
    QVariant m_isCompleteExpando;
    QString m_isCompleteExpandoMessage;
    QString m_persistenceKey;

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

    void fromSettings(const QVariant &value) override;
    QVariant toSettings() const override;

    void setupCompletion(Utils::FancyLineEdit *lineEdit);

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

    enum class Completion { Classes, Namespaces, None };
    Completion m_completion = Completion::None;
};

class TextEditField : public JsonFieldPage::Field
{
private:
    bool parseData(const QVariant &data, QString *errorMessage) override;
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;

    void setup(JsonFieldPage *page, const QString &name) override;

    bool validate(Utils::MacroExpander *expander, QString *message) override;
    void initializeData(Utils::MacroExpander *expander) override;

    void fromSettings(const QVariant &value) override;
    QVariant toSettings() const override;

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

    void fromSettings(const QVariant &value) override;
    QVariant toSettings() const override;

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
    void fromSettings(const QVariant &value) override;
    QVariant toSettings() const override;

    QString m_checkedValue;
    QString m_uncheckedValue;
    QVariant m_checkedExpression;

    bool m_isModified = false;
};

class ListField : public JsonFieldPage::Field
{
public:
    enum SpecialRoles {
        ValueRole = Qt::UserRole,
        ConditionRole = Qt::UserRole + 1,
        IconStringRole = Qt::UserRole + 2
    };
    ListField();
    ~ListField() override;

    protected:
    bool parseData(const QVariant &data, QString *errorMessage) override;

    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override = 0;
    void setup(JsonFieldPage *page, const QString &name) override = 0;

    bool validate(Utils::MacroExpander *expander, QString *message) override;
    void initializeData(Utils::MacroExpander *expander) override;
    QStandardItemModel *itemModel();
    QItemSelectionModel *selectionModel() const;
    void setSelectionModel(QItemSelectionModel *selectionModel);
    QSize maxIconSize();

private:
    void addPossibleIconSize(const QIcon &icon);
    void updateIndex();

    void fromSettings(const QVariant &value) override;
    QVariant toSettings() const override;

    std::vector<std::unique_ptr<QStandardItem>> m_itemList;
    QStandardItemModel *m_itemModel = nullptr;
    QItemSelectionModel *m_selectionModel = nullptr;
    int m_index = -1;
    int m_disabledIndex = -1;
    QSize m_maxIconSize;

    mutable int m_savedIndex = -1;
};

class ComboBoxField : public ListField
{
private:
    void setup(JsonFieldPage *page, const QString &name) override;
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;
    void initializeData(Utils::MacroExpander *expander) override;
    QVariant toSettings() const override;
};

class IconListField : public ListField
{
public:
    void setup(JsonFieldPage *page, const QString &name) override;
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;
    void initializeData(Utils::MacroExpander *expander) override;
};

} // namespace ProjectExplorer
