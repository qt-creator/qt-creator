// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <solutions/terminal/surfaceintegration.h>
#include <solutions/terminal/terminalsurface.h>

#include <QTest>

#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStringList>

#include <limits>
#include <memory>
#include <variant>

using namespace TerminalSolution;

class ClipboardRecorder : public SurfaceIntegration
{
public:
    QList<std::pair<QByteArray, ClipboardTargets>> writes;

    void onOsc(int cmd, std::string_view str, bool initial, bool final) override
    {
        Q_UNUSED(cmd)
        Q_UNUSED(str)
        Q_UNUSED(initial)
        Q_UNUSED(final)
    }

    void onSetClipboard(const QByteArray &text, ClipboardTargets targets) override
    {
        writes.append({text, targets});
    }
};

class tst_TerminalSurface : public QObject
{
    Q_OBJECT

private:
    static constexpr int notFound = std::numeric_limits<int>::min();

    ClipboardRecorder m_recorder;
    std::unique_ptr<TerminalSurface> m_surface;
    QByteArray m_written;

    QString textAt(int y) const
    {
        QString result;
        for (int x = 0; x < m_surface->liveSize().width(); ++x)
            result += m_surface->fetchCell(x, y).text;
        return result.trimmed();
    }

private slots:
    void initSurface(QSize size)
    {
        m_recorder.writes.clear();
        m_written.clear();
        m_surface = std::make_unique<TerminalSurface>(size);
        m_surface->setWriteToPty([this](const QByteArray &data) {
            m_written += data;
            return qint64(data.size());
        });
        m_surface->setSurfaceIntegration(&m_recorder);
    }

    void init() { initSurface({80, 24}); }

    void cleanup() { m_surface.reset(); }

    QString surfaceText() const
    {
        QString out;
        const QSize full = m_surface->fullSize();
        for (int y = 0; y < full.height(); ++y)
            for (int x = 0; x < full.width(); ++x)
                out += m_surface->fetchCell(x, y).text;
        return out;
    }

    std::u32string surfaceChars() const
    {
        std::u32string out;
        const QSize full = m_surface->fullSize();
        for (int y = 0; y < full.height(); ++y)
            for (int x = 0; x < full.width(); ++x)
                if (const char32_t c = m_surface->fetchCharAt(x, y))
                    out += c;
        return out;
    }

    QString rowText(int y) const
    {
        QString row;
        for (int x = 0; x < m_surface->fullSize().width(); ++x)
            row += m_surface->fetchCell(x, y).text;
        return row;
    }

    QString write(const QStringList &lines)
    {
        QString expected;
        for (const QString &line : lines) {
            m_surface->dataFromPty(line.toUtf8() + "\r\n");
            expected += line;
        }
        return expected;
    }

    void resizeTo(QSize size) { m_surface->resize(size); }

    void aCharacterSplitAcrossTwoWritesIsStillOneCharacter()
    {
        const QByteArray text = QString::fromUcs4(U"ab你好cd", 6).toUtf8();

        for (int cut = 1; cut < text.size(); ++cut) {
            initSurface({20, 4});
            m_surface->dataFromPty(text.left(cut));
            m_surface->dataFromPty(text.mid(cut));

            QCOMPARE(surfaceChars(), std::u32string(U"ab你好cd"));
        }
    }

    void plainTextHasNoHyperlink()
    {
        m_surface->dataFromPty("This is a link");

        QCOMPARE(textAt(0), QString("This is a link"));
        QVERIFY(!m_surface->hyperlinkAt({0, 0}));
        QVERIFY(!m_surface->hyperlinkAt({9, 0}));
    }

    void hyperlink_data()
    {
        QTest::addColumn<QByteArray>("terminator");

        QTest::newRow("ST") << QByteArray("\x1b\\");
        QTest::newRow("BEL") << QByteArray("\x07");
    }

    void hyperlink()
    {
        QFETCH(QByteArray, terminator);

        m_surface->dataFromPty(QByteArray("\x1b]8;;http://example.com") + terminator
                               + QByteArray("This is a link") + QByteArray("\x1b]8;;")
                               + terminator);

        QCOMPARE(textAt(0), QString("This is a link"));

        for (int x = 0; x < 14; ++x) {
            const std::optional<Hyperlink> hyperlink = m_surface->hyperlinkAt({x, 0});
            QVERIFY2(hyperlink.has_value(), qPrintable(QString("no hyperlink at %1").arg(x)));
            QCOMPARE(hyperlink->url, QString("http://example.com"));
            QCOMPARE(hyperlink->start, 0);
            QCOMPARE(hyperlink->end, 14);
        }

        QVERIFY(!m_surface->hyperlinkAt({14, 0}));
    }

    void hyperlinkArrivingInFragments()
    {
        const QByteArray data = "\x1b]8;;http://example.com\x1b\\link\x1b]8;;\x1b\\";

        for (qsizetype split = 1; split < data.size(); ++split) {
            init();
            m_surface->dataFromPty(data.left(split));
            m_surface->dataFromPty(data.mid(split));

            const std::optional<Hyperlink> hyperlink = m_surface->hyperlinkAt({0, 0});
            QVERIFY2(hyperlink.has_value(), qPrintable(QString("split at %1").arg(split)));
            QCOMPARE(hyperlink->url, QString("http://example.com"));
            QCOMPARE(hyperlink->start, 0);
            QCOMPARE(hyperlink->end, 4);
        }
    }

    void adjacentHyperlinksWithDifferentIdsAreDistinct()
    {
        m_surface->dataFromPty("\x1b]8;id=1;http://example.com\x1b\\A"
                               "\x1b]8;id=2;http://example.com\x1b\\B"
                               "\x1b]8;;\x1b\\");

        QCOMPARE(textAt(0), QString("AB"));

        const std::optional<Hyperlink> first = m_surface->hyperlinkAt({0, 0});
        const std::optional<Hyperlink> second = m_surface->hyperlinkAt({1, 0});

        QVERIFY(first);
        QVERIFY(second);
        QCOMPARE(first->url, QString("http://example.com"));
        QCOMPARE(second->url, QString("http://example.com"));
        QCOMPARE(first->end, 1);
        QCOMPARE(second->start, 1);
    }

