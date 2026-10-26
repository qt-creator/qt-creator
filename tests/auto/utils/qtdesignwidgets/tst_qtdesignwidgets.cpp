// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/qtdesignwidgets.h>
#include <utils/theme/theme.h>

#include <QBoxLayout>
#include <QImage>
#include <QMetaEnum>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

using namespace Utils;

class tst_QtDesignWidgets : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void separatorIsCenteredInTheGap_data();
    void separatorIsCenteredInTheGap();

private:
    QTemporaryDir m_dir;
};

// A theme whose every color is red, leaving out the palette colors so that
// the widgets keep the background they are given.
void tst_QtDesignWidgets::initTestCase()
{
    QVERIFY(m_dir.isValid());

    const QString path = m_dir.filePath("red.creatortheme");
    {
        QSettings settings(path, QSettings::IniFormat);
        settings.beginGroup("Colors");
        const QMetaEnum colors = QMetaEnum::fromType<Theme::Color>();
        for (int i = 0; i < colors.keyCount(); ++i) {
            if (i < Theme::PaletteWindow || i > Theme::PaletteAccentDisabled)
                settings.setValue(QLatin1String(colors.key(i)), "ff0000");
        }
        settings.endGroup();
    }

    QSettings settings(path, QSettings::IniFormat);
    auto theme = new Theme("red");
    theme->readSettings(settings);
    setCreatorTheme(theme);
}

void tst_QtDesignWidgets::cleanupTestCase()
{
    setCreatorTheme(nullptr);
}

void tst_QtDesignWidgets::separatorIsCenteredInTheGap_data()
{
    QTest::addColumn<QBoxLayout::Direction>("direction");
    QTest::addColumn<int>("spacing");

    for (const int spacing : {QtcSeparatedItemsWidget::separatorLineWidth(), 3}) {
        QTest::addRow("vertical, spacing %d", spacing) << QBoxLayout::TopToBottom << spacing;
        QTest::addRow("horizontal, spacing %d", spacing) << QBoxLayout::LeftToRight << spacing;
        QTest::addRow("mirrored, spacing %d", spacing) << QBoxLayout::RightToLeft << spacing;
    }
}

void tst_QtDesignWidgets::separatorIsCenteredInTheGap()
{
    QFETCH(QBoxLayout::Direction, direction);
    QFETCH(int, spacing);

    const int itemExtent = 10;

    QtcSeparatedItemsWidget widget;
    widget.setAttribute(Qt::WA_DontShowOnScreen);
    widget.setSeparatorInset(0);
    widget.setAutoFillBackground(true);
    QPalette palette = widget.palette();
    palette.setColor(QPalette::Window, Qt::white);
    widget.setPalette(palette);

    auto layout = new QBoxLayout(direction, &widget);
    layout->setContentsMargins({});
    layout->setSpacing(spacing);
    palette.setColor(QPalette::Window, Qt::blue);
    for (int i = 0; i < 2; ++i) {
        auto item = new QWidget;
        item->setFixedSize(itemExtent, itemExtent);
        item->setAutoFillBackground(true);
        item->setPalette(palette);
        layout->addWidget(item);
    }
    widget.show();

    QImage image(widget.size(), QImage::Format_RGB32);
    widget.render(&image);

    const int gapCenter = itemExtent + spacing / 2;
    const QPoint point = direction == QBoxLayout::TopToBottom
                             ? QPoint(itemExtent / 2, gapCenter)
                             : QPoint(gapCenter, itemExtent / 2);
    QCOMPARE(image.pixelColor(point).rgb(), QColor(Qt::red).rgb());
}

QTEST_MAIN(tst_QtDesignWidgets)

#include "tst_qtdesignwidgets.moc"
