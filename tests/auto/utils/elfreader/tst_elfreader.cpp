// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/elfreader.h>
#include <utils/qtcassert.h>

#include <QTemporaryDir>
#include <QTest>

#include <limits>

using namespace Utils;

namespace {

class Section
{
public:
    QByteArray name;
    quint32 type = Elf_SHT_PROGBITS;
    quint64 flags = 0;
    QByteArray data;
    quint64 declaredSize = 0; // Overrides sh_size when set, to forge a corrupt header.
    quint64 declaredOffset = 0; // Overrides sh_offset when set.
};

class ElfWriter
{
public:
    explicit ElfWriter(bool is64Bit)
        : m_is64Bit(is64Bit)
        , m_ehdrSize(is64Bit ? 64 : 52)
        , m_shentSize(is64Bit ? 64 : 40)
    {}

    QByteArray build(const QList<Section> &sections) const;

    quint64 chdrSize() const { return m_is64Bit ? 24 : 12; }

    QByteArray dynamicEntry(quint64 tag, quint64 value) const
    {
        QByteArray data;
        putNative(data, tag);
        putNative(data, value);
        return data;
    }

    QByteArray compress(quint32 type, const QByteArray &payload, quint64 declaredSize = 0) const
    {
        QByteArray data;
        putWord(data, type);
        if (m_is64Bit)
            putWord(data, 0); // ch_reserved
        putNative(data, declaredSize ? declaredSize : quint64(payload.size())); // ch_size
        putNative(data, 1); // ch_addralign
        data.append(qCompress(payload).mid(4)); // Strip qCompress()'s size prefix.
        return data;
    }

private:
    static void putHalfWord(QByteArray &b, quint16 v)
    {
        for (int i = 0; i < 2; ++i)
            b.append(char((v >> (8 * i)) & 0xff));
    }

    static void putWord(QByteArray &b, quint32 v)
    {
        for (int i = 0; i < 4; ++i)
            b.append(char((v >> (8 * i)) & 0xff));
    }

    void putNative(QByteArray &b, quint64 v) const
    {
        for (int i = 0; i < (m_is64Bit ? 8 : 4); ++i)
            b.append(char((v >> (8 * i)) & 0xff));
    }

    void putSectionHeader(QByteArray &b, quint32 nameIndex, const Section &section,
                          quint64 offset) const
    {
        putWord(b, nameIndex);
        putWord(b, section.type);
        putNative(b, section.flags);
        putNative(b, 0); // sh_addr
        putNative(b, section.declaredOffset ? section.declaredOffset : offset);
        putNative(b, section.declaredSize ? section.declaredSize : quint64(section.data.size()));
        putWord(b, 0); // sh_link
        putWord(b, 0); // sh_info
        putNative(b, 1); // sh_addralign
        putNative(b, 0); // sh_entsize
    }

    const bool m_is64Bit;
    const quint64 m_ehdrSize;
    const quint64 m_shentSize;
};

QByteArray ElfWriter::build(const QList<Section> &sections) const
{
    const QByteArray shstrtabName = ".shstrtab";

    QByteArray strtab(1, '\0');
    const quint32 shstrtabNameIndex = strtab.size();
    strtab.append(shstrtabName).append('\0');
    QList<quint32> nameIndexes;
    for (const Section &section : sections) {
        nameIndexes.append(strtab.size());
        strtab.append(section.name).append('\0');
    }

    const auto aligned = [](quint64 offset) { return (offset + 7) & ~quint64(7); };

    const quint64 strtabOffset = m_ehdrSize;
    quint64 offset = aligned(strtabOffset + strtab.size());
    QList<quint64> dataOffsets;
    for (const Section &section : sections) {
        dataOffsets.append(offset);
        offset = aligned(offset + section.data.size());
    }
    const quint64 shoff = offset;
    const quint64 sectionCount = 2 + sections.size();

    QByteArray elf;
    elf.append("\177ELF", 4);
    elf.append(char(m_is64Bit ? Elf_ELFCLASS64 : Elf_ELFCLASS32));
    elf.append(char(Elf_ELFDATA2LSB));
    elf.append(char(1)); // EI_VERSION
    elf.append(9, '\0'); // EI_OSABI and padding
    putHalfWord(elf, Elf_ET_DYN);
    putHalfWord(elf, m_is64Bit ? Elf_EM_X86_64 : Elf_EM_386);
    putWord(elf, 1); // e_version
    putNative(elf, 0); // e_entry
    putNative(elf, 0); // e_phoff
    putNative(elf, shoff);
    putWord(elf, 0); // e_flags
    putHalfWord(elf, quint16(m_ehdrSize));
    putHalfWord(elf, m_is64Bit ? 56 : 32); // e_phentsize
    putHalfWord(elf, 0); // e_phnum
    putHalfWord(elf, quint16(m_shentSize));
    putHalfWord(elf, quint16(sectionCount));
    putHalfWord(elf, 1); // e_shstrndx
    QTC_CHECK(quint64(elf.size()) == m_ehdrSize);

    elf.append(strtab);
    for (int i = 0; i < sections.size(); ++i) {
        elf.append(int(dataOffsets.at(i) - elf.size()), '\0');
        elf.append(sections.at(i).data);
    }
    elf.append(int(shoff - elf.size()), '\0');

    putSectionHeader(elf, 0, {.type = Elf_SHT_NULL}, 0);
    putSectionHeader(elf, shstrtabNameIndex,
                     {.name = shstrtabName, .type = Elf_SHT_STRTAB, .data = strtab},
                     strtabOffset);
    for (int i = 0; i < sections.size(); ++i)
        putSectionHeader(elf, nameIndexes.at(i), sections.at(i), dataOffsets.at(i));
    QTC_CHECK(quint64(elf.size()) == shoff + sectionCount * m_shentSize);

    return elf;
}

} // namespace

