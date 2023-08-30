// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include <utils/pathchooser.h>
#include <utils/wizardpage.h>

#include <QRegularExpression>
#include <QVariant>

#include <memory>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLabel;
class QLineEdit;
class QTextEdit;
QT_END_NAMESPACE

namespace Utils {
class MacroExpander;
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

        void setType(const QString &type);

        virtual bool validate(Utils::MacroExpander *expander, QString *message);

        void initialize(Utils::MacroExpander *expander);
        virtual void cleanup(Utils::MacroExpander *expander) { Q_UNUSED(expander) }

        virtual bool suppressName() const { return false; }

        QWidget *widget(const QString &displayName, JsonFieldPage *page);

        QString name() const;
        QString displayName() const;
        QString toolTip() const;
        QString persistenceKey() const;
        bool isMandatory() const;
        bool hasSpan() const;
        bool hasUserChanges() const;

    protected:
        QWidget *widget() const;
        virtual bool parseData(const QVariant &data, QString *errorMessage) = 0;
        virtual void initializeData(Utils::MacroExpander *expander) { Q_UNUSED(expander) }
        virtual QWidget *createWidget(const QString &displayName, JsonFieldPage *page) = 0;
        virtual void setup(JsonFieldPage *page, const QString &name)
        { Q_UNUSED(page); Q_UNUSED(name) }

        QString type() const;
        void setHasUserChanges();

    private:
        virtual void fromSettings(const QVariant &value);
        virtual QVariant toSettings() const;

        void setTexts(const QString &name, const QString &displayName, const QString &toolTip);
        void setIsMandatory(bool b);
        void setHasSpan(bool b);

        void setVisibleExpression(const QVariant &v);
        void setEnabledExpression(const QVariant &v);
        void setIsCompleteExpando(const QVariant &v, const QString &m);
        void setPersistenceKey(const QString &key);
        virtual QString toString() const = 0;

        friend class JsonFieldPage;
        friend PROJECTEXPLORER_EXPORT QDebug &operator<<(QDebug &d, const Field &f);

        const std::unique_ptr<FieldPrivate> d;
    };

    JsonFieldPage(Utils::MacroExpander *expander, QWidget *parent = nullptr);
    ~JsonFieldPage() override;

    using FieldFactory = std::function<Field *()>;
    static void registerFieldFactory(const QString &id, const FieldFactory &ff);

    bool setup(const QVariant &data);

    bool isComplete() const override;
    void initializePage() override;
    void cleanupPage() override;
    bool validatePage() override;

    QFormLayout *layout() const { return m_formLayout; }

    void showError(const QString &m) const;
    void clearError() const;

    Utils::MacroExpander *expander();

    QVariant value(const QString &key);

public:
    Field *jsonField(const QString &name);

private:
    static QHash<QString, FieldFactory> m_factories;

    static Field *createFieldData(const QString &type);

    QFormLayout *m_formLayout;
    QLabel *m_errorLabel;

    QList<Field *> m_fields;

    Utils::MacroExpander *m_expander;
};

PROJECTEXPLORER_EXPORT QDebug &operator<<(QDebug &debug, const JsonFieldPage::Field &field);

} // namespace ProjectExplorer
