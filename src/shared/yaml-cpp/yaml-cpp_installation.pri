unix {
    system(pkg-config yaml-cpp --atleast-version=0.5) {
        EXTERNAL_YAML_CPP_FOUND = 1
        EXTERNAL_YAML_CPP_LIBS = $$system(pkg-config yaml-cpp --libs)
        EXTERNAL_YAML_CPP_CXXFLAGS = $$system(pkg-config yaml-cpp --cflags)
    }
}
