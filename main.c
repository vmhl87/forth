#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
	int32_t data;
	uint8_t type; // 0: token, 1: int
} symbol_t;

symbol_t ERR;

typedef struct {
	symbol_t *data;
	size_t capacity, size;
} symbol_vec;

symbol_vec new_symbol_vec() {
	symbol_vec ret;
	ret.size = 0;
	ret.capacity = 4;
	ret.data = malloc(sizeof(symbol_t) * ret.capacity);
}

void push_symbol_vec(symbol_vec* v, symbol_t s) {
	v->data[v->size] = s;
	++v->size;
	if (v->size == v->capacity) {
		v->capacity *= 2;
		v->data = realloc(v->data, sizeof(symbol_t) * v->capacity);
	}
}

void pop_symbol_vec(symbol_vec* v) {
	assert(v->size > 1);
	--v->size;
	if (v->size < v->capacity/4 && v->capacity > 4) {
		v->capacity /= 4;
		if (v->capacity < 4) v->capacity = 4;
		v->data = realloc(v->data, sizeof(symbol_t) * v->capacity);
	}
}

symbol_vec stack;

typedef struct {
	char *strings;
	size_t s_capacity, s_size;
	size_t *indices;
	size_t capacity, size;
} string_vec;

string_vec new_string_vec() {
	string_vec ret;
	ret.s_size = 0;
	ret.s_capacity = 4;
	ret.strings = malloc(sizeof(char) * ret.s_capacity);
	ret.size = 0;
	ret.capacity = 4;
	ret.indices = malloc(sizeof(size_t) * ret.capacity);
	ret.indices[0] = 0;
	return ret;
}

void push_string_vec(string_vec *v, char *s, size_t l) {
	++v->size;
	v->indices[v->size] = v->s_size + l;
	if (v->size+1 == v->capacity) {
		v->capacity *= 2;
		v->indices = realloc(v->indices, sizeof(size_t) * v->capacity);
	}
	size_t s_size = v->s_size + l;
	if (s_size > v->s_capacity) {
		while (v->s_capacity < s_size) v->s_capacity *= 2;
		v->strings = realloc(v->strings, sizeof(char) * v->s_capacity);
	}
	for (size_t i=0; i<l; ++i) v->strings[v->s_size+i] = s[i];
	v->s_size += l;
}

int32_t lookup_string_vec(string_vec *v, char *s, size_t l) {
	for (size_t i=0; i<v->size; ++i) {
		if (v->indices[i+1]-v->indices[i] == l) {
			bool match = true;
			for (size_t j=0; j<l; ++j) {
				if (v->strings[v->indices[i]+j] != s[j]) {
					match = false;
					break;
				}
			}
			if (match) return (int32_t) i;
		}
	}
	return -1;
}

string_vec symbols;

bool is_whitespace(char c) {
	if (c == ' ') return 1;
	if (c == '\n') return 1;
	if (c == '\t') return 1;
	return 0;
}

void show_symbol(int32_t i) {
	if (i < 0 || i >= stack.size) printf("<OOB>");
	else if (stack.data[i].type == 0) {
		int32_t fid = stack.data[i].data;
		if (fid < 0 || fid >= symbols.s_size) printf("<INV>");
		else {
			for (size_t j=symbols.indices[fid]; j<symbols.indices[fid+1]; ++j) {
				printf("%c", symbols.strings[j]);
			}
		}
	} else if (stack.data[i].type == 1) printf("%d", stack.data[i].data);
	else if (stack.data[i].type == -1) printf("<ERR>");
}

//			if (fid < PRIMITIVE_FLOOR) exec_primitive(fid);
//			else if (implementation_exists(fid)) exec(fid);

int32_t PRIMITIVE_FLOOR = 0;

void INIT_PRIMITIVES() {
	// arithmetic
	push_string_vec(&symbols, "+", 1);
	push_string_vec(&symbols, "-", 1);
	push_string_vec(&symbols, "*", 1);
	push_string_vec(&symbols, "/", 1);
	push_string_vec(&symbols, "%", 1);
	// bitwise
	push_string_vec(&symbols, "&", 1);
	push_string_vec(&symbols, "|", 1);
	push_string_vec(&symbols, "^", 1);
	push_string_vec(&symbols, "~", 1);
	// load
	// TODO: maybe add heap?
	push_string_vec(&symbols, "get", 3);
	// pop
	push_string_vec(&symbols, "pop", 3);
	// output
	push_string_vec(&symbols, "print", 5);
	push_string_vec(&symbols, "show", 4);
	// ---
	PRIMITIVE_FLOOR = symbols.s_size;
}

