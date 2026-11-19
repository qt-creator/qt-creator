// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/elidinglabel.h>

#include <QApplication>
#include <QCursor>
#include <QHelpEvent>
#include <QImage>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QTest>
#include <QTextDocument>
#include <QToolTip>
#include <QVBoxLayout>

#include <memory>

using namespace Utils;

// The tool tip as QToolTip shows it: text that looks like markup is rendered.
static QString shownToolTip(const QWidget &widget)
{
    const QString toolTip = widget.toolTip();
    if (!Qt::mightBeRichText(toolTip))
        return toolTip;
    QTextDocument document;
    document.setHtml(toolTip);
    return document.toPlainText();
}

// grab() runs paintEvent(), which is where the label decides on its tool tip.
static void paint(ElidingLabel &label, int width)
{
    label.resize(width, label.sizeHint().height());
    label.grab();
}

static ElidingLabel *richLabel(const QString &html, Qt::TextElideMode mode = Qt::ElideRight)
{
    auto label = new ElidingLabel;
    label->setElideMode(mode);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setText(html);
    return label;
}

static void click(QWidget *widget, const QPoint &pos)
{
    const QPointF local(pos);
    const QPointF global(widget->mapToGlobal(pos));
    QMouseEvent press(QEvent::MouseButtonPress, local, global, Qt::LeftButton, Qt::LeftButton, {});
    QApplication::sendEvent(widget, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, local, global, Qt::LeftButton, {}, {});
    QApplication::sendEvent(widget, &release);
}

// Moves the mouse to pos and asks for a tool tip there. Returns what QToolTip
// was asked to show, or nothing if the label ignored the request.
static QString hoverToolTip(QWidget *widget, const QPoint &pos)
{
    QToolTip::hideText();
    QMouseEvent move(QEvent::MouseMove, QPointF(pos), QPointF(widget->mapToGlobal(pos)),
                     Qt::NoButton, {}, {});
    QApplication::sendEvent(widget, &move);
    QHelpEvent help(QEvent::ToolTip, pos, widget->mapToGlobal(pos));
    QApplication::sendEvent(widget, &help);
    return help.isAccepted() ? QToolTip::text() : QString();
}

// Where the painted text of the label ends, in logical pixels. Paints it white on
// black to tell text from background.
static int paintedTextEnd(QWidget *widget)
{
    QPalette palette = widget->palette();
    palette.setColor(QPalette::Window, Qt::black);
    palette.setColor(QPalette::WindowText, Qt::white);
    widget->setPalette(palette);
    widget->setAutoFillBackground(true);
    const QImage image = widget->grab().toImage();
    int lastInk = -1;
    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            if (image.pixelColor(x, y).lightness() > 60)
                lastInk = x;
        }
    }
    return lastInk < 0 ? -1 : int(lastInk / image.devicePixelRatio());
}

class tst_ElidingLabel : public QObject
{
    Q_OBJECT

private slots:
    void linkTargetDifferingFromItsTextIsShown();
    void linkNamingItsTargetAddsNothing();
    void everyDifferingLinkTargetIsShown();
    void unelidedLinkHasNoToolTip();
    void markupInPlainTextStaysText();
    void markupInAdditionalToolTipStaysText();
    void elidedLinkKeepsItsColor();
    void clickBesideElidedLinkActivatesNothing();
    void clickOnElidedLinkActivatesTheShownLink();
    void hoveringLinkShowsItsTarget_data();
    void hoveringLinkShowsItsTarget();
    void hoveringLinkWithoutSchemeShowsNoTarget();
    void tabVisitsOnlyShownLinks();
    void focusedElidedLinkIsHighlighted();
    void focusIndicatorOfElidedLinkIsClosed_data();
    void focusIndicatorOfElidedLinkIsClosed();
    void ellipsisInTextIsNotWhereItIsCut();
    void hoveredLinkIsForgottenWhenNoLongerElided();
    void linkHoveredBeforeElidingIsHoveredAgainAfter();
    void urlToolTipClosesOverLinkWithoutScheme();
};

