#define Macro 40

int main(int argc, char *argv[])
{
    int x = 5 + Macro;

#undef Macro
#define Macro 70

    x += Macro;

    x += Macro;
}


