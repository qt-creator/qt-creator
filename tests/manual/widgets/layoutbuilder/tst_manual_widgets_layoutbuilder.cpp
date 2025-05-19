// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../common/themeselector.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcwidgets.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>
#include <utils/utilsicons.h>
#include <utils/ranges.h>

#include <coreplugin/welcomepagehelper.h>

#include <QApplication>
#include <QLineEdit>
#include <QMetaEnum>
#include <QStyle>
#include <QTextEdit>
#include <QToolButton>

using namespace Layouting;

// clang-format off
class Counter : public QObject
{
    Q_OBJECT
public:
    Counter() : m_value(0) {}

    void increment() { ++m_value; emit changed(); }
    int value() const { return m_value; }

signals:
    void changed();

protected:
    int m_value;
};
// clang-format on

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto lineEdit = new QLineEdit("0");

    auto minusClick = [lineEdit] {
        lineEdit->setText(QString::number(lineEdit->text().toInt() - 1));
    };

    auto plusClick = [lineEdit] {
        lineEdit->setText(QString::number(lineEdit->text().toInt() + 1));
    };

    // clang-format off
    Row {
        PushButton { text("-"), onClicked(qApp, minusClick) },
        lineEdit,
        PushButton { text("+"), onClicked(qApp, plusClick) },
        Group {
            title("Splitter in Group"),
            Column {
                Splitter {
                    new QTextEdit("First Widget"),
                    new QTextEdit("Second Widget"),
                },
            }
        },
    }.emerge()->show();

    Grid {
        "First Row", br,
        "First Cell of second row",
        GridCell({
            ">These", ">    Should", ">    overlap"
        })
        , "This should be the third cell",
        br,
        "And another row",
        br,
        GridCell({
            Row {
                "Row> The first column ...", "Row> The second column ..."
            },
            Column {
                "Column> The first row", br,
                "Column> The second row",
            }
        }),
        br,
        "And another row", br,
        Span(3,
            GridCell({
                Label {
                    wordWrap(true),
                    text("This should span three columns, its also quite long. So we can actually wrap a bit which allows to resize the windows. Otherwise it would be a bit boring.")
                },
                "Some overlapping text",
            })
        )
    }.emerge()->show();

    Counter myCounter;

    // clang-format off
    Widget {
        windowTitle("Counter with dynamic children"),
        Row {
            PushButton { text("Increment"), onClicked(qApp, [&myCounter](){myCounter.increment();})},
            Widget {
                replaceLayoutOn(&myCounter, &Counter::changed, [&myCounter] {
                    return Row {
                        Label {
                            text(QString::number(myCounter.value()))
                        }
                    };
                })
            }
        }
    }.emerge()->show();
    // clang-format on

    Group {
        windowTitle("Group without parent layout"),
        title("This group was emerged without parent layout"),
        Column {
            Splitter {
                new QTextEdit("First Widget"),
                new QTextEdit("Second Widget"),
            },
        }
    }.emerge()->show();


    Splitter {
        windowTitle("Splitter with sub layouts"),
        Column { QString("First Widget") },
        Row { QString("Second Widget") },
    }.emerge()->show();

    QToolButton *toolButton1;
    QToolButton *toolButton2;
    QToolButton *toolButton3;
    QWidget *flowlayouts = Column {
        Label { wordWrap(true), text("All push buttons:") },
        Flow {
            PushButton { text("button1") },
            PushButton { text("button2") },
            PushButton { text("button3") },
            PushButton { text("button4") },
            PushButton { text("button5") }
        },
        hr,
        Label { wordWrap(true), text("Mixed buttons, that can have different spacing:") },
        Flow {
            PushButton { text("a pushbutton") },
            ToolButton { bindTo(&toolButton1) },
            ToolButton { bindTo(&toolButton2) },
            ToolButton { bindTo(&toolButton3) },
        },
        Label { wordWrap(true), text("Right aligned:") },
        Flow {
            alignment(Qt::AlignRight),
            PushButton { text("button1") },
            PushButton { text("button2") },
            PushButton { text("button3") },
            PushButton { text("button4") },
            PushButton { text("button5") }
        },
        Label { wordWrap(true), text("Spacing:") },
        Flow {
            spacing(1),
            PushButton { text("button1") },
            PushButton { text("button2") },
            PushButton { text("button3") },
            PushButton { text("button4") }
        },
        st
    }.emerge();
    toolButton1->setDefaultAction(new QAction("tool button 1", toolButton1));
    toolButton2->setDefaultAction(new QAction("tool button 2", toolButton2));
    toolButton3->setDefaultAction(
        new QAction(qApp->style()->standardIcon(QStyle::SP_TitleBarCloseButton), "", toolButton2));
    flowlayouts->setWindowTitle("Flow Layouts");
    flowlayouts->adjustSize();
    flowlayouts->show();

    // clang-format off
    using namespace Utils::QtcWidgets;
    Widget {
        windowTitle("Qtc Controls"),

        Column {
            "Theme selector:",
            new ManualTest::ThemeSelector,
            "QtcButton:",
            Flow {
                Utils::transform<QList>(Utils::ranges::MetaEnum<Utils::QtcButton::Role>(), [](int r) {
                    return Button{
                        text(QMetaEnum::fromType<Utils::QtcButton::Role>().valueToKey(r)),
                        role((Utils::QtcButton::Role) r)
                    };
                })
            },
            "QtcButton with Icons:",
            Flow {
                Utils::transform<QList>(Utils::ranges::MetaEnum<Utils::QtcButton::Role>(), [](int r) {
                    return Button{
                        text(QMetaEnum::fromType<Utils::QtcButton::Role>().valueToKey(r)),
                        role((Utils::QtcButton::Role) r),
                        icon(Utils::Icons::PLUS)
                    };
                })
            },
            Row {
                Switch {
                    text("Switch:"),
                    onClicked(qApp, []() { qDebug() << "Switch clicked"; })
                },
                st,
            },
            "QtcLabel:",
            Row {
                Utils::QtcWidgets::Label { text("Primary label"), role(Utils::QtcLabel::Primary) },
                Utils::QtcWidgets::Label { text("Secondary label"), role(Utils::QtcLabel::Secondary) },
                st,
            },
            "QtcSearchBox:",
            Utils::QtcWidgets::SearchBox {
                placeholderText("Search example..."),
                onTextChanged(qApp, [](const QString &text){ qDebug() << "Text:" << text; })
            },
        }
    }.emerge()->show();
    // clang-format on

    return app.exec();
}

#include "tst_manual_widgets_layoutbuilder.moc"
