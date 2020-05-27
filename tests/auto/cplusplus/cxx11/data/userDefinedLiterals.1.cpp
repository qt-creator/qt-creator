constexpr long double operator"" _inv(long double value) {
  return 1.0 / value;
}
int main() {
  auto foo = operator"" _inv(2.3);
  return 12_km + 0.5_Pa + 'c'_X + "abd"_L + u"xyz"_M + 10ms;
}
