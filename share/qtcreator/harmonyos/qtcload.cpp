// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Loads an ELF shared object into the calling process without asking the kernel to
// execute a file. HarmonyOS refuses both exec and dlopen for anything an application
// wrote itself - the SELinux type of the application's own storage carries no execute -
// but it does let a debug-signed application make anonymous memory executable, if the
// mprotect is bracketed by the platform's "jit" prctl. So the file is read, its segments
// are copied into anonymous memory, relocations are applied by hand, and only the text
// pages are handed to that guarded mprotect.
//
// Dependencies are not loaded here. Everything the object needs from outside is resolved
// with dlsym() against what the process has already loaded, which is the point: in an IDE
// the runner already holds Qt, so only the one library the user just built goes through
// this path.

#include "qtcload.h"

#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>

// Relative-relocation packing is newer than some sysroots' elf.h.
#ifndef DT_RELR
#define DT_RELRSZ 35
#define DT_RELR 36
#define DT_RELRENT 37
typedef Elf64_Xword Elf64_Relr;
#endif

// The device's elf.h is musl's and stops short of some of these.
#ifndef R_AARCH64_NONE
#define R_AARCH64_NONE 0
#endif
#ifndef R_AARCH64_ABS64
#define R_AARCH64_ABS64 257
#endif
#ifndef R_AARCH64_GLOB_DAT
#define R_AARCH64_GLOB_DAT 1025
#endif
#ifndef R_AARCH64_JUMP_SLOT
#define R_AARCH64_JUMP_SLOT 1026
#endif
#ifndef R_AARCH64_RELATIVE
#define R_AARCH64_RELATIVE 1027
#endif
#ifndef R_AARCH64_IRELATIVE
#define R_AARCH64_IRELATIVE 1032
#endif

namespace QtcLoad {

// "jit" in hex. The platform grants a debug-signed application PROT_EXEC on anonymous
// memory only while this is on; a plain mprotect is refused with EINVAL.
static const int JitPrctl = 0x6a6974;

static const size_t PageSize = 4096;

static size_t pageDown(size_t value) { return value & ~(PageSize - 1); }
static size_t pageUp(size_t value) { return (value + PageSize - 1) & ~(PageSize - 1); }

class Loader
{
public:
    Loader(Image *image, std::string *error) : m_image(image), m_error(error) {}

    bool load(const char *path);

private:
    bool fail(const char *format, ...);
    bool readFile(const char *path);
    bool mapSegments();
    bool readDynamic();
    bool loadDependencies();
    bool applyRelocations();
    bool applyRela(const Elf64_Rela *relocations, size_t count);
    bool applyRelr(const Elf64_Relr *entries, size_t count);
    bool protectSegments();
    void registerFrames();
    void *resolveInProcess(const char *name) const;
    void runInitializers();
    void *resolve(const Elf64_Sym &symbol, const char *name, bool *ok);

    const Elf64_Ehdr *header() const
    { return reinterpret_cast<const Elf64_Ehdr *>(m_file.data()); }

    char *slot(Elf64_Addr offset) const { return m_image->base + offset - m_lowest; }

