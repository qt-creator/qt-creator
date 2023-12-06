// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/layoutbuilder.h>
#include <utils/stylehelper.h>

#include <QApplication>
#include <QLabel>
#include <QWidget>

using namespace Utils;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    static const struct {
        StyleHelper::UiElement uiElement;
        const QString name;
    } uiElements[] = {
        { StyleHelper::UiElementH1, "H1" },
        { StyleHelper::UiElementH2, "H2" },
        { StyleHelper::UiElementH3, "H3" },
        { StyleHelper::UiElementH4, "H4" },
        { StyleHelper::UiElementCaptionStrong, "Caption strong" },
        { StyleHelper::UiElementCaption, "Caption" },
    };
    static const QString textSample("AaBbCcXxYyZz123");

    using namespace Layouting;
    Grid fontLabels {};
    for (auto uiElement : uiElements) {
        const QFont font = StyleHelper::uiFont(uiElement.uiElement);
        auto *uiElementLabel = new QLabel(uiElement.name);
        uiElementLabel->setFont(font);
        auto *sampleLabel = new QLabel(textSample);
        sampleLabel->setFont(font);
        fontLabels.addItems({uiElementLabel, sampleLabel, font.toString(), br});
    }

    Column {
        windowTitle("Utils::StyleHelper::uiFont"),
        Group {
            title("As QFont in QLabel"),
            fontLabels,
        },
    }.emerge()->show();

    return app.exec();
}