    void hyperlinkIsFoundInScrollback()
    {
        m_surface->dataFromPty("\x1b]8;;http://example.com\x1b\\link\x1b]8;;\x1b\\\r\n");
        for (int i = 0; i < 40; ++i)
            m_surface->dataFromPty("filler\r\n");

        QVERIFY(m_surface->fullSize().height() > m_surface->liveSize().height());
        QCOMPARE(textAt(0), QString("link"));

        const std::optional<Hyperlink> hyperlink = m_surface->hyperlinkAt({0, 0});
        QVERIFY(hyperlink);
        QCOMPARE(hyperlink->url, QString("http://example.com"));
    }

    void hyperlinkSurvivesResize()
    {
        m_surface->dataFromPty("\x1b]8;;http://example.com\x1b\\link\x1b]8;;\x1b\\\r\n");
        for (int i = 0; i < 40; ++i)
            m_surface->dataFromPty("filler\r\n");

        QVERIFY(m_surface->fullSize().height() > m_surface->liveSize().height());

        // Growing the screen pops the lines back out of the scrollback.
        m_surface->resize({80, 60});
        QCOMPARE(m_surface->fullSize().height(), 60);

        QCOMPARE(textAt(0), QString("link"));

        const std::optional<Hyperlink> hyperlink = m_surface->hyperlinkAt({0, 0});
        QVERIFY(hyperlink);
        QCOMPARE(hyperlink->url, QString("http://example.com"));
    }

    void anOpenHyperlinkDoesNotCoverScrolledInBlankCells()
    {
        // The hyperlink is never closed, so every cell written from now on is part of it,
        // but the blank cells of the lines scrolled in below must not be.
        m_surface->dataFromPty("\x1b]8;;http://example.com\x1b\\");
        for (int i = 0; i < 40; ++i)
            m_surface->dataFromPty("X\r\n");

        const int lastRow = m_surface->fullSize().height() - 1;
        QCOMPARE(textAt(lastRow - 1), QString("X"));

        QVERIFY(m_surface->hyperlinkAt({0, lastRow - 1}));
        QVERIFY(!m_surface->hyperlinkAt({1, lastRow - 1}));
        QVERIFY(!m_surface->hyperlinkAt({0, lastRow}));
    }

    void anOpenHyperlinkDoesNotCoverCellsAddedByAResize()
    {
        m_surface->dataFromPty("\x1b]8;;http://example.com\x1b\\X");
        m_surface->resize({100, 30});

        QVERIFY(m_surface->hyperlinkAt({0, 0}));
        QVERIFY(!m_surface->hyperlinkAt({90, 0}));
        QVERIFY(!m_surface->hyperlinkAt({0, 27}));
    }

    void theExtentOfAnUnclosedHyperlinkIsBounded()
    {
        const QSize live = m_surface->liveSize();
        const int screenful = live.width() * live.height();

        m_surface->dataFromPty(QByteArray("\x1b]8;;http://example.com\x1b\\")
                               + QByteArray(3 * screenful, 'x'));

        const std::optional<Hyperlink> hyperlink
            = m_surface->hyperlinkAt({0, m_surface->fullSize().height() - 2});
        QVERIFY(hyperlink);
        QCOMPARE(hyperlink->url, QString("http://example.com"));
        QVERIFY2(hyperlink->end - hyperlink->start <= 2 * screenful + 1,
                 qPrintable(QString("extent %1").arg(hyperlink->end - hyperlink->start)));
    }

    void resetClosesTheHyperlink()
    {
        m_surface->dataFromPty("\x1b]8;;http://example.com\x1b\\"
                               "\x1b" "c"
                               "A");

        QCOMPARE(textAt(0), QString("A"));
        QVERIFY(!m_surface->hyperlinkAt({0, 0}));
    }

    void resizePreservesContent_data()
    {
        QTest::addColumn<QList<QSize>>("sizes");

        QTest::newRow("narrow") << QList<QSize>{{10, 6}};
        QTest::newRow("widen") << QList<QSize>{{40, 6}};
        QTest::newRow("narrow, widen") << QList<QSize>{{10, 6}, {40, 6}};
        QTest::newRow("widen, narrow") << QList<QSize>{{40, 6}, {10, 6}};
        QTest::newRow("one column at a time")
            << QList<QSize>{{19, 6}, {18, 6}, {17, 6}, {16, 6}, {17, 6}, {18, 6}, {19, 6}, {20, 6}};
        QTest::newRow("very narrow and back") << QList<QSize>{{3, 6}, {20, 6}};
        QTest::newRow("shorter") << QList<QSize>{{20, 3}};
        QTest::newRow("taller") << QList<QSize>{{20, 20}};
        QTest::newRow("narrower and taller") << QList<QSize>{{7, 24}};
        QTest::newRow("wider and shorter") << QList<QSize>{{60, 2}};
        QTest::newRow("drag a corner")
            << QList<QSize>{{19, 7}, {17, 9}, {14, 12}, {11, 15}, {30, 4}, {20, 6}};
    }

    void resizePreservesContent()
    {
        QFETCH(QList<QSize>, sizes);

        initSurface({20, 6});

        const QString expected = write({QString(30, 'A'),
                                        "bbb",
                                        QString(45, 'C'),
                                        "d0",
                                        QString(23, 'E'),
                                        "f1"});
        QCOMPARE(surfaceText(), expected);

        for (const QSize &size : sizes) {
            resizeTo(size);
            QVERIFY2(surfaceText() == expected,
                     qPrintable(QString("after resize to %1x%2:\n  got      %3\n  expected %4")
                                    .arg(size.width())
                                    .arg(size.height())
                                    .arg(surfaceText(), expected)));
        }
    }

    void narrowingRewrapsTheScrollback()
    {
        initSurface({20, 6});

        write({QString(30, 'A'), "bbb", "c0", "c1", "c2", "c3"});
        QCOMPARE(rowText(0), QString(20, 'A'));

        resizeTo({10, 6});

        QCOMPARE(rowText(0), QString(10, 'A'));
        QCOMPARE(rowText(1), QString(10, 'A'));
        QCOMPARE(rowText(2), QString(10, 'A'));
        QCOMPARE(rowText(3), QString("bbb"));
    }

    void wideningRejoinsWrappedLines()
    {
        initSurface({20, 6});
        write({QString(30, 'A'), "bbb", "c0", "c1", "c2", "c3"});

        resizeTo({40, 6});

        QCOMPARE(rowText(0), QString(30, 'A'));
        QCOMPARE(rowText(1), QString("bbb"));
    }

