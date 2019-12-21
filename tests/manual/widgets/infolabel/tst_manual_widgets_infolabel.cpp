/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QApplication>
#include <QFormLayout>
#include <QLayout>
#include <QMessageBox>
#include <QSettings>

#include <utils/infolabel.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <coreplugin/manhattanstyle.h>

using namespace Utils;

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);

    Theme theme("");
    QSettings settings(":/themes/flat.creatortheme", QSettings::IniFormat);
    theme.readSettings(settings);
    setCreatorTheme(&theme);
    StyleHelper::setBaseColor(QColor(StyleHelper::DEFAULT_BASE_COLOR));
    QApplication::setStyle(new ManhattanStyle(creatorTheme()->preferredStyles().value(0)));

    auto widget = new QWidget;
    auto mainLayout = new QVBoxLayout(widget);
    auto variationsLayout = new QGridLayout;
    mainLayout->addLayout(variationsLayout);

    const static struct {
        const InfoLabel::InfoType type;
        const char *text;
    } labels[] = {
        {InfoLabel::Information, "Information"},
        {InfoLabel::Warning, "Warning"},
        {InfoLabel::Error, "Error"},
        {InfoLabel::Ok, "Ok"},
        {InfoLabel::NotOk, "NotOk"},
        {InfoLabel::None, "None"},
    };

    int gridRow = 0;
    for (auto filled : {false, true}) {
        for (auto label : labels) {
            for (auto enabled : {true, false}) {
                auto infoLabel = new Utils::InfoLabel(
                            label.text + QLatin1String(filled ? " (filled)" : "")
                            + QLatin1String(enabled ? "" : " (disabled)"), label.type);
                infoLabel->setEnabled(enabled);
                infoLabel->setFilled(filled);
                variationsLayout->addWidget(infoLabel, gridRow, enabled ? 0 : 1);
            }
            gridRow++;
        }
        variationsLayout->addItem(new QSpacerItem(0, 10), gridRow++, 0);
    }

    auto withLink = new Utils::InfoLabel("With <a href=\"link clicked!\">link</a>", InfoLabel::Error);
    withLink->setElideMode(Qt::ElideNone);
    QObject::connect(withLink, &QLabel::linkActivated, [widget](const QString& link){
        QMessageBox::information(widget, {}, link);
    });
    mainLayout->addWidget(withLink);

    auto stretching = new Utils::InfoLabel("Stretching and centering vertically", InfoLabel::Warning);
    stretching->setFilled(true);
    mainLayout->addWidget(stretching, 2);

    auto formLayout = new QFormLayout;
    auto multiLine = new Utils::InfoLabel("Multi line<br/>in<br/>QFormLayout");
    multiLine->setElideMode(Qt::ElideNone);
    multiLine->setFilled(true);
    formLayout->addRow("Label:", multiLine);
    mainLayout->addLayout(formLayout);

    const QString lorem =
            "Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt "
            "ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
            "ullamco laboris nisi ut aliquid ex ea commodi consequat.";

    mainLayout->addWidget(new Utils::InfoLabel("Qt::ElideRight: " + lorem, InfoLabel::Information));

    auto elideNone = new Utils::InfoLabel("Qt::ElideNone: " + lorem, InfoLabel::Information);
    elideNone->setElideMode(Qt::ElideNone);
    elideNone->setWordWrap(true);
    mainLayout->addWidget(elideNone);

    widget->resize(350, 500);
    widget->show();

    return app.exec();
}
