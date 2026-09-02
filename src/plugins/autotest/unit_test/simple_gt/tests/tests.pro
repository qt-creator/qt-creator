TEMPLATE = subdirs

SUBDIRS += gt1 \
           gt2 \
           gt3 \
           gtlib \
           gt4

gt4.depends = gtlib
