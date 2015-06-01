TEMPLATE = subdirs
SUBDIRS = codemodelbackendipc \
          codemodelbackendprocess \
          codemodelbackendprocesstest \
          clang

codemodelbackendprocess.depends = codemodelbackendipc
codemodelbackendprocesstest.depends = codemodelbackendprocess codemodelbackendipc clang