class tst_ElfReader : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void readSection_data();
    void readSection();
    void unmappableFile();
    void shentSizeTooSmall();
    void debugLinkBeyondEndOfFile();
    void buildIdBeyondEndOfFile();
    void neededLibrariesWrappingSection();

private:
    FilePath write(const QByteArray &contents);

    QTemporaryDir m_dir;
    int m_counter = 0;
};

void tst_ElfReader::init()
{
    QVERIFY(m_dir.isValid());
}

FilePath tst_ElfReader::write(const QByteArray &contents)
{
    const FilePath path = FilePath::fromString(m_dir.path())
                          / QString("elf%1").arg(++m_counter);
    QTC_CHECK(path.writeFileContents(contents));
    return path;
}

void tst_ElfReader::readSection_data()
{
    QTest::addColumn<bool>("is64Bit");
    QTest::addColumn<quint64>("flags");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<quint64>("declaredSize");
    QTest::addColumn<QByteArray>("lookup");
    QTest::addColumn<QByteArray>("expected");

    const QByteArray payload("/home/qt/work/qt/qtbase/src/corelib\0/usr/include", 48);
    const QByteArray name(".debug_str");

    for (const bool is64Bit : {false, true}) {
        const char *tag = is64Bit ? "64bit" : "32bit";
        const ElfWriter writer(is64Bit);
        const int chdrSize = int(writer.chdrSize());

        QTest::addRow("%s: plain", tag)
            << is64Bit << quint64(0) << payload << quint64(0) << name << payload;

        QTest::addRow("%s: unknown section", tag)
            << is64Bit << quint64(0) << payload << quint64(0)
            << QByteArray(".debug_line_str") << QByteArray();

        QTest::addRow("%s: beyond end of file", tag)
            << is64Bit << quint64(0) << payload << quint64(payload.size() + 4096)
            << name << QByteArray();

        QTest::addRow("%s: zlib compressed", tag)
            << is64Bit << quint64(Elf_SHF_COMPRESSED) << writer.compress(1, payload)
            << quint64(0) << name << payload;

        QTest::addRow("%s: zstd compressed", tag)
            << is64Bit << quint64(Elf_SHF_COMPRESSED) << writer.compress(2, payload)
            << quint64(0) << name << QByteArray();

        QTest::addRow("%s: not deflate data", tag)
            << is64Bit << quint64(Elf_SHF_COMPRESSED)
            << writer.compress(1, payload).left(chdrSize) + QByteArray(64, 'x')
            << quint64(0) << name << QByteArray();

        QTest::addRow("%s: ch_size does not match", tag)
            << is64Bit << quint64(Elf_SHF_COMPRESSED)
            << writer.compress(1, payload, quint64(payload.size() + 8))
            << quint64(0) << name << QByteArray();

        QTest::addRow("%s: header only", tag)
            << is64Bit << quint64(Elf_SHF_COMPRESSED)
            << writer.compress(1, payload).left(chdrSize) << quint64(0) << name << QByteArray();
    }
}

