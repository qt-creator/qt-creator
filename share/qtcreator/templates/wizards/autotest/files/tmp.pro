TEMPLATE = subdirs

@if "%{BuildAutoTests}" == "always"
SUBDIRS += src \
           tests
@else
SUBDIRS += src

CONFIG(debug, debug|release) {
    SUBDIRS += tests
}
@endif