// A window with an elided label of three links, the middle one elided away,
// followed by a line edit to take the focus after the label. Unlike a button,
// it takes the focus by Tab on macOS without full keyboard access, too.
class ThreeLinks
{
public:
    ThreeLinks()
    {
        auto layout = new QVBoxLayout(&window);
        label = richLabel("<a href=\"https://first.example/\">first</a> " + QString(40, 'y')
                              + " <a href=\"https://middle.example/\">middle</a> "
                              + QString(40, 'z') + " <a href=\"https://last.example/\">last</a>",
                          Qt::ElideMiddle);
        next = new QLineEdit;
        layout->addWidget(label);
        layout->addWidget(next);
        window.resize(200, window.sizeHint().height());
    }

    QWidget window;
    ElidingLabel *label = nullptr;
    QLineEdit *next = nullptr;
};

void tst_ElidingLabel::linkTargetDifferingFromItsTextIsShown()
{
    ElidingLabel label;
    label.setTextFormat(Qt::RichText);
    label.setText("<a href=\"http://scam.example/steal\">https://bank.example/login</a>");
    paint(label, 60);

    QCOMPARE(shownToolTip(label),
             QString("https://bank.example/login\nLink: http://scam.example/steal"));
}

void tst_ElidingLabel::linkNamingItsTargetAddsNothing()
{
    const QString url = "https://example.com/authorize?code_challenge=" + QString(100, 'x');
    ElidingLabel label;
    label.setTextFormat(Qt::RichText);
    label.setText(QString("<a href=\"%1\">%1</a>").arg(url));
    paint(label, 60);

    QCOMPARE(shownToolTip(label), url);
}

void tst_ElidingLabel::everyDifferingLinkTargetIsShown()
{
    ElidingLabel label;
    label.setTextFormat(Qt::RichText);
    label.setText("Read <a href=\"https://one.example/\">the first</a>, "
                  "<a href=\"https://two.example/\">https://two.example/</a> and "
                  "<a href=\"https://three.example/\">the third</a>.");
    paint(label, 60);

    QCOMPARE(shownToolTip(label),
             QString("Read the first, https://two.example/ and the third.\n"
                     "Link: https://one.example/\n"
                     "Link: https://three.example/"));
}

void tst_ElidingLabel::unelidedLinkHasNoToolTip()
{
    ElidingLabel label;
    label.setTextFormat(Qt::RichText);
    label.setText("<a href=\"http://scam.example/steal\">bank</a>");
    paint(label, 2000);

    QCOMPARE(label.toolTip(), QString());
}

void tst_ElidingLabel::markupInPlainTextStaysText()
{
    const QString text = "<b>not bold</b> followed by enough text to be elided";
    ElidingLabel label;
    label.setTextFormat(Qt::PlainText);
    label.setText(text);
    paint(label, 60);

    QCOMPARE(shownToolTip(label), text);
}

void tst_ElidingLabel::markupInAdditionalToolTipStaysText()
{
    const QString additional = "<b>not bold</b>\nsecond line";
    ElidingLabel label("short");
    label.setAdditionalToolTip(additional);
    paint(label, 2000);

    QCOMPARE(shownToolTip(label), additional);
}

