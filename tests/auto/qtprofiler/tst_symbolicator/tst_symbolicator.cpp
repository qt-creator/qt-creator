// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "symbolicator.h"

#include <QtTest>

#include <mach/mach.h>

#include <dlfcn.h>

#include <cstdint>

using namespace QmlProfiler::Internal;

// A function defined in this test binary itself, used to check that an address
// inside the running executable resolves to both this image and (when
// CoreSymbolication is available) this symbol name. Exported with default
// visibility and kept distinct (noinline) so the symbol survives.
extern "C" __attribute__((visibility("default"), noinline))
int qtprofiler_symbolicatorOwnFunction(int seed)
{
    volatile int acc = seed;
    for (int i = 0; i < 16; ++i)
        acc += i + seed;
    return acc;
}

namespace {

quint64 addressOf(const void *p)
{
    return quint64(reinterpret_cast<uintptr_t>(p));
}

// Finds the first image whose file name contains `needle` (case-insensitively).
const Image *imageContaining(const std::vector<Image> &images, QLatin1StringView needle)
{
    for (const Image &img : images) {
        if (img.name.contains(needle, Qt::CaseInsensitive))
            return &img;
    }
    return nullptr;
}

} // namespace

class tst_Symbolicator : public QObject
{
    Q_OBJECT

private slots:
    // demangle() turns mangled Itanium names into readable ones and passes
    // everything else (plain C names, empty/null) through unchanged.
    void demangleHandlesManglingAndPlainNames()
    {
        QCOMPARE(demangle("_Z3foov"), QStringLiteral("foo()"));
        QCOMPARE(demangle("_ZN3Foo3barEi"), QStringLiteral("Foo::bar(int)"));
        QCOMPARE(demangle("main"), QStringLiteral("main"));
        QCOMPARE(demangle("qtprofiler_symbolicatorOwnFunction"),
                 QStringLiteral("qtprofiler_symbolicatorOwnFunction"));
        QVERIFY(demangle(nullptr).isEmpty());
        QVERIFY(demangle("").isEmpty());
    }

    // readImages() returns the running process's images sorted by base address,
    // and moduleAndOffset()/moduleOffsetLabel() attribute an address inside this
    // binary back to it. Needs no special privileges (reads our own task).
    void readsAndAttributesOwnImages()
    {
        const std::vector<Image> images = readImages(mach_task_self());
        QVERIFY(!images.empty());
        for (size_t i = 1; i < images.size(); ++i)
            QVERIFY(images[i - 1].base <= images[i].base);

        const quint64 addr = addressOf(reinterpret_cast<const void *>(
            &qtprofiler_symbolicatorOwnFunction));

        QString module;
        quint64 offset = 0;
        moduleAndOffset(addr, images, &module, &offset);
        QVERIFY(!module.isEmpty());
        QVERIFY2(module.contains("symbolicator", Qt::CaseInsensitive),
                 qPrintable(module));
        QVERIFY(offset > 0);
        QCOMPARE(moduleOffsetLabel(addr, images),
                 QStringLiteral("%1+0x%2").arg(module).arg(offset, 0, 16));

        // An address below every image base belongs to no module and is rendered
        // as a bare hex address.
        QString noModule;
        quint64 noOffset = 0;
        moduleAndOffset(0x1, images, &noModule, &noOffset);
        QVERIFY(noModule.isEmpty());
        QCOMPARE(noOffset, quint64(0x1));
        QCOMPARE(moduleOffsetLabel(0x1, images), QStringLiteral("0x1"));
    }

    // The heart of the dynamic-library case: a freshly dlopen()'d library shows
    // up in readImages()/loadedImageCount() afterwards, so the sampler can pick up
    // code loaded mid-recording.
    void imageListPicksUpDlopenedLibrary()
    {
        const std::vector<Image> before = readImages(mach_task_self());
        const uint32_t countBefore = loadedImageCount(mach_task_self());
        QVERIFY(imageContaining(before, QLatin1StringView("symbolicatorfixture")) == nullptr);

        void *handle = dlopen(SYMBOLICATOR_FIXTURE_DYLIB, RTLD_NOW);
        QVERIFY2(handle, dlerror());
        const QScopeGuard closeHandle([&] { dlclose(handle); });

        QVERIFY(loadedImageCount(mach_task_self()) > countBefore);
        const std::vector<Image> after = readImages(mach_task_self());
        QVERIFY(after.size() > before.size());
        QVERIFY(imageContaining(after, QLatin1StringView("symbolicatorfixture")) != nullptr);
    }

    // End to end through LiveLabeler: after a library is loaded mid-session and
    // the labeler refreshes its image list, an address in that library is
    // attributed to it (and, when CoreSymbolication is present, to its symbol).
    // This mirrors what capture() does when the target dlopen()s a plugin.
    void liveLabelerSymbolizesDlopenedLibrary()
    {
        SampleTraceData data;
        Symbolicator symbolicator(mach_task_self());
        LiveLabeler labeler(mach_task_self(), symbolicator, data);

        void *handle = dlopen(SYMBOLICATOR_FIXTURE_DYLIB, RTLD_NOW);
        QVERIFY2(handle, dlerror());
        const QScopeGuard closeHandle([&] { dlclose(handle); });

        void *fn = dlsym(handle, "qtprofiler_symbolicatorFixtureFunction");
        QVERIFY2(fn, dlerror());
        const quint64 addr = addressOf(fn);

        // Without the refresh the labeler's image list (and the symbolicator's
        // image snapshot) predate the dlopen, so the new library must be picked
        // up explicitly first.
        labeler.refreshImagesIfChanged();

        const int id = labeler.labelIdFor(addr);
        QVERIFY(id >= 0);
        QVERIFY(id < data.labels.size());
        const SampleTraceData::Label &label = data.labels.at(id);

        // Module attribution works regardless of CoreSymbolication availability.
        QVERIFY2(label.module.contains("symbolicatorfixture", Qt::CaseInsensitive),
                 qPrintable(label.module));

        // The symbol name only resolves when the private framework is present;
        // otherwise the label falls back to "module+0xoffset".
        if (symbolicator.isValid()) {
            QVERIFY2(label.name.contains("qtprofiler_symbolicatorFixtureFunction"),
                     qPrintable(label.name));
        } else {
            QVERIFY2(label.name.contains("symbolicatorfixture", Qt::CaseInsensitive),
                     qPrintable(label.name));
        }

        // Re-resolving the same address is cached and yields the same label.
        QCOMPARE(labeler.labelIdFor(addr), id);
    }

    // Symbolicating an address inside the running test binary against our own
    // task. Skips when the private CoreSymbolication framework is unavailable.
    void symbolizesOwnTask()
    {
        Symbolicator symbolicator(mach_task_self());
        if (!symbolicator.isValid())
            QSKIP("CoreSymbolication is unavailable on this system.");

        const quint64 addr = addressOf(reinterpret_cast<const void *>(
            &qtprofiler_symbolicatorOwnFunction));
        const QString name = symbolicator.name(addr);
        QVERIFY2(name.contains("qtprofiler_symbolicatorOwnFunction"), qPrintable(name));
    }
};

QTEST_GUILESS_MAIN(tst_Symbolicator)

#include "tst_symbolicator.moc"
