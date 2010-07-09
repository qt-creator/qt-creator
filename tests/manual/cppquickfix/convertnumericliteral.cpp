int main()
{
    // standard case
    199;
    074;
    0x856A;
    // with type specifier
    199L;
    074L;
    0xFA0Bu;
    // uppercase X
    0X856A;
    // negativ values
    -199;
    -017;
    // not integer, do nothing
    298.3;
    // ignore invalid octal
    0783;
    0; // border case, only hex<->decimal
}
