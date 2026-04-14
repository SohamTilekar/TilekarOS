#include <stdint.h>
#include <stddef.h>

// Simple bit-by-bit 64-bit unsigned division and modulus
// Required for 32-bit systems when using 64-bit integers with / and %

uint64_t __udivmoddi4(uint64_t n, uint64_t d, uint64_t *r) {
    uint64_t q = 0;
    uint64_t rem = 0;
    for (int i = 63; i >= 0; i--) {
        rem = (rem << 1) | ((n >> i) & 1);
        if (rem >= d) {
            rem -= d;
            q |= (1ULL << i);
        }
    }
    if (r) *r = rem;
    return q;
}

uint64_t __udivdi3(uint64_t n, uint64_t d) {
    return __udivmoddi4(n, d, NULL);
}

uint64_t __umoddi3(uint64_t n, uint64_t d) {
    uint64_t r;
    __udivmoddi4(n, d, &r);
    return r;
}

// Signed versions can be added if needed, but printf mostly uses unsigned for internal formatting
int64_t __divdi3(int64_t n, int64_t d) {
    uint64_t un = (n < 0) ? -n : n;
    uint64_t ud = (d < 0) ? -d : d;
    uint64_t uq = __udivdi3(un, ud);
    if ((n < 0) ^ (d < 0)) return -(int64_t)uq;
    return (int64_t)uq;
}

int64_t __moddi3(int64_t n, int64_t d) {
    uint64_t un = (n < 0) ? -n : n;
    uint64_t ud = (d < 0) ? -d : d;
    uint64_t ur;
    __udivmoddi4(un, ud, &ur);
    if (n < 0) return -(int64_t)ur;
    return (int64_t)ur;
}
