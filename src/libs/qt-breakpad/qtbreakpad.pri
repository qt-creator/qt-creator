isEmpty(BREAKPAD_SOURCE_DIR): return()

HEADERS += $$PWD/qtbreakpad/qtsystemexceptionhandler.h
SOURCES += $$PWD/qtbreakpad/qtsystemexceptionhandler.cpp

DEFINES += ENABLE_QT_BREAKPAD

win32:BREAKPAD_SOURCE_DIR ~= s,\\,/,

INCLUDEPATH += \
    $$BREAKPAD_SOURCE_DIR/src \
    $$PWD/qtbreakpad

SOURCES += \
    $$BREAKPAD_SOURCE_DIR/src/common/string_conversion.cc \
    $$BREAKPAD_SOURCE_DIR/src/common/convert_UTF.c \
    $$BREAKPAD_SOURCE_DIR/src/common/md5.cc

linux:SOURCES += \
    $$BREAKPAD_SOURCE_DIR/src/client/minidump_file_writer.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/linux/log/log.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/linux/handler/exception_handler.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/linux/handler/minidump_descriptor.cc \
    $$BREAKPAD_SOURCE_DIR/src/common/linux/guid_creator.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/linux/dump_writer_common/thread_info.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/linux/dump_writer_common/ucontext_reader.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/linux/minidump_writer/linux_dumper.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/linux/minidump_writer/minidump_writer.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/linux/minidump_writer/linux_ptrace_dumper.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/linux/microdump_writer/microdump_writer.cc \
    $$BREAKPAD_SOURCE_DIR/src/common/linux/file_id.cc \
    $$BREAKPAD_SOURCE_DIR/src/common/linux/elfutils.cc \
    $$BREAKPAD_SOURCE_DIR/src/common/linux/linux_libc_support.cc \
    $$BREAKPAD_SOURCE_DIR/src/common/linux/memory_mapped_file.cc \
    $$BREAKPAD_SOURCE_DIR/src/common/linux/safe_readlink.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/linux/crash_generation/crash_generation_client.cc

win32:SOURCES += \
    $$BREAKPAD_SOURCE_DIR/src/common/windows/guid_string.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/windows/handler/exception_handler.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/windows/crash_generation/minidump_generator.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/windows/crash_generation/client_info.cc \
    $$BREAKPAD_SOURCE_DIR/src/client/windows/crash_generation/crash_generation_client.cc

macos {
    SOURCES +=  \
        $$BREAKPAD_SOURCE_DIR/src/client/minidump_file_writer.cc \
        $$BREAKPAD_SOURCE_DIR/src/client/mac/crash_generation/crash_generation_client.cc \
        $$BREAKPAD_SOURCE_DIR/src/client/mac/handler/exception_handler.cc \
        $$BREAKPAD_SOURCE_DIR/src/client/mac/handler/minidump_generator.cc \
        $$BREAKPAD_SOURCE_DIR/src/client/mac/handler/breakpad_nlist_64.cc \
        $$BREAKPAD_SOURCE_DIR/src/client/mac/handler/dynamic_images.cc \
        $$BREAKPAD_SOURCE_DIR/src/client/mac/handler/protected_memory_allocator.cc \
        $$BREAKPAD_SOURCE_DIR/src/common/mac/bootstrap_compat.cc \
        $$BREAKPAD_SOURCE_DIR/src/common/mac/file_id.cc \
        $$BREAKPAD_SOURCE_DIR/src/common/mac/macho_id.cc \
        $$BREAKPAD_SOURCE_DIR/src/common/mac/macho_reader.cc \
        $$BREAKPAD_SOURCE_DIR/src/common/mac/macho_utilities.cc \
        $$BREAKPAD_SOURCE_DIR/src/common/mac/macho_walker.cc \
        $$BREAKPAD_SOURCE_DIR/src/common/mac/string_utilities.cc
    OBJECTIVE_SOURCES += \
        $$BREAKPAD_SOURCE_DIR/src/common/mac/MachIPC.mm
    LIBS += -framework Foundation
}
