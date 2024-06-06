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
        { StyleHelper::UiElementH5, "H5" },
        { StyleHelper::UiElementH6, "H6" },
        { StyleHelper::UiElementH6Capital, "H6 CAPITAL" },
        { StyleHelper::UiElementBody1, "Body-01" },
        { StyleHelper::UiElementBody2, "Body-02" },
        { StyleHelper::UiElementButtonMedium, "Button Medium" },
        { StyleHelper::UiElementButtonSmall, "Button Small" },
        { StyleHelper::UiElementLabelMedium, "Label Medium" },
        { StyleHelper::UiElementLabelSmall, "Label Small" },
        { StyleHelper::UiElementCaptionStrong, "Caption strong" },
        { StyleHelper::UiElementCaption, "Caption" },
        { StyleHelper::UiElementIconStandard, "Icon Standard" },
        { StyleHelper::UiElementIconActive, "Icon Active" },
    };
    static const QString textSample("AaBbCcXxYyZz123");

    using namespace Layouting;
    Grid fontLabels;
    for (auto uiElement : uiElements) {
        const QFont font = StyleHelper::uiFont(uiElement.uiElement);
        auto *uiElementLabel = new QLabel(uiElement.name);
        uiElementLabel->setFont(font);
        uiElementLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        auto *sampleLabel = new QLabel(textSample);
        sampleLabel->setFont(font);
        sampleLabel->setSizePolicy(uiElementLabel->sizePolicy());
        QFontInfo fontInfo(font);
        const QString pixelMetrics = QString::fromLatin1("Pixel size: %1, line height: %2")
                                         .arg(fontInfo.pixelSize())
                                         .arg(StyleHelper::uiFontLineHeight(uiElement.uiElement));
        auto *metricsLabel = new QLabel(pixelMetrics);
        fontLabels.addItems({uiElementLabel, sampleLabel, st, font.toString(), metricsLabel, br});
    }

    QString html("<html><body><table>");
    for (auto uiElement : uiElements) {
        const QFont font = StyleHelper::uiFont(uiElement.uiElement);
        html.append(QString(R"(
            <tr>
                <td style="%1">%2</td>
                <td style="%1">%3</td>
                <td>%1</td>
            </tr>
        )").arg(StyleHelper::fontToCssProperties(font)).arg(uiElement.name).arg(textSample));
    }
    html.append("</table></body></html>");

    Widget {
        windowTitle(QString("Utils::StyleHelper::uiFont")),
        Column {
            Group {
                title(QString("As QFont in QLabel")),
                fontLabels,
            },
            Group {
                title(QString("As inline CSS in HTML elements")),
                Row { html },
            },
            st,
        }
    }.show();

    return app.exec();
}
