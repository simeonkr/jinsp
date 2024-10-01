#include <stdio.h>
#include "json.h"
#include "parse.h"
#include "print.h"

int main() {
    FILE *f = fopen("examples/simple.json", "r");
    json_value j = parse(f);
    print(j);
}