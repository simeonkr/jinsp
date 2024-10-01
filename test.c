#include <stdio.h>
#include "json.h"
#include "parse.h"
#include "print.h"

void round_trip_test(FILE *f) {
    json_value j = parse(f);
    print(j);
}

int main() {
    round_trip_test(stdin);
}
