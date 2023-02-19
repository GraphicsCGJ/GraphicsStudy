#include <iostream>

using namespace std;

int A = 30;

int f1(int &x) {
    A = x;
}

int f2(int &&x) {
    A = x;
}

int ret() {
    return 300;
}

int main(void) {
    cout << A << '\n';

    // f1(30); // Compile Error
    f2(40);
    cout << A << '\n';


    // f1(ret());
    f2(ret());
    cout << A << '\n';

    return 0;
}