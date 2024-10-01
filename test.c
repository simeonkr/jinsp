#include <stdio.h>
#include "json.h"
#include "parse.h"
#include "print.h"

void round_trip_test(FILE *f) {
    parse_result pr = parse_json(f);
    if (pr.success) {
        print_json(pr.res);
    }
    else {
        print_error(stderr, pr);
    }
}

int main() {
    round_trip_test(stdin);
}