    void resizeWithALineStraddlingTheScrollbackBoundary()
    {
        initSurface({20, 6});

        const QString expected = write({QString(45, 'C'), "d0", "d1", "d2", "d3"});

        QCOMPARE(m_surface->fullSize().height(), 8);
        QCOMPARE(rowText(1), QString(20, 'C'));
        QCOMPARE(rowText(2), QString(5, 'C'));

        resizeTo({40, 6});
        QCOMPARE(surfaceText(), expected);

        resizeTo({10, 6});
        QCOMPARE(surfaceText(), expected);
    }

    void aMultiRowScrollKeepsTheWrapInformation()
    {
        initSurface({20, 6});
        const QString expected = write({QString(30, 'X'), "y"});

        m_surface->dataFromPty("\x1b[3S");
        QCOMPARE(surfaceText(), expected);

        resizeTo({40, 6});

        QCOMPARE(surfaceText(), expected);
        QCOMPARE(rowText(0), QString(30, 'X'));
    }

    void growingTheHeightKeepsPoppedLinesJoined()
    {
        initSurface({20, 6});
        const QString expected = write({QString(45, 'C'), "d0", "d1", "d2", "d3"});

        resizeTo({20, 12});
        QCOMPARE(surfaceText(), expected);
        QCOMPARE(m_surface->fullSize().height(), 12);

        resizeTo({40, 12});
        QCOMPARE(surfaceText(), expected);
        QCOMPARE(rowText(0), QString(40, 'C'));
        QCOMPARE(rowText(1), QString(5, 'C'));
    }

    void thePaddingOfAWrappedRowIsNotKept()
    {
        initSurface({20, 6});

        const QChar wide(0x4f60);
        m_surface->dataFromPty(QString("x" + QString(10, wide)).toUtf8() + "\r\n");
        write({"a0", "a1", "a2", "a3", "a4"});
        QCOMPARE(m_surface->fetchCell(19, 0).text, QString());

        resizeTo({40, 6});

        QCOMPARE(m_surface->fetchCell(19, 0).text, QString(wide));
        QCOMPARE(m_surface->cellWidthAt(19, 0), 2);
    }

    void randomResizeSequencesPreserveContent()
    {
        QRandomGenerator rng(42);

        for (int iteration = 0; iteration < 60; ++iteration) {
            initSurface({2 + int(rng.bounded(60)), 2 + int(rng.bounded(20))});

            std::u32string expected;
            const int lines = 1 + rng.bounded(6);
            for (int l = 0; l < lines; ++l) {
                const QString line(rng.bounded(80), QChar('a' + l));
                m_surface->dataFromPty(line.toUtf8() + "\r\n");
                for (const QChar c : line)
                    expected += c.unicode();
            }
            QCOMPARE(surfaceChars(), expected);

            for (int step = 0; step < 5; ++step) {
                const QSize size{2 + int(rng.bounded(60)), 2 + int(rng.bounded(20))};
                m_surface->resize(size);

                QVERIFY2(surfaceChars() == expected,
                         qPrintable(QString("iteration %1 step %2, resized to %3x%4: %5 chars "
                                            "instead of %6")
                                        .arg(iteration)
                                        .arg(step)
                                        .arg(size.width())
                                        .arg(size.height())
                                        .arg(surfaceChars().size())
                                        .arg(expected.size())));
            }
        }
    }

    int rowOfThePrompt() const
    {
        for (int y = 0; y < m_surface->fullSize().height(); ++y) {
            if (rowText(y).startsWith("PP>"))
                return y;
        }
        return notFound;
    }

    QByteArray promptWithInput(int inputLength) const
    {
        QByteArray out = QByteArray("PP> ") + QByteArray(inputLength, 'e');
        if ((out.size() % m_surface->liveSize().width()) == 0)
            out += " \r\x1b[K";
        return out;
    }

    static QByteArray repaintFrom(int rowsAbove)
    {
        QByteArray out;
        if (rowsAbove > 0)
            out = "\x1b[" + QByteArray::number(rowsAbove) + "A";
        return out + "\r\x1b[J";
    }

    void aShellsRepaintAfterAResizeLandsOnItsPrompt_data()
    {
        QTest::addColumn<int>("inputLength");
        QTest::addColumn<QList<QSize>>("sizes");

        QTest::newRow("wrapped input, narrower") << 60 << QList<QSize>{{30, 12}};
        QTest::newRow("wrapped input, wider") << 60 << QList<QSize>{{70, 12}};
        QTest::newRow("input filling a row, narrower") << 36 << QList<QSize>{{15, 12}};
        QTest::newRow("input filling a row, wider") << 36 << QList<QSize>{{55, 12}};
        QTest::newRow("input over three rows") << 100 << QList<QSize>{{25, 12}};
        QTest::newRow("one column at a time")
            << 60 << QList<QSize>{{36, 12}, {34, 12}, {32, 12}, {30, 12}, {32, 12}, {34, 12}};
        QTest::newRow("drag a corner") << 60 << QList<QSize>{{31, 10}, {60, 16}, {24, 12}};
    }

    void aShellsRepaintAfterAResizeLandsOnItsPrompt()
    {
        QFETCH(int, inputLength);
        QFETCH(QList<QSize>, sizes);

        initSurface({40, 12});
        const QString history = write({"history one", "history two"});
        m_surface->dataFromPty(promptWithInput(inputLength));

        const QString expected = history + "PP> " + QString(inputLength, 'e');
        QCOMPARE(surfaceText(), expected);

        for (const QSize &size : sizes) {
            QVERIFY(rowOfThePrompt() != notFound);
            const int rowsBelowThePrompt = m_surface->cursor().position.y() - rowOfThePrompt();
            QVERIFY(rowsBelowThePrompt >= 0);

            resizeTo(size);
            m_surface->dataFromPty(repaintFrom(rowsBelowThePrompt) + promptWithInput(inputLength));

            QVERIFY2(surfaceText() == expected,
                     qPrintable(QString("after resize to %1x%2 and a repaint from %3 rows "
                                        "above:\n  got      %4\n  expected %5")
                                    .arg(size.width())
                                    .arg(size.height())
                                    .arg(rowsBelowThePrompt)
                                    .arg(surfaceText(), expected)));
        }
    }