void tst_ElfReader::readSection()
{
    QFETCH(bool, is64Bit);
    QFETCH(quint64, flags);
    QFETCH(QByteArray, data);
    QFETCH(quint64, declaredSize);
    QFETCH(QByteArray, lookup);
    QFETCH(QByteArray, expected);

    const FilePath path = write(ElfWriter(is64Bit).build(
        {{".debug_str", Elf_SHT_PROGBITS, flags, data, declaredSize}}));

    ElfReader reader(path);
    const ElfData elfData = reader.readHeaders();
    QCOMPARE(elfData.elfclass, is64Bit ? Elf_ELFCLASS64 : Elf_ELFCLASS32);
    QCOMPARE(elfData.sectionHeaders.size(), qsizetype(3));
    QVERIFY(reader.errorString().isEmpty());

    const std::unique_ptr<ElfMapper> mapper = reader.readSection(lookup);
    if (expected.isEmpty()) {
        QVERIFY(!mapper);
        return;
    }
    QVERIFY(mapper);
    QCOMPARE(QByteArray(mapper->start, qsizetype(mapper->fdlen)), expected);
}

void tst_ElfReader::unmappableFile()
{
    const ElfWriter writer(true);
    const QByteArray payload = "/home/qt/work/qt/qtbase/src/corelib";
    const FilePath path = write(writer.build({{".debug_str", Elf_SHT_PROGBITS, 0, payload, 0}}));

    ElfReader reader(path);
    QCOMPARE(reader.readHeaders().sectionHeaders.size(), qsizetype(3));
    QVERIFY(path.removeFile());
    QVERIFY(!reader.readSection(".debug_str"));
}

void tst_ElfReader::shentSizeTooSmall()
{
    // parseSectionHeader() reads a full header whatever e_shentsize says, so
    // a stride below the real header size makes the last one read past the end.
    QByteArray elf = ElfWriter(true).build(
        {{.name = ".debug_str", .data = QByteArray(64, 'd')}});
    elf[58] = 4; // e_shentsize
    elf[59] = 0;

    ElfReader reader(write(elf));
    QVERIFY(reader.readHeaders().sectionHeaders.isEmpty());
    QVERIFY2(reader.errorString().contains("e_shentsize"),
             qPrintable(reader.errorString()));
}

void tst_ElfReader::debugLinkBeyondEndOfFile()
{
    const FilePath path = write(ElfWriter(true).build(
        {{.name = ".gnu_debuglink",
          .data = QByteArray("libfoo.debug\0", 13),
          .declaredOffset = quint64(1) << 40}}));

    ElfReader reader(path);
    const ElfData elfData = reader.readHeaders();
    QCOMPARE(elfData.symbolsType, LinkedSymbols);
    QVERIFY(elfData.debugLink.isEmpty());
}

void tst_ElfReader::buildIdBeyondEndOfFile()
{
    const FilePath path = write(ElfWriter(true).build(
        {{.name = ".note.gnu.build-id",
          .type = Elf_SHT_NOTE,
          .data = QByteArray(36, 'n'),
          .declaredSize = quint64(1) << 42}}));

    ElfReader reader(path);
    const ElfData elfData = reader.readHeaders();
    QCOMPARE(elfData.symbolsType, BuildIdSymbols);
    QVERIFY(elfData.buildId.isEmpty());
}

void tst_ElfReader::neededLibrariesWrappingSection()
{
    // The declared size wraps when added to the offset, so a bounds check
    // that adds the two lets the entry loop read .dynstr as further
    // Elf_DT_NEEDED entries, yielding "evil.so".
    const ElfWriter writer(true);
    const QByteArray dynamic = writer.dynamicEntry(Elf_DT_NEEDED, 1000);
    const QByteArray dynstr = writer.dynamicEntry(Elf_DT_NEEDED, 16)
                              + QByteArray("evil.so\0", 8);

    const FilePath path = write(writer.build(
        {{.name = ".dynamic",
          .type = Elf_SHT_DYNAMIC,
          .data = dynamic,
          .declaredSize = std::numeric_limits<quint64>::max() - 64},
         {.name = ".dynstr", .type = Elf_SHT_STRTAB, .data = dynstr}}));

    ElfReader reader(path);
    QVERIFY(reader.neededLibraries().isEmpty());
}

QTEST_GUILESS_MAIN(tst_ElfReader)

#include "tst_elfreader.moc"
