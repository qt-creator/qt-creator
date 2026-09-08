void setViaPointer(int *ptr, int value)
{
    *ptr = value;
}

void user(int *arr)
{
    *(arr + 1) = 5;
}
