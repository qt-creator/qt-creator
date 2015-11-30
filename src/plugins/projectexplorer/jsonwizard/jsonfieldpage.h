/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef JSONFIELDPAGE_H
#define JSONFIELDPAGE_H

#include "../projectexplorer_export.h"

#include <utils/pathchooser.h>
#include <utils/wizardpage.h>

#include <QRegularExpression>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLabel;
class QLineEdit;
class QTextEdit;
QT_END_NAMESPACE

namespace Utils {
class MacroExpander;
class TextFieldCheckBox;
class TextFieldComboBox;
} // namespace Utils

namespace ProjectExplorer {

// Documentation inside.
class PROJECTEXPLORER_EXPORT JsonFieldPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    class PROJECTEXPLORER_EXPORT Field
    {
    public:
        class FieldPrivate;

        Field();
        virtual ~Field();

        static Field *parse(const QVariant &input, QString *errorMessage);
        void createWidget(JsonFieldPage *page);

        void adjustState(Utils::MacroExpander *expander);
        virtual void setEnabled(bool e);
        void setVisible(bool v);

        virtual bool validate(Utils::MacroExpander *expander, QString *message);

        void initialize(Utils::MacroExpander *expander);
        virtual void cleanup(Utils::MacroExpander *expander) {  Q_UNUSED(expander); }

        virtual bool suppressName() const { return false; }

        QWidget *widget(const QString &displayName, JsonFieldPage *page);

        QString name();
        QString displayName();
        QString toolTip();
        bool isMandatory();
        bool hasSpan();

    protected:
        QWidget *widget() const;
        virtual bool parseData(const QVariant &data, QString *errorMessage) = 0;
        virtual void initializeData(Utils::MacroExpander *expander) { Q_UNUSED(expander); }
        virtual QWidget *createWidget(const QString &displayName, JsonFieldPage *page) = 0;
        virtual void setup(JsonFieldPage *page, const QString &name)
        { Q_UNUSED(page); Q_UNUSED(name); }

    private:
        void setTexts(const QString &n, const QString &dn, const QString &tt);
        void setIsMandatory(bool b);
        void setHasSpan(bool b);

        void setVisibleExpression(const QVariant &v);
        void setEnabledExpression(const QVariant &v);
        void setIsCompleteExpando(const QVariant &v, const QString &m);

        friend class JsonFieldPage;

        FieldPrivate *const d;
    };

    JsonFieldPage(Utils::MacroExpander *expander, QWidget *parent = 0);
    ~JsonFieldPage();

    typedef std::function<Field *()> FieldFactory;
    static void registerFieldFactory(const QString &id, const FieldFactory &ff);

    bool setup(const QVariant &data);

    bool isComplete() const;
    void initializePage();
    void cleanupPage();

    QFormLayout *layout() const { return m_formLayout; }

    void showError(const QString &m) const;
    void clearError() const;

    Utils::MacroExpander *expander();

private:
    static QHash<QString, FieldFactory> m_factories;

    static Field *createFieldData(const QString &type);

    QFormLayout *m_formLayout;
    QLabel *m_errorLabel;

    QList<Field *> m_fields;

    Utils::MacroExpander *m_expander;
};

} // namespace ProjectExplorer

#endif // JSONFIELDPAGE_H
