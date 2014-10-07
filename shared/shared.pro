TEMPLATE = aux

content.files = \
    autotest

content.path = $$QTC_PREFIX/share/qtcreator/templates/wizards

OTHER_FILES += $${content.files}

INSTALLS += content