// QTextDocument takes the link color from the application palette, for QLabel
// as well.
void tst_ElidingLabel::elidedLinkKeepsItsColor()
{
    const QPalette applicationPalette = QApplication::palette();
    QPalette linkPalette = applicationPalette;
    linkPalette.setColor(QPalette::Link, Qt::red);
    QApplication::setPalette(linkPalette);

    std::unique_ptr<ElidingLabel> label(richLabel("<a href=\"https://example.com/\">"
                                                  + QString(80, 'x') + "</a>"));
    QPalette palette = label->palette();
    palette.setColor(QPalette::Window, Qt::black);
    palette.setColor(QPalette::WindowText, Qt::white);
    label->setPalette(palette);
    label->setAutoFillBackground(true);
    label->resize(100, label->sizeHint().height());

    const QImage image = label->grab().toImage();
    QApplication::setPalette(applicationPalette);
    int linkPixels = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.red() > 150 && color.green() < 80 && color.blue() < 80)
                ++linkPixels;
        }
    }
    QVERIFY2(linkPixels > 0, "no pixel of the link color painted");
}

// The text after the ellipsis is not part of the link, although the link is
// there in the unelided text.
void tst_ElidingLabel::clickBesideElidedLinkActivatesNothing()
{
    std::unique_ptr<ElidingLabel> label(
        richLabel("<a href=\"https://link.example/\">" + QString(60, 'x') + "</a>"
                      + " plain words at the end",
                  Qt::ElideLeft));
    label->resize(120, label->sizeHint().height());
    label->show();
    QVERIFY(QTest::qWaitForWindowExposed(label.get()));
    QSignalSpy activated(label.get(), &QLabel::linkActivated);

    click(label.get(), QPoint(label->width() / 2, label->height() / 2));

    QCOMPARE(activated.count(), 0);
}

void tst_ElidingLabel::clickOnElidedLinkActivatesTheShownLink()
{
    std::unique_ptr<ElidingLabel> label(
        richLabel("<a href=\"https://first.example/\">first</a> " + QString(60, 'y')
                      + " <a href=\"https://last.example/\">last</a>",
                  Qt::ElideMiddle));
    label->resize(150, label->sizeHint().height());
    label->show();
    QVERIFY(QTest::qWaitForWindowExposed(label.get()));
    QSignalSpy activated(label.get(), &QLabel::linkActivated);

    // Eliding in the middle can leave a gap at the end, so find where the
    // shown text ends.
    const int textEnd = paintedTextEnd(label.get());
    QVERIFY(textEnd > 0);

    click(label.get(), QPoint(textEnd - 1, label->height() / 2));
    click(label.get(), QPoint(3, label->height() / 2));

    QCOMPARE(activated.count(), 2);
    QCOMPARE(activated.at(0).at(0).toString(), QString("https://last.example/"));
    QCOMPARE(activated.at(1).at(0).toString(), QString("https://first.example/"));
}

void tst_ElidingLabel::hoveringLinkShowsItsTarget_data()
{
    QTest::addColumn<int>("width");
    QTest::newRow("elided") << 100;
    QTest::newRow("unelided") << 1000;
}

void tst_ElidingLabel::hoveringLinkShowsItsTarget()
{
    QFETCH(int, width);
    std::unique_ptr<ElidingLabel> label(
        richLabel("<a href=\"http://scam.example/steal\">https://bank.example/login</a>"));
    label->resize(width, label->sizeHint().height());
    label->show();
    QVERIFY(QTest::qWaitForWindowExposed(label.get()));

    QCOMPARE(hoverToolTip(label.get(), QPoint(5, label->height() / 2)),
             QString("http://scam.example/steal"));
}

void tst_ElidingLabel::hoveringLinkWithoutSchemeShowsNoTarget()
{
    std::unique_ptr<ElidingLabel> label(richLabel("<a href=\"settings\">Configure</a>"));
    label->resize(1000, label->sizeHint().height());
    label->show();
    QVERIFY(QTest::qWaitForWindowExposed(label.get()));

    QCOMPARE(hoverToolTip(label.get(), QPoint(5, label->height() / 2)), QString());
}

