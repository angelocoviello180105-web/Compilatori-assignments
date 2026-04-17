int testMolt(int x) {
    int a = x * 2;
    int b = a * 15;
    int c = b * 18;
    int d = c * 24;
    return a+b+c+d;
}

float testDiv(int x) {
    float a = x / 4;
    float b = x / 16;
    float c = x / 17;
    return a+b+c;
}

int main() {
    return testMolt(3);
}
