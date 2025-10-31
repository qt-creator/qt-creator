// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <QVariant>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QToolBar;
QT_END_NAMESPACE

namespace Utils {
class BoolAspect;
class IntegerAspect;
class StringAspect;
} // Utils

namespace VcsBase {

namespace Internal { class VcsBaseEditorConfigPrivate; }

// Documentation->inside.
class VCSBASE_EXPORT VcsBaseEditorConfig : public QObject
{
    Q_OBJECT

public:
    explicit VcsBaseEditorConfig(QToolBar *toolBar);
    ~VcsBaseEditorConfig() override;

    class VCSBASE_EXPORT ChoiceItem
    {
    public:
        ChoiceItem() = default;
        ChoiceItem(const QString &text, const QVariant &val);
        QString displayText;
        QVariant value;
    };

    QStringList baseArguments() const;
    void setBaseArguments(const QStringList &);

    QAction *addReloadButton();
    QAction *addToggleButton(const QString &option, const QString &label,
                             const QString &tooltip = {});
    QAction *addToggleButton(const QStringList &options, const QString &label,
                             const QString &tooltip = {});
    QComboBox *addChoices(const QString &title,
                          const QStringList &options,
                          const QList<ChoiceItem> &items);

    void mapSetting(QAction *button, Utils::BoolAspect *setting);
    void mapSetting(QComboBox *comboBox, Utils::StringAspect *setting);
    void mapSetting(QComboBox *comboBox, Utils::IntegerAspect *setting);

    // Return the effective arguments according to setting.
    virtual QStringList arguments() const;

    void handleArgumentsChanged();
    void executeCommand();

signals:
    void commandExecutionRequested();

    // Trigger a re-run to show changed output according to new argument list.
    void argumentsChanged();

protected:
    class OptionMapping
    {
    public:
        OptionMapping() = default;
        OptionMapping(const QStringList &optionList, QObject *obj);
        QStringList options;
        QObject *object = nullptr;
    };

    const QList<OptionMapping> &optionMappings() const;
    virtual QStringList argumentsForOption(const OptionMapping &mapping) const;
    void updateMappedSettings();

private:
    void addAction(QAction *action);

    friend class Internal::VcsBaseEditorConfigPrivate;
    Internal::VcsBaseEditorConfigPrivate *const d;
};

} // namespace VcsBase