void exec_primitive(int32_t fid) {
	int32_t cmp = 0;

	// arithmetic
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 2) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data +
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			printf("ERR: operation '+' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 2) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data -
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			printf("ERR: operation '-' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 2) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data *
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			printf("ERR: operation '*' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 2) {
			if (stack.data[stack.size-2].data == 0) {
				printf("ERR: division by zero: ");
				show_symbol(stack.size-2);
				printf(" ");
				show_symbol(stack.size-1);
				printf("\n");
				return;
			}
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data /
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			printf("ERR: operation '/' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 2) {
			if (stack.data[stack.size-2].data == 0) {
				printf("ERR: modulo by zero: ");
				show_symbol(stack.size-2);
				printf(" ");
				show_symbol(stack.size-1);
				printf("\n");
				return;
			}
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data %
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			printf("ERR: operation '%%' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}

	// bitwise
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 2) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data &
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			printf("ERR: operation '&' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 2) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data |
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			printf("ERR: operation '|' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 2) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data ^
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			printf("ERR: operation '^' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 1 && stack.data[stack.size-1].type == 1) {
			symbol_t res;
			res.type = 1;
			res.data = ~stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			printf("ERR: operation '~' expects (int), received: ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}

	// get
	if (fid == cmp++) {
		if (stack.size >= 1 && stack.data[stack.size-1].type == 1) {
			int32_t index = stack.size-2 - stack.data[stack.size-1].data;
			if (index < 0) {
				printf("ERR: cannot get stack frame %d: out of bounds\n",
						stack.data[stack.size-1].data);
				return;
			}
			symbol_t res = stack.data[index];
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			printf("ERR: operation 'get' expects (int), received: ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}

	// pop
	if (fid == cmp++) {
		if (stack.size >= 1) {
			pop_symbol_vec(&stack);

		} else {
			printf("ERR: cannot pop from stack: execution stack empty\n");
		}
	}

	// output
	if (fid == cmp++) {
		if (stack.size >= 1 && stack.data[stack.size-1].type == 1) {
			printf("%c", stack.data[stack.size-1].data);

		} else {
			printf("ERR: operation 'print' expects (int), received: ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 1) {
			show_symbol(stack.size-1);

		} else {
			printf("ERR: operation 'show' expects (sym), received: ");
			show_symbol(stack.size-1);
			printf("\n");
		}
	}
}

// TODO: add function exec (after function compilation)

bool implementation_exists(int32_t fid) {
	return false;
}

void exec(int32_t fid) {
}

int main() {
	ERR.type = -1;

	stack = new_symbol_vec();
	symbols = new_string_vec();

	INIT_PRIMITIVES();

	while (1) {
		printf("> "); fflush(stdout);

		char *line;
		ssize_t bytes;
		if ((bytes = getline(&line, &bytes, stdin)) == -1) break;

		{
			// 1: str
			// 2: int+
			// 3: int-
			size_t state = 0;
			size_t start = 0;

			int int_buf = 0;

			for (size_t i=0; i<bytes; ++i) {
				if (is_whitespace(line[i])) {
					symbol_t res;

					if (state == 1) {
						// insert into vec, return id
						res.data = lookup_string_vec(&symbols,
								line+start, i-start);
						if (res.data == -1) {
							push_string_vec(&symbols, line+start, i-start);
							res.data = lookup_string_vec(&symbols,
									line+start, i-start);
						}
						res.type = 0;

					} else if (state == 2 || state == 3) {
						res.data = int_buf;
						res.type = 1;
					}

					push_symbol_vec(&stack, res);

					state = 0;
					start = i+1;

				} else if (line[0] == '{') {
					// TODO implement function compilation
					state = 0;
					start = i+1;

				} else if (line[0] == '}') {
					// TODO implement function compilation
					state = 0;
					start = -1;

				} else if (state == 0) {
					if (line[i] == '-') {
						int_buf = 0;
						state = 3;

					} else if (line[i] >= '0' && line[i] <= '9') {
						int_buf = line[i]-'0';
						state = 2;

					} else {
						state = 1;
					}
					
				} else if (state == 2) {
					int_buf = 10*int_buf + (line[i]-'0');

				} else if (state == 3) {
					int_buf = 10*int_buf - (line[i]-'0');
				}
			}

			if (bytes != 0) free(line);
		};

		while (stack.size && stack.data[stack.size-1].type == 0) {
			int32_t fid = stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);

			if (fid < PRIMITIVE_FLOOR) exec_primitive(fid);
			else if (implementation_exists(fid)) exec(fid);
			else {
				// TODO: throw less generic error
				printf("ERR: function not implemented: id(%d) sym(", fid);
				// show_symbol(fid);
				printf(")\n");
				break;
			}
		}

		if (stack.size != 0) {
			puts("[execution stack not empty]");
		}
	}

	puts("execution complete!");
}
