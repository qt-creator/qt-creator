TEMPLATE = subdirs

@if "%BuildTests%" == "always"
SUBDIRS += src \
           tests
@else
SUBDIRS += src

CONFIG(debug, debug|release) {
    SUBDIRS += tests
}
@endif
