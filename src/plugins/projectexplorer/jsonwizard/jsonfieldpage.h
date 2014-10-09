/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef JSONFIELDPAGE_H
#define JSONFIELDPAGE_H

#include <utils/pathchooser.h>
#include <utils/wizardpage.h>

#include <QVariant>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLabel;
class QLineEdit;
class QTextEdit;
QT_END_NAMESPACE

namespace Utils {
class AbstractMacroExpander;
class TextFieldCheckBox;
class TextFieldComboBox;
} // namespace Utils

namespace ProjectExplorer {

// Documentation inside.
class JsonFieldPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    class Field
    {
    public:
        Field() : mandatory(false), span(false), m_visibleExpression(true), m_widget(0) { }
        virtual ~Field() { delete m_widget; }

        static Field *parse(const QVariant &input, QString *errorMessage);
        void createWidget(JsonFieldPage *page);

        void adjustState(Utils::AbstractMacroExpander *expander);
        virtual void setEnabled(bool e) { m_widget->setEnabled(e); }
        void setVisible(bool v) { m_widget->setVisible(v); }

        virtual bool validate(Utils::AbstractMacroExpander *expander, QString *message)
        { Q_UNUSED(expander); Q_UNUSED(message); return true; }

        void initialize(Utils::AbstractMacroExpander *expander);
        virtual void cleanup(Utils::AbstractMacroExpander *expander) {  Q_UNUSED(expander); }

        virtual bool suppressName() const { return false; }

        QString name;
        QString displayName;
        bool mandatory;
        bool span;

    protected:
        QVariant m_visibleExpression;
        QVariant m_enabledExpression;

        virtual bool parseData(const QVariant &data, QString *errorMessage) = 0;
        virtual void initializeData(Utils::AbstractMacroExpander *expander) { Q_UNUSED(expander); }
        virtual QWidget *widget(const QString &displayName) = 0;
        virtual void setup(JsonFieldPage *page, const QString &name)
        { Q_UNUSED(page); Q_UNUSED(name); }

        QWidget *m_widget;
    };

    class LabelField : public Field
    {
    public:
        LabelField();

    private:
        QWidget *widget(const QString &displayName);
        bool parseData(const QVariant &data, QString *errorMessage);

        bool m_wordWrap;
        QString m_text;
    };

    class SpacerField : public Field
    {
    public:
        SpacerField();

        bool suppressName() const { return true; }

    private:
        bool parseData(const QVariant &data, QString *errorMessage);
        QWidget *widget(const QString &displayName);

        int m_factor;
    };

    class LineEditField : public Field
    {
    public:
        LineEditField();

    private:
        bool parseData(const QVariant &data, QString *errorMessage);
        QWidget *widget(const QString &displayName);

        void setup(JsonFieldPage *page, const QString &name);

        bool validate(Utils::AbstractMacroExpander *expander, QString *message);
        void initializeData(Utils::AbstractMacroExpander *expander);

        QString m_placeholderText;
        QString m_defaultText;
        QString m_disabledText;

        bool m_isModified;
        mutable QString m_currentText;
    };

    class TextEditField : public Field
    {
    public:
        TextEditField();

    private:
        bool parseData(const QVariant &data, QString *errorMessage);
        QWidget *widget(const QString &displayName);

        void setup(JsonFieldPage *page, const QString &name);

        bool validate(Utils::AbstractMacroExpander *expander, QString *message);
        void initializeData(Utils::AbstractMacroExpander *expander);

        QString m_defaultText;
        bool m_acceptRichText;
        QString m_disabledText;

        mutable QString m_currentText;
    };

    class PathChooserField : public Field
    {
    public:
        PathChooserField();

    private:
        bool parseData(const QVariant &data, QString *errorMessage);

        QWidget *widget(const QString &displayName);
        void setEnabled(bool e);

        void setup(JsonFieldPage *page, const QString &name);

        bool validate(Utils::AbstractMacroExpander *expander, QString *message);
        void initializeData(Utils::AbstractMacroExpander *expander);

        QString m_path;
        QString m_basePath;
        Utils::PathChooser::Kind m_kind;

        QString m_currentPath;
    };

    class CheckBoxField : public Field
    {
    public:
        CheckBoxField();

        bool suppressName() const { return true; }

    private:
        bool parseData(const QVariant &data, QString *errorMessage);

        QWidget *widget(const QString &displayName);

        void setup(JsonFieldPage *page, const QString &name);

        bool validate(Utils::AbstractMacroExpander *expander, QString *message);
        void initializeData(Utils::AbstractMacroExpander *expander);

        QString m_checkedValue;
        QString m_uncheckedValue;
        QVariant m_checkedExpression;

        bool m_isModified;
    };

    class ComboBoxField : public Field
    {
    public:
        ComboBoxField();

    private:
        bool parseData(const QVariant &data, QString *errorMessage);

        QWidget *widget(const QString &displayName);

        void setup(JsonFieldPage *page, const QString &name);

        bool validate(Utils::AbstractMacroExpander *expander, QString *message);
        void initializeData(Utils::AbstractMacroExpander *expander);

        QStringList m_itemList;
        QStringList m_itemDataList;
        int m_index;
        int m_disabledIndex;

        mutable int m_savedIndex;
        int m_currentIndex;
    };

    JsonFieldPage(Utils::AbstractMacroExpander *expander, QWidget *parent = 0);
    ~JsonFieldPage();

    bool setup(const QVariant &data);

    bool isComplete() const;
    void initializePage();
    void cleanupPage();

    QFormLayout *layout() const { return m_formLayout; }

    void showError(const QString &m) const;
    void clearError() const;

    Utils::AbstractMacroExpander *expander();

private:
    QFormLayout *m_formLayout;
    QLabel *m_errorLabel;

    QList<Field *> m_fields;

    Utils::AbstractMacroExpander *m_expander;
};

} // namespace ProjectExplorer

#endif // JSONFIELDPAGE_H
