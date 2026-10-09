// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dwarflinetable_test.h"

#include <profiler/dwarflinetable.h>

#include <utils/elfreader.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>

using namespace Utils;
using namespace Qt::StringLiterals;

namespace Profiler::Internal {

namespace {

// Deliberately several distinct statements: enough for a compiler to emit a
// non-trivial line-number program (several rows, more than one line number)
// without this test caring about the exact address<->line mapping a specific
// compiler version chooses -- see the tests below for what is and isn't
// asserted about it.
const char kSource[] = R"(
int add(int a, int b)
{
    int c = a + b;
    return c;
}

int main(void)
{
    return add(1, 2);
}
)";

// Compiles and links `source` with `extraArgs` (e.g. "-g -gdwarf-5", or
// nothing for a no-debug-info build) using whatever C compiler is in PATH.
// Returns the resulting executable's path, or nullopt if no compiler is
// available or compilation failed. A real linked executable (rather than a
// ".o") both matches what PerfRecordReader actually symbolicates in
// production (process images, not relocatable objects) and avoids
// ElfReader's program-header sanity check firing on a relocatable object's
// header, which has none.
std::optional<FilePath> compileToExecutable(const QString &source, const QStringList &extraArgs,
                                            const QTemporaryDir &dir)
{
    const QString srcPath = dir.filePath("dwarftest.c");
    QFile src(srcPath);
    if (!src.open(QIODevice::WriteOnly))
        return std::nullopt;
    src.write(source.toUtf8());
    src.close();

    const FilePath compiler = Environment::systemEnvironment().searchInPath("cc");
    if (compiler.isEmpty())
        return std::nullopt;

    const QString exePath = dir.filePath("dwarftest");
    QProcess proc;
    proc.start(compiler.toFSPathString(), QStringList{srcPath, "-o", exePath} + extraArgs);
    if (!proc.waitForFinished(30000) || proc.exitStatus() != QProcess::NormalExit
        || proc.exitCode() != 0) {
        return std::nullopt;
    }
    return FilePath::fromString(exePath);
}

void appendLe(QByteArray &out, quint64 value, int bytes)
{
    for (int i = 0; i < bytes; ++i)
        out.append(char((value >> (8 * i)) & 0xff));
}

void appendUleb(QByteArray &out, quint64 value)
{
    do {
        quint8 byte = value & 0x7f;
        value >>= 7;
        if (value)
            byte |= 0x80;
        out.append(char(byte));
    } while (value);
}

// A .debug_line section of one unit: `version`, then everything after the
// header_length field up to the line program -- `afterHeaderLength` -- and
// then `program`.
QByteArray buildLineUnit(quint16 version, const QByteArray &afterHeaderLength,
                         const QByteArray &program)
{
    QByteArray unit;
    appendLe(unit, version, 2);
    if (version >= 5) {
        unit.append(char(8)); // address_size
        unit.append(char(0)); // segment_selector_size
    }
    appendLe(unit, quint64(afterHeaderLength.size()), 4);
    unit.append(afterHeaderLength);
    unit.append(program);

    QByteArray section;
    appendLe(section, quint64(unit.size()), 4);
    section.append(unit);
    return section;
}

// minimum_instruction_length .. standard_opcode_lengths, as every compiler
// writes them.
QByteArray standardLineHeaderFields()
{
    QByteArray fields;
    fields.append(char(1));    // minimum_instruction_length
    fields.append(char(1));    // maximum_operations_per_instruction (DWARF4+)
    fields.append(char(1));    // default_is_stmt
    fields.append(char(-5));   // line_base
    fields.append(char(14));   // line_range
    fields.append(char(13));   // opcode_base
    fields.append(QByteArray("\x00\x01\x01\x01\x01\x00\x00\x00\x01\x00\x00\x01", 12));
    return fields;
}

QByteArray readElfSection(const FilePath &path, const QByteArray &name)
{
    ElfReader reader(path);
    reader.readHeaders();
    const std::unique_ptr<ElfMapper> mapper = reader.readSection(name);
    if (!mapper || mapper->fdlen == 0)
        return {};
    return QByteArray(mapper->start, int(mapper->fdlen));
}

// A minimal, single-symbol ELF64 ".symtab" lookup: enough to find where the
// real compiler actually placed a named function, so the tests below can
// look up a real, meaningful address instead of guessing one. Deliberately
// not shared with perfrecordreader.cpp's own (fuller, STT_FUNC-filtering)
// symbol-table reader -- this only ever needs one symbol's value.
std::optional<quint64> findSymbolAddress(const FilePath &path, const QByteArray &symbolName)
{
    ElfReader reader(path);
    if (reader.readHeaders().elfclass != Elf_ELFCLASS64)
        return std::nullopt; // this test only exercises the common 64-bit case
    const std::unique_ptr<ElfMapper> symtab = reader.readSection(".symtab");
    const std::unique_ptr<ElfMapper> strtab = reader.readSection(".strtab");
    if (!symtab || !strtab || symtab->fdlen == 0 || strtab->fdlen == 0)
        return std::nullopt;
    const qsizetype count = qsizetype(symtab->fdlen) / 24; // Elf64_Sym
    for (qsizetype i = 0; i < count; ++i) {
        const char *sym = symtab->start + i * 24;
        const quint32 nameIndex = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(sym));
        if (nameIndex >= strtab->fdlen)
            continue;
        const char *name = strtab->start + nameIndex;
        const char *end = strtab->start + strtab->fdlen;
        const char *nul = name;
        while (nul < end && *nul != '\0')
            ++nul;
        if (QByteArray(name, int(nul - name)) != symbolName)
            continue;
        return qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(sym + 8));
    }
    return std::nullopt;
}

} // namespace

class DwarfLineTableTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testParsesRealCompilerOutput_data();
    void testParsesRealCompilerOutput();
    void testEmptyForBinaryWithoutDebugInfo();
    void testMalformedSectionDoesNotCrash();
    void testRejectsImpossibleEntryCount();
    void testSequenceStartWinsOverAdjacentEnd();
};

// This whole test needs an ELF+DWARF-producing toolchain, which every
// environment able to build Qt Creator itself already has (this plugin's own
// build just used one) -- but guard anyway rather than assume, and QSKIP
// (not fail) if either is actually missing. QSKIP inside initTestCase()
// skips every test function in this class, not just this one.
void DwarfLineTableTest::initTestCase()
{
    if (!HostOsInfo::isLinuxHost()) {
        QSKIP("DwarfLineTable is only exercised against ELF/DWARF, produced here via a "
              "Linux C compiler");
    }
    if (Environment::systemEnvironment().searchInPath("cc").isEmpty())
        QSKIP("no C compiler (\"cc\") in PATH");
}

// The real target: what an actual, unmodified GCC/Clang emits. Hand-encoding
// DWARF5's form/entry-format-driven line-table header by hand would be far
// more fragile than just compiling something and reading the real result --
// this is also, deliberately, the same toolchain default parseUnitBody() has
// to cope with (see dwarflinetable.cpp), so this is a version this parser
// must handle, not an arbitrary choice. -gdwarf-4 exercises the older,
// simpler (plain NUL-terminated-string) directory/file table format the same
// state machine also has to support.
void DwarfLineTableTest::testParsesRealCompilerOutput_data()
{
    QTest::addColumn<QString>("dwarfVersionFlag");
    QTest::newRow("default (whatever this compiler defaults to)") << QString();
    QTest::newRow("dwarf4") << QStringLiteral("-gdwarf-4");
    QTest::newRow("dwarf5") << QStringLiteral("-gdwarf-5");
}

void DwarfLineTableTest::testParsesRealCompilerOutput()
{
    QFETCH(QString, dwarfVersionFlag);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QStringList args = {"-g", "-O0"};
    if (!dwarfVersionFlag.isEmpty())
        args << dwarfVersionFlag;
    const std::optional<FilePath> exe = compileToExecutable(QString::fromLatin1(kSource), args, dir);
    if (!exe)
        QSKIP("failed to compile the test fixture");

    const DwarfLineTable table = DwarfLineTable::parse(readElfSection(*exe, ".debug_line"),
                                                        readElfSection(*exe, ".debug_line_str"),
                                                        readElfSection(*exe, ".debug_str"));
    QVERIFY(!table.isEmpty());

    const std::optional<quint64> addAddr = findSymbolAddress(*exe, "add");
    QVERIFY(addAddr.has_value());
    const std::optional<SourceLocation> loc = table.lookup(*addAddr);
    QVERIFY(loc.has_value());
    QVERIFY2(loc->file.endsWith("dwarftest.c"_L1), qPrintable(loc->file));
    // Not asserting the exact line: whether a compiler attributes a
    // function's entry address to its signature line or its opening brace
    // is toolchain-specific. Both are within the source's first few lines.
    QVERIFY2(loc->line >= 1 && loc->line <= 5, qPrintable(QString::number(loc->line)));

    // Near address 0, well below where any loaded code lives even for a
    // tiny PIE executable.
    QVERIFY(!table.lookup(0).has_value());
}