void tst_ElidingLabel::tabVisitsOnlyShownLinks()
{
    ThreeLinks links;
    links.window.show();
    links.window.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&links.window));
    links.label->setFocus(Qt::TabFocusReason);
    QSignalSpy activated(links.label, &QLabel::linkActivated);

    QStringList visited;
    for (int i = 0; i < 3 && QApplication::focusWidget() == links.label; ++i) {
        QTest::keyClick(links.label, Qt::Key_Tab);
        if (QApplication::focusWidget() != links.label)
            break;
        QTest::keyClick(links.label, Qt::Key_Return);
        QCOMPARE(activated.count(), 1);
        visited.append(activated.takeFirst().at(0).toString());
    }

    QCOMPARE(visited, QStringList({"https://first.example/", "https://last.example/"}));
    QCOMPARE(QApplication::focusWidget(), links.next);
}

void tst_ElidingLabel::focusedElidedLinkIsHighlighted()
{
    ThreeLinks links;
    links.window.show();
    links.window.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&links.window));
    links.label->setFocus(Qt::TabFocusReason);
    const QImage unfocused = links.label->grab().toImage();

    QTest::keyClick(links.label, Qt::Key_Tab);

    QCOMPARE(QApplication::focusWidget(), links.label);
    QVERIFY(links.label->grab().toImage() != unfocused);
}

void tst_ElidingLabel::focusIndicatorOfElidedLinkIsClosed_data()
{
    QTest::addColumn<int>("width");
    // Which widths make the elided text end right at the label's edge depends
    // on the font, so try them all in a range.
    for (int width = 140; width <= 200; ++width)
        QTest::addRow("%d", width) << width;
}

// The focus indicator is an outline around the link. Its right edge must be
// drawn as its left one, also when the elided link reaches the end of the label.
void tst_ElidingLabel::focusIndicatorOfElidedLinkIsClosed()
{
    QFETCH(int, width);
    const QString url = "https://example.com/authorize?code_challenge=0123456789abcdefghijklmnop";
    QWidget window;
    auto layout = new QVBoxLayout(&window);
    layout->setContentsMargins(0, 0, 0, 0);
    ElidingLabel *label = richLabel(QString("<a href=\"%1\">%1</a>").arg(url), Qt::ElideMiddle);
    layout->addWidget(label);
    layout->addWidget(new QLineEdit);
    window.resize(width, window.sizeHint().height());
    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    label->setFocus(Qt::TabFocusReason);
    const QImage unfocused = label->grab().toImage();
    QTest::keyClick(label, Qt::Key_Tab);
    const QImage focused = label->grab().toImage();

    // Rows changed by the focus indicator in each column.
    QList<int> changedRows;
    for (int x = 0; x < focused.width(); ++x) {
        int rows = 0;
        for (int y = 0; y < focused.height(); ++y)
            rows += focused.pixel(x, y) != unfocused.pixel(x, y);
        if (rows > 0)
            changedRows.append(rows);
    }
    QVERIFY(!changedRows.isEmpty());
    QVERIFY2(changedRows.last() * 2 >= changedRows.first(),
             qPrintable(QString("left edge: %1 rows, right edge: %2 rows")
                            .arg(changedRows.first())
                            .arg(changedRows.last())));
}

// The cut is where elidedText() cut, not at the first ellipsis in the text.
// Cut at the text's own ellipsis instead, the label would show the end of the
// text, and with it the link there.
void tst_ElidingLabel::ellipsisInTextIsNotWhereItIsCut()
{
    const QString plain = QString("Waiting\u2026 for the server to respond");
    std::unique_ptr<ElidingLabel> label(
        richLabel(QString("Waiting\u2026 for the server to <a href=\"https://link.example/\">"
                          "respond</a>")));
    label->resize(label->fontMetrics().horizontalAdvance(plain) * 2 / 3,
                  label->sizeHint().height());
    label->show();
    QVERIFY(QTest::qWaitForWindowExposed(label.get()));
    QSignalSpy activated(label.get(), &QLabel::linkActivated);

    const int textEnd = paintedTextEnd(label.get());
    QVERIFY(textEnd > 0);
    click(label.get(), QPoint(textEnd - 1, label->height() / 2));

    QCOMPARE(activated.count(), 0);
}

