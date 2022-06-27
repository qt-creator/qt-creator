// clang-tidy -export-fixes=clang-analyzer.dividezero.yaml "-checks=-*,clang-analyzer-core.DivideZero" clang-analyzer.dividezero.cpp
void test(int z) {
  if (z == 0)
    int x = 1 / z;
}