    Image *m_image = nullptr;
    std::string *m_error = nullptr;
    std::vector<char> m_file;
    Elf64_Addr m_lowest = 0;
    const Elf64_Dyn *m_dynamic = nullptr;
    const char *m_strings = nullptr;
    const Elf64_Sym *m_symbols = nullptr;
    size_t m_symbolCount = 0;
    const Elf64_Rela *m_rela = nullptr;
    size_t m_relaCount = 0;
    const Elf64_Rela *m_jmprel = nullptr;
    size_t m_jmprelCount = 0;
    const Elf64_Relr *m_relr = nullptr;
    size_t m_relrCount = 0;
    std::vector<Elf64_Xword> m_neededNames;
    const char *m_ehFrame = nullptr;
    size_t m_ehFrameSize = 0;
    void (*m_init)() = nullptr;
    void (**m_initArray)() = nullptr;
    size_t m_initCount = 0;
};

bool Loader::fail(const char *format, ...)
{
    char buffer[512] = {0};
    va_list args;
    va_start(args, format);
    ::vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    if (m_error)
        *m_error = buffer;
    return false;
}

bool Loader::readFile(const char *path)
{
    const int fd = ::open(path, O_RDONLY);
    if (fd < 0)
        return fail("cannot open %s (errno %d)", path, errno);
    struct stat status = {};
    if (::fstat(fd, &status) != 0) {
        ::close(fd);
        return fail("cannot stat %s (errno %d)", path, errno);
    }
    m_file.resize(size_t(status.st_size));
    char *at = m_file.data();
    size_t left = m_file.size();
    while (left > 0) {
        const ssize_t got = ::read(fd, at, left);
        if (got <= 0) {
            ::close(fd);
            return fail("short read on %s (errno %d)", path, errno);
        }
        at += got;
        left -= size_t(got);
    }
    ::close(fd);

    if (m_file.size() < sizeof(Elf64_Ehdr) || ::memcmp(m_file.data(), ELFMAG, SELFMAG) != 0)
        return fail("%s is not an ELF file", path);
    const Elf64_Ehdr *elf = header();
    if (elf->e_ident[EI_CLASS] != ELFCLASS64 || elf->e_machine != EM_AARCH64)
        return fail("%s is not aarch64 ELF64", path);
    if (elf->e_type != ET_DYN)
        return fail("%s is not a shared object (e_type %d)", path, int(elf->e_type));
    return true;
}

bool Loader::mapSegments()
{
    const Elf64_Ehdr *elf = header();
    const Elf64_Phdr *segments =
        reinterpret_cast<const Elf64_Phdr *>(m_file.data() + elf->e_phoff);

    bool seen = false;
    Elf64_Addr highest = 0;
    for (int index = 0; index < elf->e_phnum; ++index) {
        const Elf64_Phdr &segment = segments[index];
        if (segment.p_type != PT_LOAD)
            continue;
        if (!seen || segment.p_vaddr < m_lowest)
            m_lowest = segment.p_vaddr;
        highest = std::max<Elf64_Addr>(highest, segment.p_vaddr + segment.p_memsz);
        seen = true;
    }
    if (!seen)
        return fail("no PT_LOAD segments");
    m_lowest = pageDown(m_lowest);
    m_image->size = pageUp(highest - m_lowest);

    // Anonymous and writable. RWX up front is refused outright, so execute is asked for
    // later, per segment, under the jit prctl.
    void *base = ::mmap(nullptr, m_image->size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED)
        return fail("mmap of %zu bytes failed (errno %d)", m_image->size, errno);
    m_image->base = static_cast<char *>(base);

    for (int index = 0; index < elf->e_phnum; ++index) {
        const Elf64_Phdr &segment = segments[index];
        if (segment.p_type != PT_LOAD || segment.p_filesz == 0)
            continue;
        if (segment.p_offset + segment.p_filesz > m_file.size())
            return fail("segment %d runs past the end of the file", index);
        ::memcpy(slot(segment.p_vaddr), m_file.data() + segment.p_offset, segment.p_filesz);
    }
    return true;
}

bool Loader::readDynamic()
{
    const Elf64_Ehdr *elf = header();
    const Elf64_Phdr *segments =
        reinterpret_cast<const Elf64_Phdr *>(m_file.data() + elf->e_phoff);
    for (int index = 0; index < elf->e_phnum; ++index) {
        if (segments[index].p_type == PT_DYNAMIC)
            m_dynamic = reinterpret_cast<const Elf64_Dyn *>(slot(segments[index].p_vaddr));
    }
    if (!m_dynamic)
        return fail("no PT_DYNAMIC segment");

    size_t relaSize = 0, relaEntry = sizeof(Elf64_Rela);
    size_t jmprelSize = 0, relrSize = 0, relrEntry = sizeof(Elf64_Relr);
    size_t initArraySize = 0;
    for (const Elf64_Dyn *entry = m_dynamic; entry->d_tag != DT_NULL; ++entry) {
        switch (entry->d_tag) {
        case DT_STRTAB:   m_strings = slot(entry->d_un.d_ptr); break;
        case DT_SYMTAB:
            m_symbols = reinterpret_cast<const Elf64_Sym *>(slot(entry->d_un.d_ptr));
            break;
        case DT_RELA:
            m_rela = reinterpret_cast<const Elf64_Rela *>(slot(entry->d_un.d_ptr));
            break;
        case DT_RELASZ:   relaSize = entry->d_un.d_val; break;
        case DT_RELAENT:  relaEntry = entry->d_un.d_val; break;
        case DT_JMPREL:
            m_jmprel = reinterpret_cast<const Elf64_Rela *>(slot(entry->d_un.d_ptr));
            break;
        case DT_PLTRELSZ: jmprelSize = entry->d_un.d_val; break;
        case DT_RELR:
            m_relr = reinterpret_cast<const Elf64_Relr *>(slot(entry->d_un.d_ptr));
            break;
        case DT_RELRSZ:   relrSize = entry->d_un.d_val; break;
        case DT_RELRENT:  relrEntry = entry->d_un.d_val; break;
        case DT_INIT:
            m_init = reinterpret_cast<void (*)()>(slot(entry->d_un.d_ptr));
            break;
        case DT_INIT_ARRAY:
            m_initArray = reinterpret_cast<void (**)()>(slot(entry->d_un.d_ptr));
            break;
        case DT_INIT_ARRAYSZ: initArraySize = entry->d_un.d_val; break;
        case DT_NEEDED: m_neededNames.push_back(entry->d_un.d_val); break;
        case DT_REL:
            return fail("DT_REL is not supported; only RELA and RELR are");
        default:
            break;
        }
    }
    if (!m_strings || !m_symbols)
        return fail("no dynamic string or symbol table");
    m_relaCount = relaEntry ? relaSize / relaEntry : 0;
    m_jmprelCount = jmprelSize / sizeof(Elf64_Rela);
    m_relrCount = relrEntry ? relrSize / relrEntry : 0;
    m_initCount = initArraySize / sizeof(void (*)());

    // The dynamic table carries no symbol count. The section headers are still in the
    // file buffer, so take it from there rather than guessing from a hash table.
    const Elf64_Shdr *sections =
        reinterpret_cast<const Elf64_Shdr *>(m_file.data() + elf->e_shoff);
    const char *names = elf->e_shstrndx != SHN_UNDEF
        ? m_file.data() + sections[elf->e_shstrndx].sh_offset : nullptr;
    for (int index = 0; index < elf->e_shnum; ++index) {
        if (sections[index].sh_type == SHT_DYNSYM && sections[index].sh_entsize > 0)
            m_symbolCount = sections[index].sh_size / sections[index].sh_entsize;
        if (names && ::strcmp(names + sections[index].sh_name, ".eh_frame") == 0) {
            m_ehFrame = slot(sections[index].sh_addr);
            m_ehFrameSize = sections[index].sh_size;
        }
    }
    return true;
}

// A plain dlopen("libQt6Core.so") is answered with ENOENT here: the application's own
// libraries are not on any search path the platform consults. They are loaded, though, so
// ask the loader where it put them and open that path.
static std::string loadedPath(const char *name)
{
    struct Search
    {
        const char *name;
        std::string found;
    } search{name, {}};

    ::dl_iterate_phdr([](dl_phdr_info *info, size_t, void *data) {
        Search *search = static_cast<Search *>(data);
        if (!info->dlpi_name || !*info->dlpi_name || !search->found.empty())
            return 0;
        const char *base = ::strrchr(info->dlpi_name, '/');
        if (::strcmp(base ? base + 1 : info->dlpi_name, search->name) == 0)
            search->found = info->dlpi_name;
        return 0;
    }, &search);
    return search.found;
}

// Where the process keeps its libraries. A dependency that is not loaded yet - a Qt module
// the runner itself does not use, say - is not on any search path either, but it sits next
// to one that is.
static std::vector<std::string> loadedDirectories()
{
    std::vector<std::string> directories;
    ::dl_iterate_phdr([](dl_phdr_info *info, size_t, void *data) {
        auto *found = static_cast<std::vector<std::string> *>(data);
        if (!info->dlpi_name || !*info->dlpi_name)
            return 0;
        const char *base = ::strrchr(info->dlpi_name, '/');
        if (!base)
            return 0;
        const std::string directory(info->dlpi_name, size_t(base - info->dlpi_name));
        for (const std::string &known : *found) {
            if (known == directory)
                return 0;
        }
        found->push_back(directory);
        return 0;
    }, &directories);
    return directories;
}

// Not every process has the unwinder in its global scope: an application whose libraries
// were dlopened privately keeps libc++ out of it too, and then RTLD_DEFAULT does not find
// the registration entry point that makes exceptions work.
void *Loader::resolveInProcess(const char *name) const
{
    if (void *found = ::dlsym(RTLD_DEFAULT, name))
        return found;
    for (void *handle : m_image->dependencies) {
        if (void *found = ::dlsym(handle, name))
            return found;
    }
    for (const std::string &directory : loadedDirectories()) {
        for (const char *library : {"libc++_shared.so", "libc++.so", "libunwind.so"}) {
            void *handle = ::dlopen((directory + '/' + library).c_str(), RTLD_NOW | RTLD_LOCAL);
            if (!handle)
                continue;
            if (void *found = ::dlsym(handle, name))
                return found;
            ::dlclose(handle);
        }
    }
    return nullptr;
}

// The unwinder finds an ordinary library's frames by walking the loader's list of
// images, which this one is not in, so a throw inside it reaches no handler at all and
// terminates. Hand the frames over one by one instead: LLVM's unwinder keeps a registry
// for exactly this case, and that is what a JIT uses.
void Loader::registerFrames()
{
    if (!m_ehFrame || m_ehFrameSize == 0)
        return;
    typedef void (*Register)(const void *);
    Register add = reinterpret_cast<Register>(resolveInProcess("__register_frame"));
    if (!add) {
        m_image->framesRegistered = false;
        return;
    }
    const char *at = m_ehFrame;
    const char *end = m_ehFrame + m_ehFrameSize;
    while (at + 8 <= end) {
        uint64_t length = *reinterpret_cast<const uint32_t *>(at);
        size_t headerSize = 4;
        if (length == 0xffffffffu) {           // 64-bit length form
            length = *reinterpret_cast<const uint64_t *>(at + 4);
            headerSize = 12;
        }
        if (length == 0)                       // terminator
            break;
        const uint32_t id = *reinterpret_cast<const uint32_t *>(at + headerSize);
        if (id != 0) {                         // zero marks a CIE, anything else an FDE
            add(at);
            ++m_image->frameCount;
        }
        at += headerSize + length;
    }
    m_image->framesRegistered = true;
}

// What the object needs from outside. These are ordinary libraries the platform is willing
// to load - the application's own Qt among them - so the real loader does it. The handles
// are kept because the process may hold them privately: an application library that was
// itself dlopened with RTLD_LOCAL keeps its whole dependency tree out of the global scope,
// and then RTLD_DEFAULT does not find a single Qt symbol.
bool Loader::loadDependencies()
{
    for (Elf64_Xword offset : m_neededNames) {
        const char *name = m_strings + offset;
        void *handle = ::dlopen(name, RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            const std::string path = loadedPath(name);
            if (!path.empty())
                handle = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        }
        if (!handle) {
            for (const std::string &directory : loadedDirectories()) {
                handle = ::dlopen((directory + '/' + name).c_str(), RTLD_NOW | RTLD_LOCAL);
                if (handle)
                    break;
            }
        }
        if (handle)
            m_image->dependencies.push_back(handle);
        else
            m_image->missing.push_back(std::string(name) + " (" + ::dlerror() + ")");
    }
    return true;
}

void *Loader::resolve(const Elf64_Sym &symbol, const char *name, bool *ok)
{
    *ok = true;
    if (symbol.st_shndx != SHN_UNDEF)
        return slot(symbol.st_value);
    if (!name || !*name)
        return nullptr;
    for (void *handle : m_image->dependencies) {
        if (void *found = ::dlsym(handle, name))
            return found;
    }
    if (void *found = ::dlsym(RTLD_DEFAULT, name))
        return found;
    if (ELF64_ST_BIND(symbol.st_info) == STB_WEAK)
        return nullptr;
    *ok = false;
    return nullptr;
}

bool Loader::applyRela(const Elf64_Rela *relocations, size_t count)
{
    for (size_t index = 0; index < count; ++index) {
        const Elf64_Rela &relocation = relocations[index];
        const uint32_t type = ELF64_R_TYPE(relocation.r_info);
        const uint32_t which = ELF64_R_SYM(relocation.r_info);
        uint64_t *target = reinterpret_cast<uint64_t *>(slot(relocation.r_offset));

        if (type == R_AARCH64_RELATIVE) {
            *target = uint64_t(m_image->base) - m_lowest + uint64_t(relocation.r_addend);
            continue;
        }
        if (type == R_AARCH64_NONE)
            continue;
        if (type == R_AARCH64_IRELATIVE) {
            typedef void *(*Resolver)();
            Resolver resolver = reinterpret_cast<Resolver>(
                m_image->base - m_lowest + relocation.r_addend);
            *target = uint64_t(resolver());
            continue;
        }
        if (type != R_AARCH64_ABS64 && type != R_AARCH64_GLOB_DAT
                && type != R_AARCH64_JUMP_SLOT) {
            // TLS relocations would need a thread-local block of our own; nothing that
            // reaches this loader is expected to carry them, so say so instead of
            // silently producing an image that crashes later.
            return fail("relocation type %u is not supported (symbol %s)", type,
                        which < m_symbolCount ? m_strings + m_symbols[which].st_name : "?");
        }
        if (which >= m_symbolCount)
            return fail("relocation %zu names symbol %u, out of %zu", index, which,
                        m_symbolCount);
        const Elf64_Sym &symbol = m_symbols[which];
        const char *name = m_strings + symbol.st_name;
        bool ok = false;
        void *value = resolve(symbol, name, &ok);
        if (!ok)
            return fail("undefined symbol: %s", name);
        *target = uint64_t(value) + (value ? uint64_t(relocation.r_addend) : 0);
    }
    return true;
}

bool Loader::applyRelr(const Elf64_Relr *entries, size_t count)
{
    // RELR packs a run of relative relocations: an even word is an address, an odd word
    // is a bitmap for the 63 words that follow the last address.
    const uint64_t load = uint64_t(m_image->base) - m_lowest;
    uint64_t *where = nullptr;
    for (size_t index = 0; index < count; ++index) {
        Elf64_Relr entry = entries[index];
        if ((entry & 1) == 0) {
            where = reinterpret_cast<uint64_t *>(slot(entry));
            *where++ += load;
            continue;
        }
        if (!where)
            return fail("RELR bitmap before any address");
        uint64_t *at = where;
        for (entry >>= 1; entry != 0; entry >>= 1, ++at) {
            if (entry & 1)
                *at += load;
        }
        where += 63;
    }
    return true;
}

bool Loader::applyRelocations()
{
    if (m_relr && !applyRelr(m_relr, m_relrCount))
        return false;
    if (m_rela && !applyRela(m_rela, m_relaCount))
        return false;
    if (m_jmprel && !applyRela(m_jmprel, m_jmprelCount))
        return false;
    return true;
}

bool Loader::protectSegments()
{
    const Elf64_Ehdr *elf = header();
    const Elf64_Phdr *segments =
        reinterpret_cast<const Elf64_Phdr *>(m_file.data() + elf->e_phoff);
    for (int index = 0; index < elf->e_phnum; ++index) {
        const Elf64_Phdr &segment = segments[index];
        if (segment.p_type != PT_LOAD)
            continue;
        int protection = 0;
        if (segment.p_flags & PF_R)
            protection |= PROT_READ;
        if (segment.p_flags & PF_W)
            protection |= PROT_WRITE;
        if (segment.p_flags & PF_X)
            protection |= PROT_EXEC;
        char *start = m_image->base + pageDown(segment.p_vaddr - m_lowest);
        const size_t length =
            pageUp(segment.p_vaddr - m_lowest + segment.p_memsz) - (start - m_image->base);

        if (!(protection & PROT_EXEC)) {
            if (::mprotect(start, length, protection) != 0)
                return fail("mprotect of segment %d failed (errno %d)", index, errno);
            continue;
        }
        // The one thing the platform insists on.
        ::prctl(JitPrctl, 0, 0);
        const int result = ::mprotect(start, length, protection);
        const int failure = errno;
        ::prctl(JitPrctl, 0, 1);
        if (result != 0)
            return fail("mprotect PROT_EXEC of segment %d failed (errno %d)", index, failure);
    }
    __builtin___clear_cache(m_image->base, m_image->base + m_image->size);
    return true;
}

void Loader::runInitializers()
{
    if (m_init)
        m_init();
    for (size_t index = 0; index < m_initCount; ++index) {
        if (m_initArray[index])
            m_initArray[index]();
    }
}

bool Loader::load(const char *path)
{
    if (!readFile(path) || !mapSegments() || !readDynamic() || !loadDependencies()
            || !applyRelocations() || !protectSegments())
        return false;
    registerFrames();
    m_image->lowest = m_lowest;
    m_image->symbols = m_symbols;
    m_image->symbolCount = m_symbolCount;
    m_image->strings = m_strings;
    runInitializers();
    return true;
}

bool load(const char *path, Image *image, std::string *error)
{
    Loader loader(image, error);
    if (loader.load(path))
        return true;
    if (image->base) {
        ::munmap(image->base, image->size);
        image->base = nullptr;
    }
    return false;
}

void *lookup(const Image &image, const char *name)
{
    for (size_t index = 0; index < image.symbolCount; ++index) {
        const Elf64_Sym &symbol = image.symbols[index];
        if (symbol.st_shndx == SHN_UNDEF || symbol.st_value == 0)
            continue;
        if (::strcmp(image.strings + symbol.st_name, name) == 0)
            return image.base + symbol.st_value - image.lowest;
    }
    return nullptr;
}

} // namespace QtcLoad
