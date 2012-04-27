struct QQ {};

#define NN(x) typedef QQ PP ## x;

NN(CC)

typedef PPCC RR;