    void aResizeKeepsTheCursorWhereTheApplicationLeftIt_data()
    {
        QTest::addColumn<int>("inputLength");
        QTest::addColumn<QList<QSize>>("sizes");

        QTest::newRow("one row, narrower") << 14 << QList<QSize>{{10, 6}};
        QTest::newRow("one row, wider") << 14 << QList<QSize>{{40, 6}};
        QTest::newRow("short input") << 2 << QList<QSize>{{11, 6}, {30, 6}};
        QTest::newRow("wrapped input") << 31 << QList<QSize>{{13, 6}, {29, 6}};
        QTest::newRow("very narrow and back") << 14 << QList<QSize>{{3, 6}, {20, 6}};
        QTest::newRow("drag one column at a time")
            << 14
            << QList<QSize>{{19, 6}, {18, 6}, {17, 6}, {16, 6}, {17, 6}, {18, 6}, {19, 6}, {20, 6}};
    }

    void aResizeKeepsTheCursorWhereTheApplicationLeftIt()
    {
        QFETCH(int, inputLength);
        QFETCH(QList<QSize>, sizes);

        const int width = 20;
        initSurface({width, 6});
        write({"history one", "history two"});
        m_surface->dataFromPty(promptWithInput(inputLength));

        const int written = 4 + inputLength;
        int rowsBelowThePrompt = written / width;
        int column = written % width;
        if (column == 0) {
            --rowsBelowThePrompt;
            column = width;
        }

        for (const QSize &size : sizes) {
            resizeTo(size);
            column = qMin(column, size.width());
            QVERIFY(rowOfThePrompt() != notFound);

            const QPoint cursor = m_surface->cursor().position;
            const QPoint wanted{qMin(column, size.width() - 1),
                                rowOfThePrompt() + rowsBelowThePrompt};
            QVERIFY2(cursor == wanted,
                     qPrintable(QString("after resizing to %1x%2 the cursor is at %3,%4 instead "
                                        "of %5,%6")
                                    .arg(size.width())
                                    .arg(size.height())
                                    .arg(cursor.x())
                                    .arg(cursor.y())
                                    .arg(wanted.x())
                                    .arg(wanted.y())));
        }
    }

    void aCursorParkedOnACharacterIsNotTakenForADeferredWrap()
    {
        initSurface({20, 6});
        write({"history one", "history two"});
        m_surface->dataFromPty(QByteArray(20, 'x') + "\r" + QByteArray(19, 'x'));

        const int row = m_surface->cursor().position.y();
        int column = 19;
        QCOMPARE(m_surface->cursor().position, QPoint(column, row));

        for (const QSize &size : {QSize{19, 6}, {18, 6}, {17, 6}, {18, 6}, {19, 6}, {20, 6}}) {
            resizeTo(size);
            column = qMin(column, size.width());

            QCOMPARE(m_surface->cursor().position, QPoint(qMin(column, size.width() - 1), row));
        }
    }

    void aResizeKeepsTheTextBelowAParkedCursor()
    {
        initSurface({100, 10});

        const QString written(500, 'a');
        m_surface->dataFromPty(written.toUtf8());
        QCOMPARE(surfaceText(), written);

        for (const QSize &size : {QSize{20, 10}, {1, 6}, {100, 10}}) {
            resizeTo(size);
            QVERIFY2(surfaceText() == written,
                     qPrintable(QString("after resize to %1x%2, %3 of %4 characters are left")
                                    .arg(size.width())
                                    .arg(size.height())
                                    .arg(surfaceText().size())
                                    .arg(written.size())));
        }
    }

    void aResizeKeepsBlankLinesAboveAParkedCursor()
    {
        initSurface({20, 6});
        write({"", "", "one", "two"});
        m_surface->dataFromPty(promptWithInput(56));

        resizeTo({60, 6});

        QCOMPARE(rowText(0), QString());
        QCOMPARE(rowText(1), QString());
        QCOMPARE(rowText(2), QString("one"));
        QCOMPARE(rowText(3), QString("two"));
        QCOMPARE(rowText(4), QString("PP> ") + QString(56, 'e'));
    }

    void aWriteAfterANarrowingContinuesWhereTheWriterLeftOff()
    {
        initSurface({20, 6});

        const QString filled = QString(19, '.') + "Z";
        m_surface->dataFromPty(filled.toUtf8());

        resizeTo({10, 6});
        m_surface->dataFromPty("Y");

        QCOMPARE(rowText(0), QString(10, '.'));
        QCOMPARE(rowText(1), QString("Y") + QString(8, '.') + "Z");
    }

    void aResizeDoesNotLeaveTheCursorInsideAWideCharacter()
    {
        initSurface({20, 6});

        const QChar wide(0x4f60);
        m_surface->dataFromPty(QString(11, 'a').toUtf8() + QString(5, wide).toUtf8() + "b");

        resizeTo({14, 6});

        const QPoint cursor = m_surface->cursor().position;
        QVERIFY2(cursor.x() == 0 || m_surface->cellWidthAt(cursor.x() - 1, cursor.y()) == 1,
                 qPrintable(QString("the cursor at %1,%2 is the second half of the character at "
                                    "%3,%2")
                                .arg(cursor.x())
                                .arg(cursor.y())
                                .arg(cursor.x() - 1)));
    }

    void aWriteAfterAResizeDoesNotOverwriteTheLastCharacter()
    {
        initSurface({20, 6});

        const QString filled = QString(19, '.') + "Z";
        m_surface->dataFromPty(filled.toUtf8());
        QCOMPARE(surfaceText(), filled);

        resizeTo({40, 6});
        m_surface->dataFromPty("Y");

        QCOMPARE(surfaceText(), filled + "Y");
        QCOMPARE(rowText(0), filled + "Y");
    }

    int interiorHole(int y) const
    {
        const int width = m_surface->fullSize().width();
        int lastWritten = -1;
        for (int x = 0; x < width; ++x) {
            if (!m_surface->fetchCell(x, y).text.isEmpty())
                lastWritten = x;
        }
        for (int x = 0; x < lastWritten; ++x) {
            if (m_surface->fetchCell(x, y).text.isEmpty())
                return x;
        }
        return -1;
    }

