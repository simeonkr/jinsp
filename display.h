#include "json.h"
#include "str.h"
#include "term.h"

typedef struct {
	int top, left;
	int rows, cols;
	string *left_rows, *right_rows;
	int cur_row;
} panel;

void display_json(panel *p, const json_value json_value);