void tst_ElidingLabel::hoveredLinkIsForgottenWhenNoLongerElided()
{
    const QString linkText(40, 'x');
    std::unique_ptr<ElidingLabel> label(richLabel("<a href=\"https://link.example/\">" + linkText
                                                  + "</a> and plain words after it"));
    // Away from the mouse cursor: a resize under it sends the label a mouse
    // move of its own, which would let QLabel catch up.
    label->move(QCursor::pos() + QPoint(40, 40));
    label->resize(80, label->sizeHint().height());
    label->show();
    QVERIFY(QTest::qWaitForWindowExposed(label.get()));
    const int y = label->height() / 2;
    QMouseEvent overLink(QEvent::MouseMove, QPointF(10, y), QPointF(label->mapToGlobal(QPoint(10, y))),
                         Qt::NoButton, {}, {});
    QApplication::sendEvent(label.get(), &overLink);
    QCOMPARE(label->cursor().shape(), Qt::PointingHandCursor);

    label->resize(2000, label->height());
    label->grab(); // The repaint that follows the resize.
    const QPoint plainText(label->fontMetrics().horizontalAdvance(linkText + " and pl"), y);

    QCOMPARE(hoverToolTip(label.get(), plainText), QString());
    QVERIFY(label->cursor().shape() != Qt::PointingHandCursor);
}

// QLabel only reports a link when the mouse reaches it, so it must forget the
// link it hovered when the label takes over, or hovering it again reports nothing.
void tst_ElidingLabel::linkHoveredBeforeElidingIsHoveredAgainAfter()
{
    std::unique_ptr<ElidingLabel> label(richLabel("<a href=\"https://link.example/\">"
                                                  + QString(40, 'x') + "</a> and plain words"));
    label->move(QCursor::pos() + QPoint(40, 40));
    label->resize(2000, label->sizeHint().height());
    label->show();
    QVERIFY(QTest::qWaitForWindowExposed(label.get()));
    const QPoint overLink(10, label->height() / 2);
    QCOMPARE(hoverToolTip(label.get(), overLink), QString("https://link.example/"));
    QCOMPARE(label->cursor().shape(), Qt::PointingHandCursor);

    label->resize(80, label->height());
    label->grab();
    label->resize(2000, label->height());
    label->grab();

    QCOMPARE(hoverToolTip(label.get(), overLink), QString("https://link.example/"));
    QCOMPARE(label->cursor().shape(), Qt::PointingHandCursor);
}

void tst_ElidingLabel::urlToolTipClosesOverLinkWithoutScheme()
{
    std::unique_ptr<ElidingLabel> label(
        richLabel("Open <a href=\"https://docs.example/\">docs</a> or "
                  "<a href=\"settings\">configure</a>"));
    label->resize(1000, label->sizeHint().height());
    label->show();
    QVERIFY(QTest::qWaitForWindowExposed(label.get()));
    const QFontMetrics fm = label->fontMetrics();
    const int y = label->height() / 2;
    QCOMPARE(hoverToolTip(label.get(), QPoint(fm.horizontalAdvance("Open d"), y)),
             QString("https://docs.example/"));

    const QPoint configure(fm.horizontalAdvance("Open docs or con"), y);
    QMouseEvent move(QEvent::MouseMove, QPointF(configure), QPointF(label->mapToGlobal(configure)),
                     Qt::NoButton, {}, {});
    QApplication::sendEvent(label.get(), &move);
    QHelpEvent help(QEvent::ToolTip, configure, label->mapToGlobal(configure));
    QApplication::sendEvent(label.get(), &help);

    QTRY_VERIFY(!QToolTip::isVisible());
}

QTEST_MAIN(tst_ElidingLabel)

#include "tst_elidinglabel.moc"