    void resizingNeverInsertsBlanksIntoTheText()
    {
        initSurface({80, 10});

        for (int i = 0; i < 30; ++i) {
            m_surface->dataFromPty(
                QString("-rw-r--r--  1 user  staff  %1 Mar 20 02:38 file%2%3")
                    .arg(100 + i * 37)
                    .arg(i)
                    .arg(QString(i % 7 == 0 ? 40 : 3, 'x'))
                    .toUtf8()
                + "\r\n");
        }

        QRandomGenerator rng(7);
        for (int step = 0; step < 30; ++step) {
            const QSize size{20 + int(rng.bounded(70)), 8 + int(rng.bounded(20))};
            m_surface->resize(size);

            for (int y = 0; y < m_surface->fullSize().height(); ++y) {
                const int hole = interiorHole(y);
                QVERIFY2(hole == -1,
                         qPrintable(QString("step %1, resized to %2x%3: row %4 has a gap at "
                                            "column %5:\n  |%6|")
                                        .arg(step)
                                        .arg(size.width())
                                        .arg(size.height())
                                        .arg(y)
                                        .arg(hole)
                                        .arg(rowText(y))));
            }
        }
    }

    QString rowLayout(int y) const
    {
        QString row;
        for (int x = 0; x < m_surface->fullSize().width(); ++x) {
            const QString text = m_surface->fetchCell(x, y).text;
            row += text.isEmpty() ? QString("~") : text;
        }
        return row;
    }

    void theLayoutDoesNotDependOnTheResizeHistory()
    {
        QByteArray output;
        for (int i = 0; i < 30; ++i) {
            output += QString("-rw-r--r--    1 user  staff  %1 Mar 20 02:38 file%2%3")
                          .arg(100 + i * 37)
                          .arg(i)
                          .arg(QString(i % 7 == 0 ? 40 : 3, 'x'))
                          .toUtf8()
                      + "\r\n";
        }

        const auto layoutAt = [&](QSize size, const QList<QSize> &path) {
            initSurface(path.isEmpty() ? size : path.first());
            m_surface->dataFromPty(output);
            for (const QSize &step : path)
                m_surface->resize(step);
            m_surface->resize(size);

            QStringList rows;
            for (int y = 0; y < m_surface->fullSize().height(); ++y)
                rows += rowLayout(y);
            while (!rows.isEmpty() && !rows.last().contains(QRegularExpression("[^~]")))
                rows.removeLast();
            return rows;
        };

        QRandomGenerator rng(7);
        for (int iteration = 0; iteration < 40; ++iteration) {
            const QSize target{20 + int(rng.bounded(70)), 8 + int(rng.bounded(20))};

            QList<QSize> path;
            for (int i = 0; i < 6; ++i)
                path.append({20 + int(rng.bounded(70)), 8 + int(rng.bounded(20))});

            const QStringList direct = layoutAt(target, {});
            const QStringList viaPath = layoutAt(target, path);

            QString pathText;
            for (const QSize &step : path)
                pathText += QString(" %1x%2").arg(step.width()).arg(step.height());

            QString detail;
            for (int i = 0; i < qMax(direct.size(), viaPath.size()) && detail.size() < 600; ++i) {
                const QString a = i < direct.size() ? direct[i] : QString("<none>");
                const QString b = i < viaPath.size() ? viaPath[i] : QString("<none>");
                if (a != b)
                    detail += QString("row %1\n  written at the width |%2|\n  resized to it   |%3|\n")
                                  .arg(i).arg(a, b);
            }

            QVERIFY2(direct == viaPath,
                     qPrintable(QString("iteration %1, target %2x%3 reached through%4\n%5")
                                    .arg(iteration).arg(target.width()).arg(target.height())
                                    .arg(pathText, detail)));
        }
    }

    void rewrappedRowsKeepTheBackgroundOfUntouchedCells()
    {
        initSurface({40, 4});
        for (int i = 0; i < 10; ++i)
            m_surface->dataFromPty(QString("line%1").arg(i).toUtf8() + "\r\n");

        const auto background = [this](int x, int y) {
            return m_surface->fetchCell(x, y).backgroundColor;
        };
        const int screenRow = m_surface->fullSize().height() - 1;
        const std::variant<int, QColor> untouched = background(30, screenRow);

        m_surface->resize({30, 4});

        QVERIFY(m_surface->fullSize().height() > m_surface->liveSize().height());
        for (int y = 0; y < m_surface->fullSize().height(); ++y) {
            QVERIFY2(background(25, y) == untouched,
                     qPrintable(QString("row %1 column 25 does not have the background of an "
                                        "untouched cell")
                                    .arg(y)));
        }
    }

    void aGapInsideALineSurvivesRewrapping()
    {
        initSurface({40, 6});

        m_surface->dataFromPty("left\x1b[26Cright\r\n");
        write({"a0", "a1", "a2", "a3", "a4"});

        const auto trimmed = [this] {
            QString row = rowLayout(0);
            while (row.endsWith('~'))
                row.chop(1);
            return row;
        };
        const QString expected = QString("left") + QString(26, '~') + "right";
        QCOMPARE(trimmed(), expected);

        resizeTo({20, 6});
        resizeTo({40, 6});

        QCOMPARE(trimmed(), expected);
    }

    void blankLinesArePreserved()
    {
        initSurface({20, 6});
        write({"one", "", "", "two", "e0", "e1", "e2", "e3", "e4"});

        QVERIFY(m_surface->fullSize().height() > m_surface->liveSize().height());

        resizeTo({10, 6});

        QCOMPARE(rowText(0), QString("one"));
        QCOMPARE(rowText(1), QString());
        QCOMPARE(rowText(2), QString());
        QCOMPARE(rowText(3), QString("two"));
    }

    void poppingARowFromTheScrollbackKeepsTheRestOfItsLine()
    {
        initSurface({20, 6});

        const QString expected = write({QString(45, 'C'), "d0", "d1", "d2", "d3", "d4"});
        QVERIFY(m_surface->fullSize().height() > m_surface->liveSize().height());

        m_surface->dataFromPty("\x1b[?1049h");
        resizeTo({20, 10});
        m_surface->dataFromPty("\x1b[?1049l");

        QCOMPARE(surfaceText(), expected);
    }

    void wideningInAltscreenKeepsTheScrollbackContent()
    {
        initSurface({20, 6});

        const QString expected = write({QString(45, 'C'), "d0", "d1", "d2", "d3", "d4"});
        QVERIFY(m_surface->fullSize().height() > m_surface->liveSize().height());

        m_surface->dataFromPty("\x1b[?1049h");
        resizeTo({40, 6});
        m_surface->dataFromPty("\x1b[?1049l");

        QCOMPARE(m_surface->liveSize().width(), 40);

        // Reading every cell is what the renderer does on the next repaint. The
        // scrollback holds logical lines that wrap when read, so widening has to
        // reach it too: otherwise the columns are bounded by the new width while
        // the row buffer is still built for the old one.
        QCOMPARE(surfaceText(), expected);
    }

