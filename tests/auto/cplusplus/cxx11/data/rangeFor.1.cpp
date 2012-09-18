int main() {
    for (int x : {1, 2, 3}) {}
    for (int x : foo) ;
    for (int& x : array) x += 2;
}
