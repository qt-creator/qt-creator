// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "onboardingwizard.h"

#include "learningtr.h"
#include "learningsettings.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/overlaywidget.h>
#include <utils/qtcwidgets.h>
#include <utils/stylehelper.h>

#include <coreplugin/welcomepagehelper.h>

#include <QPainter>
#include <QRadioButton>
#include <QWidget>

using namespace Utils;
using namespace Utils::StyleHelper;

namespace Learning::Internal {

class RecommendationsOptionsPage final : public QWidget
{
public:
    RecommendationsOptionsPage(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        auto levelBasic = new QRadioButton(Tr::tr("Basic"));
        auto levelAdvanced = new QRadioButton(Tr::tr("Advanced"));
        (level() == QLatin1String(EXPERIENCE_PREFIX) + EXPERIENCE_BASIC ? levelBasic
                                                                        : levelAdvanced)
            ->setChecked(true);

        using namespace Layouting;
        auto vSpace = Space(SpacingTokens::GapVXl);
        Column {
            tfLabel(Tr::tr("Recommend content"), pageHeaderTf),
            Tr::tr("Qt Creator suggests tutorials, courses and examples based on your "
                   "experience and needs."),
            vSpace,
            tfLabel(Tr::tr("Your experience level"), sectionHeaderTf),
            Row { levelBasic, levelAdvanced, st },
            vSpace,
            tfLabel(Tr::tr("Select your target platforms"), sectionHeaderTf),
            Flow {
                targetButton(Tr::tr("Desktop"), TARGET_DESKTOP),
                targetButton(Tr::tr("Android"), TARGET_ANDROID),
                targetButton(Tr::tr("iOS"), TARGET_IOS),
                targetButton(Tr::tr("Boot2Qt"), TARGET_BOOT2QT),
                targetButton(Tr::tr("Qt for MCUs"), TARGET_QTFORMCUS),
            },
            noMargin,
        }.attachTo(this);

        connect(levelBasic, &QAbstractButton::clicked, this, [] {
            setLevel(EXPERIENCE_BASIC);
        });
        connect(levelAdvanced, &QAbstractButton::clicked, this, [] {
            setLevel(EXPERIENCE_ADVANCED);
        });
    }

private:
    static void setLevel(const QString &level)
    {
        QStringList userFlags = Utils::filtered(settings().userFlags(), [](const QString &flag) {
            return !flag.startsWith(EXPERIENCE_PREFIX);
        });
        userFlags.append(EXPERIENCE_PREFIX + level);
        settings().userFlags.setValue(userFlags);
    }

    static QString level()
    {
        QStringList userFlags = Utils::filtered(settings().userFlags(), [](const QString &flag) {
            return flag.startsWith(EXPERIENCE_PREFIX);
        });
        return userFlags.isEmpty() ? settings().defaultExperience() : userFlags.first();
    }

    static QAbstractButton *targetButton(const QString &text, const QString &target)
    {
        const QString targetWithPrefix = TARGET_PREFIX + target;
        auto button = new QtcButton(text, QtcButton::Tag);
        button->setCheckable(true);
        button->setChecked(settings().userFlags().contains(targetWithPrefix));
        button->setProperty(TARGET_PREFIX, targetWithPrefix);
        QObject::connect(button, &QAbstractButton::toggled, button, [button]{
            const QString target = button->property(TARGET_PREFIX).toString();
            QStringList userFlags = settings().userFlags();
            userFlags.removeAll(target);
            if (button->isChecked())
                userFlags.append(target);
            settings().userFlags.setValue(userFlags);
        });
        return button;
    }

    static constexpr TextFormat pageHeaderTf
        {Theme::Token_Text_Default, UiElement::UiElementH3};
    static constexpr TextFormat sectionHeaderTf
        {pageHeaderTf.themeColor, UiElement::UiElementH5};
    static constexpr TextFormat captionTf
        {Theme::Token_Text_Muted, UiElement::UiElementCaption};
};

QWidget *createOnboardingWizard(QWidget *parent)
{
    auto closeButton = new QtcButton(Tr::tr("Close"), QtcButton::LargePrimary);

    auto widget = new OverlayWidget(parent);
    widget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    widget->setPaintFunction(
        [](QWidget *that, QPainter &p, QPaintEvent *)
        {
            QColor color = creatorColor(Theme::Token_Background_Default);
            color.setAlpha(210);
            p.fillRect(that->rect(), color);
        });
    widget->attachToWidget(parent);

    using namespace Layouting;
    QWidget *wizard = Column {
        QtcWidgets::Rectangle {
            radius(SpacingTokens::PrimitiveL),
            fillBrush(creatorColor(Theme::Token_Background_Muted)),
            strokePen(creatorColor(Core::WelcomePageHelpers::cardDefaultStroke)),
            Column {
                new RecommendationsOptionsPage,
                Row { st, closeButton },
                spacing(SpacingTokens::GapVL),
            },
        },
    }.emerge();

    Grid {
        GridCell({ Align(Qt::AlignCenter, wizard) }),
    }.attachTo(widget);

    QObject::connect(closeButton, &QAbstractButton::clicked, widget, [widget] {
        settings().showWizardOnStart.setValue(false);
        settings().writeSettings();
        widget->hide();
    });

    return widget;
}

QWidget *tfLabel(const QString &text, const TextFormat &format)
{
    auto label = new QLabel(text);
    applyTf(label, format);
    label->setFixedHeight(label->height() * 1.2); // HACK: many lineHeights are too low
    return label;
}

} // namespace Learning::Internal
