int function(int* pointer, int value)
{
  if (pointer == nullptr) {
    return value + 1;
  } else {
    return value - 1;
  }
}
