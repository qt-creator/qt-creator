// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfdwarfunwinder.h"

#include <QFile>
#include <QTest>
#include <QtEndian>

using namespace Profiler::Internal;

// Unwinds samples a Linux program recorded of itself (see fixture/), on
// whatever machine runs the test: a recording is unwound wherever it is
// opened.
class tst_DwarfUnwinder : public QObject
{
    Q_OBJECT

private slots:
    void unwindsByCfi_data();
    void unwindsByCfi();
};

struct Symbol
{
    QString name;
    quint64 start = 0;
    quint64 size = 0;
};

// "nm -S" lines: address, size, type, name.
static QList<Symbol> readSymbols(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QList<Symbol> symbols;
    while (!file.atEnd()) {
        const QList<QByteArray> fields = file.readLine().simplified().split(' ');
        if (fields.size() == 4)
            symbols.append({QString::fromLatin1(fields[3]), fields[0].toULongLong(nullptr, 16),
                            fields[1].toULongLong(nullptr, 16)});
    }
    return symbols;
}

static QString functionAt(const QList<Symbol> &symbols, quint64 address)
{
    for (const Symbol &symbol : symbols) {
        if (address >= symbol.start && address < symbol.start + symbol.size)
            return symbol.name;
    }
    return QString::number(address, 16);
}

// What fixture.c writes: the number of registers, the registers, the size of
// the stack and the stack, all little endian.
static UnwindInput readSample(const QString &path, PerfArchitecture arch)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QByteArray data = file.readAll();
    const auto word = [&data](qsizetype offset, int size) -> std::optional<quint64> {
        if (offset + size > data.size())
            return {};
        return size == 4 ? qFromLittleEndian<quint32>(data.constData() + offset)
                         : qFromLittleEndian<quint64>(data.constData() + offset);
    };
    const std::optional<quint64> count = word(0, 4);
    const PerfRegisterLayout layout = perfRegisterLayout(arch);
    if (!count || *count != quint64(layout.count))
        return {};
    UnwindInput input;
    input.arch = arch;
    qsizetype offset = 4;
    for (int i = 0; i < layout.count; ++i, offset += 8)
        input.regs.append(word(offset, 8).value_or(0));
    const std::optional<quint64> stackSize = word(offset, 8);
    offset += 8;
    if (!stackSize || offset + qsizetype(*stackSize) != data.size())
        return {};
    input.stackStartAddr = input.regs.at(layout.sp);
    input.stackBytes = data.mid(offset);
    return input;
}

void tst_DwarfUnwinder::unwindsByCfi_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("arch");

    QTest::newRow("x86_64") << QString("x86_64") << int(PerfArchitecture::X86_64);
    QTest::newRow("aarch64") << QString("aarch64") << int(PerfArchitecture::Aarch64);
}

// Without frame pointers, only the CFI leads from one frame to the next.
void tst_DwarfUnwinder::unwindsByCfi()
{
    QFETCH(QString, name);
    QFETCH(int, arch);

    const QString binary = QString(FIXTURE_DIR "/fixture-") + name;
    const QList<Symbol> symbols = readSymbols(binary + ".symbols");
    QCOMPARE(symbols.size(), 5);
    const UnwindInput input = readSample(binary + ".sample", PerfArchitecture(arch));
    QVERIFY(input.isValid());

    // Not position independent: mapped where it was linked.
    PerfDwarfUnwinder unwinder({{binary, 0}});
    QVERIFY(unwinder.isValid());
    QStringList functions;
    for (quint64 pc : unwinder.unwind(input)) // innermost first
        functions.append(functionAt(symbols, pc));

    const QStringList expected{"leaf", "level3", "level2", "level1", "main"};
    QVERIFY2(functions.size() >= expected.size(), qPrintable(functions.join(", ")));
    QCOMPARE(functions.first(expected.size()), expected);
}

QTEST_GUILESS_MAIN(tst_DwarfUnwinder)

#include "tst_dwarfunwinder.moc"
