// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <solutions/terminal/terminalsurface.h>

#include <QTest>

#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStringList>

#include <memory>
#include <variant>

using namespace TerminalSolution;

class tst_TerminalSurface : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<TerminalSurface> m_surface;

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
        m_surface = std::make_unique<TerminalSurface>(size);
        m_surface->setWriteToPty([](const QByteArray &data) { return qint64(data.size()); });
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

    void resizeKeepsTheCursorOnItsCharacter_data()
    {
        QTest::addColumn<int>("promptLength");
        QTest::addColumn<QList<QSize>>("sizes");

        QTest::newRow("full row, narrower") << 19 << QList<QSize>{{10, 6}};
        QTest::newRow("full row, wider") << 19 << QList<QSize>{{40, 6}};
        QTest::newRow("short prompt") << 5 << QList<QSize>{{11, 6}, {30, 6}};
        QTest::newRow("wrapped prompt") << 34 << QList<QSize>{{13, 6}, {29, 6}};
        QTest::newRow("drag one column at a time")
            << 19
            << QList<QSize>{{19, 6}, {18, 6}, {17, 6}, {16, 6}, {17, 6}, {18, 6}, {19, 6}, {20, 6}};
    }

    void resizeKeepsTheCursorOnItsCharacter()
    {
        QFETCH(int, promptLength);
        QFETCH(QList<QSize>, sizes);

        initSurface({20, 6});
        write({"history one", "history two"});

        m_surface->dataFromPty(QString(promptLength, '.').toUtf8() + "Z");

        const auto cursorIsAfterTheZ = [this] {
            const QPoint pos = m_surface->cursor().position;
            QPoint before = pos - QPoint(1, 0);
            if (before.x() < 0)
                before = {m_surface->liveSize().width() - 1, pos.y() - 1};

            return m_surface->fetchCell(before.x(), before.y()).text == QString("Z")
                   || m_surface->fetchCell(pos.x(), pos.y()).text == QString("Z");
        };
        QVERIFY(cursorIsAfterTheZ());

        for (const QSize &size : sizes) {
            resizeTo(size);
            QVERIFY2(cursorIsAfterTheZ(),
                     qPrintable(QString("after resize to %1x%2 the cursor is at %3,%4")
                                    .arg(size.width())
                                    .arg(size.height())
                                    .arg(m_surface->cursor().position.x())
                                    .arg(m_surface->cursor().position.y())));
        }
    }

    void aWriteAfterAResizeDoesNotOverwriteTheLastCharacter()
    {
        initSurface({20, 6});

        const QString filled = QString(19, '.') + "Z";
        m_surface->dataFromPty(filled.toUtf8());
        QCOMPARE(surfaceText(), filled);

        resizeTo({10, 6});
        m_surface->dataFromPty("Y");

        QCOMPARE(surfaceText(), filled + "Y");
        QCOMPARE(rowText(2), QString("Y"));
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
};

QTEST_GUILESS_MAIN(tst_TerminalSurface)

#include "tst_terminalsurface.moc"
