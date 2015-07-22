void f() noexcept {
}

void g() noexcept(1) {
}

void h() noexcept(noexcept(f())) {

}

int main() {
    bool noExcept_hf = noexcept(h()) && noexcept(f());
}
