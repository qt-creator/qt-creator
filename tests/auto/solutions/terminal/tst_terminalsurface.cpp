// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <solutions/terminal/terminalsurface.h>

#include <QTest>

#include <memory>

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
    void init()
    {
        m_surface = std::make_unique<TerminalSurface>(QSize{80, 24});
        m_surface->setWriteToPty([](const QByteArray &data) { return qint64(data.size()); });
    }

    void cleanup() { m_surface.reset(); }

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
