#include <stdio.h>
#include "json.h"
#include "parse.h"
#include "print.h"

#ifdef DEBUG
FILE *trace;
#endif

void round_trip_test(FILE *f) {
    parse_result pr = parse_json(f);
    if (pr.success) {
        //print_json(pr.res);
        value_free(pr.res);
    }
    else {
        print_error(stderr, pr);
    }
}

int main() {
#ifdef DEBUG
    trace = fopen("trace.txt", "w");
#endif

    round_trip_test(stdin);

#ifdef DEBUG
    fclose(trace);
#endif
}