    void aCellFilledWithCombiningMarksIsNotReadPastItsEnd()
    {
        initSurface({20, 4});

        // A base character plus five combining marks fills all six of the
        // cell's character slots, so no terminating zero is stored after them.
        m_surface->dataFromPty("A\xcc\x81\xcc\x81\xcc\x81\xcc\x81\xcc\x81");

        const TerminalCell cell = m_surface->fetchCell(0, 0);
        QCOMPARE(cell.text.size(), 6);
    }

    void aCombiningSequenceIsComposedAndItsNeighbourIsNot()
    {
        // A guard on how many character slots fetchCharAt reads, not a
        // reproduction of reading too many: with the slot count hard-coded at
        // six the written NUL still sits in front of the stale tail and blocks
        // composition, so this passes either way. It is here to hold the
        // composition behaviour still while the read is bounded.
        m_surface->dataFromPty(QString(QChar(0x0061)).toUtf8()      // a
                               + QString(QChar(0x0308)).toUtf8()    // combining diaeresis
                               + QString(QChar(0x00e9)).toUtf8());  // e-acute, precomposed

        QCOMPARE(m_surface->fetchCharAt(0, 0), char32_t(0x00e4)); // a-diaeresis, composed
        QCOMPARE(m_surface->fetchCharAt(1, 0), char32_t(0x00e9));

        // And in the other order, so neither read is being served by the
        // other one having run first.
        QCOMPARE(m_surface->fetchCharAt(1, 0), char32_t(0x00e9));
        QCOMPARE(m_surface->fetchCharAt(0, 0), char32_t(0x00e4));
    }

    void aBlankCellCarriesNoHyperlink()
    {
        // Enough short lines to push rows into the scrollback. Their unwritten
        // columns are padded with the blank cell, which is what this is about.
        for (int i = 0; i < 40; ++i)
            m_surface->dataFromPty("short\r\n");

        const int scrollbackRows = m_surface->fullSize().height()
                                   - m_surface->liveSize().height();
        QVERIFY(scrollbackRows > 0);

        // A closed hyperlink on the first cell of the *live* screen, written
        // without scrolling so it stays there. The blank used to be a copy of
        // that cell with only its text cleared, and it is re-read on demand,
        // so its uri reached every padded column of every scrolled-back row.
        m_surface->dataFromPty("\x1b[H\x1b]8;;http://example.com\x1b\\x\x1b]8;;\x1b\\");

        // Control: the cell it was written on does carry it, so a blank coming
        // back empty is not the uri table having gone missing.
        const std::optional<Hyperlink> onTheLink = m_surface->hyperlinkAt({0, scrollbackRows});
        QVERIFY(onTheLink.has_value());
        QCOMPARE(onTheLink->url, QString("http://example.com"));

        // The padded columns of a scrolled-back row were never written.
        for (int y = 0; y < qMin(scrollbackRows, 4); ++y) {
            const std::optional<Hyperlink> onABlank = m_surface->hyperlinkAt({10, y});
            QVERIFY2(!onABlank.has_value(),
                     qPrintable(QString("blank at row %1 carries a link").arg(y)));
        }
    }

    void anOscNumberIsNotAccumulatedWithoutABound()
    {
        initSurface({20, 4});

        // The OSC command number is accumulated into an int with no limit on
        // the digit count, so the value the dispatcher compares against is not
        // necessarily the value that was written. 4294967304 is 2^32 + 8, and 8
        // is the hyperlink handler.
        m_surface->dataFromPty("\x1b]4294967304;;https://wrapped.invalid/\x1b\\");
        m_surface->dataFromPty("X");

        QVERIFY2(!m_surface->hyperlinkAt({0, 0}).has_value(),
                 "an OSC number of 4294967304 was dispatched to the hyperlink handler");
    }

    void aCsiArgumentIsNotAccumulatedWithoutABound()
    {
        // CSI_ARG masks off the top bit, so a padded number used to arrive as
        // whatever it masks down to. 4294969300 is 2^32 + 2004, and 2004 is
        // bracketed paste - which the host reads to decide whether a paste
        // needs confirming.
        m_surface->dataFromPty("\x1b[?4294969300h");
        QVERIFY(!m_surface->isBracketedPasteEnabled());

        // The number it would have aliased to still works, so the check above
        // is not passing because the mode became unreachable.
        m_surface->dataFromPty("\x1b[?2004h");
        QVERIFY(m_surface->isBracketedPasteEnabled());
    }

    void aCsiArgumentListDoesNotRunPastItsEnd()
    {
        // Every ';' advanced the argument index with no bound, and the
        // arguments sit in a fixed-size array followed by the parser's own
        // callbacks pointer - so a sequence with enough of them wrote over
        // that pointer, and the next CSI dereferenced it.
        m_surface->dataFromPty("\x1b[" + QByteArray(64, ';') + "m");

        // Getting here at all is most of the point. A round trip through the
        // parser afterwards shows it still works rather than merely not
        // having crashed yet: SGR left the cursor at the origin.
        m_surface->dataFromPty("ok");
        QCOMPARE(textAt(0), QString("ok"));
    }

    void aWideCharacterIsNotSplitByRewrapping()
    {
        initSurface({20, 6});

        m_surface->dataFromPty(QString(3, QChar(0x4f60)).toUtf8() + "\r\n");
        write({"a0", "a1", "a2", "a3", "a4"});
        resizeTo({5, 6});

        QCOMPARE(m_surface->cellWidthAt(0, 0), 2);
        QCOMPARE(m_surface->cellWidthAt(2, 0), 2);
        QCOMPARE(m_surface->cellWidthAt(0, 1), 2);
        QCOMPARE(m_surface->fetchCell(0, 1).text, QString(QChar(0x4f60)));
    }

    void anOverlongUriIsIgnored()
    {
        m_surface->dataFromPty(QByteArray("\x1b]8;;http://example.com/") + QByteArray(8192, 'x')
                               + QByteArray("\x1b\\link\x1b]8;;\x1b\\"));

        QCOMPARE(textAt(0), QString("link"));
        QVERIFY(!m_surface->hyperlinkAt({0, 0}));
    }

