#!/bin/bash

export CXX="g++ -O3 -g "

function stat() {
    echo -e ""
    echo -e "Symbols:  $(nm $@ | grep ' [TVW] ' | grep _Z | grep -v "_ZNSs" | wc -l)"
    echo -e "Size:  $(size -d -B $@ | tail -1 | cut -c1-10)"
    echo -e "Debug:    $(echo "ibase=16;"$(readelf -S $@ | grep debug_info | cut -c50-56 | tr a-f A-F) | bc)"
    echo -e ""
    echo -e ""
#    echo -e "Symbols: $(nm "$@" | grep ' [TUVW] '|  grep _Z )"
}


function test_1() {

echo "Interface/Implementation (out of line)"

cat > interface_1.h <<EOF
#include <string>
struct Interface { virtual ~Interface() {} virtual std::string func() const = 0; };
EOF

cat > implementation_1.h <<EOF
#include "interface_1.h"
struct Implementation : Interface { std::string func() const; };
EOF

cat > implementation_1.cpp <<EOF
#include "implementation_1.h"
std::string Implementation::func() const { return "Test1"; }
EOF

cat > main_1.cpp <<EOF
#include "implementation_1.h"
int main() { Interface *x = new Implementation(); return x->func().size(); }
EOF

$CXX implementation_1.cpp main_1.cpp -o test_1
stat test_1

}


function test_2() {

echo "Interface/Implementation (non-virtual)"

cat > interface_2.h <<EOF
#include <string>
struct Interface { virtual ~Interface() {} virtual std::string func() const = 0; };
EOF

cat > implementation_2.h <<EOF
#include "interface_2.h"
struct Implementation : Interface {
    ~Implementation() {} std::string func() const { return "Test2"; } };
EOF

cat > main_2.cpp <<EOF
#include "implementation_2.h"
int main() { Interface *x = new Implementation(); return x->func().size(); }
EOF

$CXX main_2.cpp -o test_2
stat test_2

}


function test_3() {

echo "Interface/Implementation (inline)"

cat > interface_3.h <<EOF
#include <string>
struct Interface
{
    virtual ~Interface() {}
    std::string func() const { return m_data; }
    std::string m_data;
};
EOF

cat > implementation_3.h <<EOF
#include "interface_3.h"
struct Implementation : Interface {
    Implementation() { m_data = "Test3"; }
};
EOF

cat > main_3.cpp <<EOF
#include "implementation_3.h"
int main() { Interface *x = new Implementation(); return x->func().size(); }
EOF

$CXX main_3.cpp -o test_3
stat test_3

}


function test_4() {

echo "Merged"

cat > interface_4.h <<EOF
#include <string>
struct Interface
{
    virtual ~Interface() {}
    std::string func() const { return m_data; }
    void setData(const std::string &data) { m_data = data; }
private:
    std::string m_data;
};
EOF

cat > implementation_4.h <<EOF
#include "interface_4.h"
void setupImplementation(Interface *i) { i->setData("Test4"); }
EOF

cat > main_4.cpp <<EOF
#include "implementation_4.h"
int main() { Interface *x = new Interface(); setupImplementation(x); return x->func().size(); }
EOF

$CXX main_4.cpp -o test_4
stat test_4

}


function test_5() {

echo "Slimmest possible"

cat > interface_5.h <<EOF
#include <string>
struct Interface
{
    std::string func() const { return m_data; }
    void setData(const std::string &data) { m_data = data; }
private:
    std::string m_data;
};
EOF

cat > main_5.cpp <<EOF
#include "interface_5.h"
int main() { Interface *x = new Interface(); x->setData("Test4"); return x->func().size(); }
EOF

$CXX main_5.cpp -o test_5
stat test_5

}


function test_6() {

echo "Constructor"

cat > interface_6.h <<EOF
#include <string>
struct Interface
{
    explicit Interface(const std::string &data) : m_data(data) {}
    std::string func() const { return m_data; }
private:
    const std::string m_data;
};
EOF

cat > main_6.cpp <<EOF
#include "interface_6.h"
int main() { Interface *x = new Interface("Test4"); return x->func().size(); }
EOF

$CXX main_6.cpp -o test_6
stat test_6

}

function main() {
    test_1
    test_2
    test_3
    test_4
    test_5
    #test_6
}

main
