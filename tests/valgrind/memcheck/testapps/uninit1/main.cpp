#pragma GCC diagnostic ignored "-Wuninitialized"

int main()
{
    bool b;
    if (b) {
        return 1;
    }

    return 0;
}
