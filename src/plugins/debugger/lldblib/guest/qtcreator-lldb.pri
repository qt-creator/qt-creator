WITH_LLDB = $$(WITH_LLDB)
macx: !isEmpty(WITH_LLDB) : SUBDIRS += $$PWD/qtcreator-lldb.pro
