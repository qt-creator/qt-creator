void setViaPointer(int *ptr, int value)
{
    *ptr = value;
}

void user(int *arr)
{
    setViaPoin@ter(arr + 1, 5);
}
