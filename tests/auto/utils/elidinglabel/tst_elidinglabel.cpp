// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/elidinglabel.h>

#include <QTest>
#include <QTextDocument>

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

QTEST_MAIN(tst_ElidingLabel)

#include "tst_elidinglabel.moc"
