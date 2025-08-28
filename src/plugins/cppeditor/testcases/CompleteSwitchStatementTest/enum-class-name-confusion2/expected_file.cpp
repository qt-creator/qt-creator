enum class test1 { Wrong11, Wrong12 };
enum class test { Right1, Right2 };
enum class test2 { Wrong21, Wrong22 };

int main() {
    enum test test;
    switch (test) {
    case test::Right1:
        break;
    case test::Right2:
        break;
    }
}