void DwarfLineTableTest::testEmptyForBinaryWithoutDebugInfo()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const std::optional<FilePath> exe = compileToExecutable(QString::fromLatin1(kSource), {"-O0"}, dir);
    if (!exe)
        QSKIP("failed to compile the test fixture");

    const DwarfLineTable table = DwarfLineTable::parse(readElfSection(*exe, ".debug_line"),
                                                        readElfSection(*exe, ".debug_line_str"),
                                                        readElfSection(*exe, ".debug_str"));
    QVERIFY(table.isEmpty());
    QVERIFY(!table.lookup(0).has_value());
}

// A section that isn't a real line program (garbage, or a truncated real
// one) must degrade to "no rows", never crash -- this is a best-effort
// enrichment layered on top of already-working symbol resolution (see
// dwarflinetable.h), so a hostile or corrupt ".debug_line" must not be able
// to take down the whole capture.
void DwarfLineTableTest::testMalformedSectionDoesNotCrash()
{
    const DwarfLineTable allOnes = DwarfLineTable::parse(QByteArray(64, '\xff'), {}, {});
    QVERIFY(allOnes.isEmpty());

    const DwarfLineTable allZeros = DwarfLineTable::parse(QByteArray(64, '\0'), {}, {});
    QVERIFY(allZeros.isEmpty());

    const DwarfLineTable tooShort = DwarfLineTable::parse(QByteArray(2, '\0'), {}, {});
    QVERIFY(tooShort.isEmpty());
}

// A DWARF5 directory table claiming about 2^31 entries, each of no format at
// all: a corrupt table, which must come out empty rather than allocate
// gigabytes, or loop, before it notices.
void DwarfLineTableTest::testRejectsImpossibleEntryCount()
{
    QByteArray header = standardLineHeaderFields();
    header.append(char(0));        // directory_entry_format_count
    appendUleb(header, 0x7fffffff); // directories_count
    const QByteArray section = buildLineUnit(5, header, {});

    const DwarfLineTable table = DwarfLineTable::parse(section, {}, {});
    QVERIFY(table.isEmpty());
}

// Sequences need not come in address order -- one per function with
// -ffunction-sections, say. Where one ends exactly at the address the next
// starts at, the start is what covers that address.
void DwarfLineTableTest::testSequenceStartWinsOverAdjacentEnd()
{
    QByteArray header = standardLineHeaderFields();
    header.append(char(0));            // include_directories: none
    header.append("a.c", 4);           // file_names[1], NUL included
    appendUleb(header, 0);             // directory index
    appendUleb(header, 0);             // mtime
    appendUleb(header, 0);             // length
    header.append(char(0));            // end of file_names

    const auto sequence = [](quint64 start, int line) {
        QByteArray program;
        program.append(char(0));       // DW_LNE_set_address
        appendUleb(program, 9);
        program.append(char(2));
        appendLe(program, start, 8);
        program.append(char(3));       // DW_LNS_advance_line
        appendUleb(program, quint64(line - 1)); // small positive: its SLEB is its ULEB
        program.append(char(1));       // DW_LNS_copy
        program.append(char(2));       // DW_LNS_advance_pc
        appendUleb(program, 0x10);
        program.append(char(0));       // DW_LNE_end_sequence
        appendUleb(program, 1);
        program.append(char(1));
        return program;
    };
    // Written first, at the higher address; the second ends where it starts.
    const QByteArray section
        = buildLineUnit(4, header, sequence(0x2000, 10) + sequence(0x1ff0, 20));

    const DwarfLineTable table = DwarfLineTable::parse(section, {}, {});
    QVERIFY(!table.isEmpty());
    const std::optional<SourceLocation> before = table.lookup(0x1ff8);
    QVERIFY(before);
    QCOMPARE(before->line, 20);
    const std::optional<SourceLocation> at = table.lookup(0x2000);
    QVERIFY(at);
    QCOMPARE(at->line, 10);
    QVERIFY(at->file.endsWith("a.c"_L1));
}

QObject *createDwarfLineTableTest()
{
    return new DwarfLineTableTest;
}

} // namespace Profiler::Internal

#include "dwarflinetable_test.moc"
