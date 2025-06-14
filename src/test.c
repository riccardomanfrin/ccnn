#include <immintrin.h>
#include <stdio.h>
#include <sys/time.h>

struct timeval take_time(struct timeval *tprev) {
    struct timeval t;
    gettimeofday(&t, NULL);

    if (tprev != NULL) {
        if (t.tv_usec < tprev->tv_usec) {
            t.tv_usec = t.tv_usec + 1000000 - tprev->tv_usec;
            t.tv_sec = t.tv_sec - 1 - tprev->tv_sec;
        } else {
            t.tv_usec -= tprev->tv_usec;
            t.tv_sec -= tprev->tv_sec;
        }
    }
    return t;
}

void print_time(struct timeval t) {
    printf(">>>%li.%03li\n", t.tv_sec, t.tv_usec / 1000);
}

int main() {
    struct timeval t = take_time(NULL);

    for (int i = 0; i < 10000000; i++) {
        __m512d a = _mm512_set_pd(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
        __m512d b =
            _mm512_set_pd(9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0);

        // AVX-512 multiplication
        __m512d result = _mm512_mul_pd(a, b);

        // Store and print result
        double output[8];
        _mm512_storeu_pd(output, result);
    }
    t = take_time(&t);
    print_time(t);

    t = take_time(NULL);
    for (int i = 0; i < 10000000; i++) {
        int k = 42;
        
        for (int l = 0; l < 16; l++) {
            int res = k * k;
        }
    }
    t = take_time(&t);
    print_time(t);


    t = take_time(NULL);
    for (int i = 0; i < 10000000; i++) {
        double k = 42;
        
        for (int l = 0; l < 16; l++) {
            double res = k * k;
        }
    }
    t = take_time(&t);
    print_time(t);
    return 0;
}