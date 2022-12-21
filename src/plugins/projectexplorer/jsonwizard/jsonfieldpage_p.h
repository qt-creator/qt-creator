// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonfieldpage.h"

#include <utils/fileutils.h>
#include <utils/pathchooser.h>

#include <QWidget>
#include <QString>
#include <QVariant>
#include <QStandardItem>
#include <QDir>
#include <QTextStream>

#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE
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
    QString toString() const override
    {
        QString result;
        QTextStream out(&result);
        out << "LabelField{text:" << m_text << "}";
        return result;
    }

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
    QString toString() const override
    {
        QString result;
        QTextStream out(&result);
        out << "SpacerField{factor:" << m_factor << "}";
        return result;
    }

    bool parseData(const QVariant &data, QString *errorMessage) override;
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;

    int m_factor = 1;
};

class PROJECTEXPLORER_EXPORT LineEditField : public JsonFieldPage::Field
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

    QString toString() const override
    {
        QString result;
        QTextStream out(&result);
        out << "LineEditField{currentText:" << m_currentText
            << "; default:" << m_defaultText
            << "; placeholder:" << m_placeholderText
            << "; history id:" << m_historyId
            << "; validator: " << m_validatorRegExp.pattern()
            << "; fixupExpando: " << m_fixupExpando
            << "; completion: " << QString::number((int)m_completion)
            << "}";
        return result;

    }

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

public:
    void setText(const QString &text);
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

    QString toString() const override
    {
        QString result;
        QTextStream out(&result);
        out << "TextEditField{default:" << m_defaultText
            << "; rich:" << m_acceptRichText
            << "; disabled: " << m_disabledText
            << "}";
        return result;
    }

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

    QString toString() const override
    {
        QString result;
        QTextStream out(&result);
        out << "PathChooser{path:" << m_path.toString()
            << "; base:" << m_basePath
            << "; historyId:" << m_historyId
            << "; kind:" << (int)Utils::PathChooser::ExistingDirectory
            << "}";
        return result;
    }

    Utils::FilePath m_path;
    Utils::FilePath m_basePath;
    QString m_historyId;
    Utils::PathChooser::Kind m_kind = Utils::PathChooser::ExistingDirectory;
};

class PROJECTEXPLORER_EXPORT CheckBoxField : public JsonFieldPage::Field
{
public:
    bool suppressName() const override { return true; }

    void setChecked(bool);

private:
    bool parseData(const QVariant &data, QString *errorMessage) override;

    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;

    void setup(JsonFieldPage *page, const QString &name) override;

    bool validate(Utils::MacroExpander *expander, QString *message) override;
    void initializeData(Utils::MacroExpander *expander) override;
    void fromSettings(const QVariant &value) override;
    QVariant toSettings() const override;

    QString toString() const override
    {
        QString result;
        QTextStream out(&result);
        out << "CheckBoxField{checked:" << m_checkedValue
            << "; unchecked: " + m_uncheckedValue
            << "; checkedExpression: QVariant("
            << m_checkedExpression.typeName() << ":" << m_checkedExpression.toString()
            <<  ")"
            << "; isModified:" << m_isModified
            << "}";
        return result;
    }

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

    QStandardItemModel *model() { return m_itemModel; }
    virtual bool selectRow(int row);

protected:
    bool parseData(const QVariant &data, QString *errorMessage) override;

    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override = 0;
    void setup(JsonFieldPage *page, const QString &name) override = 0;

    bool validate(Utils::MacroExpander *expander, QString *message) override;
    void initializeData(Utils::MacroExpander *expander) override;
    QStandardItemModel *itemModel();
    QItemSelectionModel *selectionModel() const;
    void setSelectionModel(QItemSelectionModel *selectionModel);
    QSize maxIconSize() const;

    QString toString() const override
    {
        QString result;
        QTextStream out(&result);
        out << "ListField{index:" << m_index
            << "; disabledIndex:" << m_disabledIndex
            << "; savedIndex: " << m_savedIndex
            << "; items Count: " << m_itemList.size()
            << "; items:";

        if (m_itemList.empty())
            out << "(empty)";
        else
            out << m_itemList.front()->text() << ", ...";

        out << "}";

        return result;
    }

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

class PROJECTEXPLORER_EXPORT ComboBoxField : public ListField
{
private:
    void setup(JsonFieldPage *page, const QString &name) override;
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;
    void initializeData(Utils::MacroExpander *expander) override;
    QVariant toSettings() const override;

    QString toString() const override
    {
        QString result;
        QTextStream out(&result);
        out << "ComboBox{" << ListField::toString() << "}";
        return result;
    }

public:
    bool selectRow(int row) override;
    int selectedRow() const;
};

class IconListField : public ListField
{
public:
    void setup(JsonFieldPage *page, const QString &name) override;
    QWidget *createWidget(const QString &displayName, JsonFieldPage *page) override;
    void initializeData(Utils::MacroExpander *expander) override;

    QString toString() const override
    {
        QString result;
        QTextStream out(&result);
        out << "IconList{" << ListField::toString()<< "}";
        return result;
    }
};

} // namespace ProjectExplorer