    void aClipboardWriteReachesOnlyTheRequestedSelections()
    {
        m_surface->dataFromPty("\x1b]52;c;Y2xpcA==\x07");
        m_surface->dataFromPty("\x1b]52;p;cHJpbQ==\x07");
        m_surface->dataFromPty("\x1b]52;cp;Ym90aA==\x07");

        QCOMPARE(m_recorder.writes.size(), 3);

        QCOMPARE(m_recorder.writes.at(0).first, QByteArray("clip"));
        QCOMPARE(m_recorder.writes.at(0).second, ClipboardTargets(ClipboardTarget::Clipboard));

        QCOMPARE(m_recorder.writes.at(1).first, QByteArray("prim"));
        QCOMPARE(m_recorder.writes.at(1).second, ClipboardTargets(ClipboardTarget::Selection));

        QCOMPARE(m_recorder.writes.at(2).first, QByteArray("both"));
        QCOMPARE(m_recorder.writes.at(2).second,
                 ClipboardTarget::Clipboard | ClipboardTarget::Selection);
    }

    void aClipboardWriteNamingNoSelectionNamesThePrimaryOne()
    {
        // An empty parameter is the form xterm documents as the default, and
        // reads there as the primary selection. libvterm turns it into
        // SELECT together with cut buffer 0, which has no counterpart here.
        m_surface->dataFromPty("\x1b]52;;Y2xpcA==\x07");

        QCOMPARE(m_recorder.writes.size(), 1);
        QCOMPARE(m_recorder.writes.at(0).first, QByteArray("clip"));
        QCOMPARE(m_recorder.writes.at(0).second, ClipboardTargets(ClipboardTarget::Selection));
    }

    void aClipboardWriteNamingNothingQtHasIsIgnored()
    {
        m_surface->dataFromPty("\x1b]52;q;Y2xpcA==\x07"); // the secondary selection
        m_surface->dataFromPty("\x1b]52;0;Y2xpcA==\x07"); // cut buffer 0 on its own

        QVERIFY(m_recorder.writes.isEmpty());
    }

    void anOverlongClipboardWriteIsDiscardedWhole()
    {
        const QByteArray tooMuch(8 * 1024 * 1024 + 1, 'x');
        m_surface->dataFromPty("\x1b]52;c;" + tooMuch.toBase64() + "\x07");

        QVERIFY(m_recorder.writes.isEmpty());

        m_surface->dataFromPty("\x1b]52;c;Y2xpcA==\x07");

        QCOMPARE(m_recorder.writes.size(), 1);
        QCOMPARE(m_recorder.writes.first().first, QByteArray("clip"));
    }

    void bracketedPasteModeIsVisibleToTheHost()
    {
        QVERIFY(!m_surface->isBracketedPasteEnabled());

        m_surface->dataFromPty("\x1b[?2004h");
        QVERIFY(m_surface->isBracketedPasteEnabled());

        m_surface->dataFromPty("\x1b[?2004l");
        QVERIFY(!m_surface->isBracketedPasteEnabled());
    }

    // The introducer and the terminator around the data of a sixel image
    static QByteArray sixel(const QByteArray &data) { return "\x1bP0;0;0q" + data + "\x1b\\"; }

    // An image one pixel wide and `count` bands of six pixels tall
    static QByteArray bands(int count)
    {
        QByteArray data = "#0;2;100;0;0~";
        for (int i = 1; i < count; ++i)
            data += "-~";
        return sixel(data);
    }

    void anImageCoversTheCellsItNeeds()
    {
        m_surface->setCellSize({10, 20});

        // 25 pixels wide and 6 tall: three cells wide, one high
        m_surface->dataFromPty(sixel("#0;2;100;0;0!25~"));

        for (int x = 0; x < 3; ++x) {
            const quint32 tag = m_surface->fetchCell(x, 0).image;
            QVERIFY2(tag != 0, qPrintable(QString("no image in cell %1").arg(x)));
            QCOMPARE(ImageCell::column(tag), x);
            QCOMPARE(ImageCell::row(tag), 0);
        }

        QCOMPARE(m_surface->fetchCell(3, 0).image, 0u);
    }

    void anImageStartsAtTheCursorAndEndsOnAFreshLine()
    {
        m_surface->setCellSize({10, 20});

        m_surface->dataFromPty("ab" + sixel("#0;2;100;0;0!25~"));
        m_surface->dataFromPty("below");

        QCOMPARE(m_surface->fetchCell(1, 0).image, 0u);
        QVERIFY(m_surface->fetchCell(2, 0).image != 0);
        QCOMPARE(textAt(1), QString("below"));
    }

    void aLineBreakInTheDataDoesNotMoveTheImage()
    {
        m_surface->setCellSize({10, 20});

        // Some encoders wrap their output, and the line break moves the cursor
        m_surface->dataFromPty(sixel("#0;2;100;0;0!25~\r\n-!25~"));

        QVERIFY(m_surface->fetchCell(0, 0).image != 0);
        QCOMPARE(m_surface->cursor().position.y(), 1);
    }

    void anImageAfterAFullLineStartsOnTheNextOne()
    {
        initSurface({20, 5});
        m_surface->setCellSize({10, 20});

        // A line filled to its last column leaves the cursor in the phantom
        // column, where the next character would wrap
        m_surface->dataFromPty("\x1b[5;1H" + QByteArray(20, 'a'));
        m_surface->dataFromPty(sixel("#0;2;100;0;0!25~"));
        m_surface->dataFromPty("X");

        const int rows = m_surface->fullSize().height();
        QCOMPARE(rowText(rows - 3).trimmed(), QString(20, 'a'));
        QVERIFY(m_surface->fetchCell(0, rows - 2).image != 0);
        QCOMPARE(rowText(rows - 1).trimmed(), QString("X"));
    }

    void theCellsOfAnImageFindTheirPartOfIt()
    {
        m_surface->setCellSize({10, 20});

        m_surface->dataFromPty(sixel("#0;2;100;0;0!25~"));

        const quint32 tag = m_surface->fetchCell(1, 0).image;
        const std::optional<ImageTile> tile = m_surface->imageTile(tag);
        QVERIFY(tile);
        QCOMPARE(tile->image.size(), QSize(25, 6));
        QCOMPARE(tile->source, QRectF(10, 0, 10, 20));
    }

    void anImageTallerThanTheScreenScrollsIt()
    {
        initSurface({20, 5});
        m_surface->setCellSize({10, 6});

        m_surface->dataFromPty(bands(6));

        // Six rows of image and the row the cursor was left on, in five rows of
        // screen and two of scrollback
        QCOMPARE(m_surface->fullSize().height(), 7);

        const int id = ImageCell::id(m_surface->fetchCell(0, 0).image);
        QVERIFY(id != 0);
        for (int y = 0; y < 6; ++y) {
            const quint32 tag = m_surface->fetchCell(0, y).image;
            QCOMPARE(ImageCell::id(tag), id);
            QCOMPARE(ImageCell::row(tag), y);
        }
        QCOMPARE(m_surface->fetchCell(0, 6).image, 0u);
    }

    void theEmptySpaceAroundAnImageIsNotPartOfIt()
    {
        initSurface({20, 5});
        m_surface->setCellSize({10, 6});

        // Tall enough that the top row of the screen is a row of the image
        m_surface->dataFromPty(bands(8));

        QVERIFY(m_surface->fullSize().height() > m_surface->liveSize().height());
        QVERIFY(m_surface->fetchCell(0, 0).image != 0);
        QCOMPARE(m_surface->fetchCell(5, 0).image, 0u);
    }

    void anImageIdThatComesRoundAgainLeavesNoCellNamingIt()
    {
        initSurface({20, 5});
        m_surface->setCellSize({10, 20});

        const QByteArray image = sixel("#0;2;100;0;0!25~");
        m_surface->dataFromPty(image);

        const quint32 first = m_surface->fetchCell(0, 0).image;
        QVERIFY(first != 0);
        QVERIFY(m_surface->imageTile(first));

        // Use up the ids, so that the next image is given the one of the first
        for (int i = 0; i < ImageCell::maxId; ++i)
            m_surface->dataFromPty(image);

        // The id belongs to one of the new images now, so the cells of the
        // first one must not be naming it any more: they would show it.
        QCOMPARE(m_surface->fetchCell(0, 0).image, 0u);

        // What was drawn after the ids came round is still shown
        const int lastRow = m_surface->fullSize().height() - 2;
        QVERIFY(m_surface->imageTile(m_surface->fetchCell(0, lastRow).image));
    }

    void anImageIdComingRoundOnTheAltscreenClearsThePrimaryToo()
    {
        initSurface({20, 5});
        m_surface->setCellSize({10, 20});

        const QByteArray image = sixel("#0;2;100;0;0!25~");
        m_surface->dataFromPty(image);
        QVERIFY(m_surface->fetchCell(0, 0).image != 0);

        // The primary screen keeps its cells while the altscreen is up, so the
        // ids coming round there have to reach them as well
        m_surface->dataFromPty("\x1b[?1049h");
        for (int i = 0; i < ImageCell::maxId; ++i)
            m_surface->dataFromPty(image);
        m_surface->dataFromPty("\x1b[?1049l");

        QCOMPARE(m_surface->fetchCell(0, 0).image, 0u);
    }

    void textWrittenOverAnImageCoversIt()
    {
        m_surface->setCellSize({10, 20});

        m_surface->dataFromPty(sixel("#0;2;100;0;0!25~"));
        m_surface->dataFromPty("\x1b[H"
                               "x");

        QCOMPARE(m_surface->fetchCell(0, 0).image, 0u);
        QVERIFY(m_surface->fetchCell(1, 0).image != 0);
    }

    void erasingTheScreenRemovesTheImage()
    {
        m_surface->setCellSize({10, 20});

        m_surface->dataFromPty(sixel("#0;2;100;0;0!25~"));
        m_surface->dataFromPty("\x1b[2J");

        QCOMPARE(m_surface->fetchCell(0, 0).image, 0u);
    }

    void anImageInTheScrollbackSurvivesARewrap()
    {
        initSurface({20, 5});
        m_surface->setCellSize({10, 20});

        m_surface->dataFromPty(sixel("#0;2;100;0;0!25~"));
        for (int i = 0; i < 10; ++i)
            m_surface->dataFromPty("filler\r\n");

        QVERIFY(m_surface->fullSize().height() > m_surface->liveSize().height());
        const quint32 before = m_surface->fetchCell(2, 0).image;
        QVERIFY(before != 0);

        resizeTo({10, 5});

        QCOMPARE(m_surface->fetchCell(2, 0).image, before);
        QVERIFY(m_surface->imageTile(before));
    }

    void clearingTheTerminalForgetsTheImages()
    {
        m_surface->setCellSize({10, 20});

        m_surface->dataFromPty(sixel("#0;2;100;0;0!25~"));
        const quint32 tag = m_surface->fetchCell(0, 0).image;
        QVERIFY(m_surface->imageTile(tag));

        m_surface->clearAll();

        QVERIFY(!m_surface->imageTile(tag));
    }

    void aStringThatOnlyLooksLikeASixelIsNotOne()
    {
        m_surface->setCellSize({10, 20});

        // XTGETTCAP, which ends in a q as well
        m_surface->dataFromPty("\x1bP+q544e\x1b\\"
                               "text");

        QCOMPARE(textAt(0), QString("text"));
        QCOMPARE(m_surface->fetchCell(0, 0).image, 0u);
    }

    void theDeviceAttributesReportSixelSupport()
    {
        m_surface->dataFromPty("\x1b[c");

        QTRY_COMPARE(m_written, QByteArray("\x1b[?1;2;4c"));
    }

    void theRoomAnImageHasIsReported()
    {
        initSurface({80, 24});
        m_surface->setCellSize({10, 20});

        m_surface->dataFromPty("\x1b[?2;1S");
        QTRY_COMPARE(m_written, QByteArray("\x1b[?2;0;800;480S"));

        m_written.clear();
        m_surface->dataFromPty("\x1b[?1;1S");
        QTRY_COMPARE(m_written, QByteArray("\x1b[?1;0;256S"));

        m_written.clear();
        m_surface->dataFromPty("\x1b[14t");
        QTRY_COMPARE(m_written, QByteArray("\x1b[4;480;800t"));

        m_written.clear();
        m_surface->dataFromPty("\x1b[16t");
        QTRY_COMPARE(m_written, QByteArray("\x1b[6;20;10t"));
    }
};

QTEST_GUILESS_MAIN(tst_TerminalSurface)

#include "tst_terminalsurface.moc"
