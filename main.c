#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

void START_ERR_FMT() { printf("\x1b[2;33m"); }
void END_ERR_FMT() { printf("\x1b[0m"); }

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
	return ret;
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
	assert(v->size > 0);
	--v->size;
	if (v->size < v->capacity/4 && v->capacity > 4) {
		v->capacity /= 4;
		if (v->capacity < 4) v->capacity = 4;
		v->data = realloc(v->data, sizeof(symbol_t) * v->capacity);
	}
}

void reset_symbol_vec(symbol_vec* v) {
	free(v->data);
	v->size = 0;
	v->capacity = 4;
	v->data = malloc(sizeof(symbol_t) * v->capacity);
}

symbol_vec stack;

typedef struct {
	char *strings;
	size_t s_capacity, s_size;
	size_t *indices;
	symbol_vec *impl;
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
	ret.impl = malloc(sizeof(symbol_vec) * ret.capacity);
	return ret;
}

void push_string_vec(string_vec *v, char *s, size_t l) {
	v->impl[v->size] = new_symbol_vec();
	++v->size;
	v->indices[v->size] = v->s_size + l;
	if (v->size+1 == v->capacity) {
		v->capacity *= 2;
		v->indices = realloc(v->indices, sizeof(size_t) * v->capacity);
		v->impl = realloc(v->impl, sizeof(symbol_vec) * v->capacity);
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

typedef struct {
	int32_t *data;
	size_t size, capacity;
} int_vec;

int_vec new_int_vec() {
	int_vec res;
	res.size = 0;
	res.capacity = 4;
	res.data = malloc(sizeof(int32_t) * 4);
	return res;
}

void push_int_vec(int_vec *v, int32_t c) {
	v->data[v->size] = c;
	++v->size;
	if (v->size == v->capacity) {
		v->capacity *= 2;
		v->data = malloc(sizeof(int32_t) * v->capacity);
	}
}

void pop_int_vec(int_vec *v) {
	assert(v->size > 0);
	--v->size;
	if (v->size < v->capacity/4 && v->capacity > 4) {
		v->capacity /= 4;
		if (v->capacity < 4) v->capacity = 4;
		v->data = realloc(v->data, sizeof(int32_t) * v->capacity);
	}
}

int32_t top_int_vec(int_vec *v) {
	assert(v->size > 0);
	return v->data[v->size-1];
}

int_vec logic_stack;

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
		if (fid < 0 || fid >= symbols.size) printf("<INV>");
		else {
			for (size_t j=symbols.indices[fid]; j<symbols.indices[fid+1]; ++j) {
				printf("%c", symbols.strings[j]);
			}
		}
	} else if (stack.data[i].type == 1) printf("%d", stack.data[i].data);
	else if (stack.data[i].type == -1) printf("<ERR>");
}

int32_t PRIMITIVE_FLOOR = 0, CONDITIONAL_FLOOR = 0;

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
	// logic
	push_string_vec(&symbols, "<", 1);
	push_string_vec(&symbols, "==", 2);
	push_string_vec(&symbols, ">", 1);
	push_string_vec(&symbols, "!=", 2);
	// conditional
	CONDITIONAL_FLOOR = symbols.size;
	push_string_vec(&symbols, "if", 2);
	push_string_vec(&symbols, "else", 4);
	push_string_vec(&symbols, "then", 4);
	// ---
	PRIMITIVE_FLOOR = symbols.size;
}

void exec_primitive(int32_t fid) {
	int32_t cmp = 0;

	// arithmetic
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 1) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data +
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation '+' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 1) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data -
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation '-' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 1) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data *
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation '*' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 1) {
			if (stack.data[stack.size-2].data == 0) {
				START_ERR_FMT();
				printf(" [[ERR: division by zero: ");
				show_symbol(stack.size-2);
				printf(" ");
				show_symbol(stack.size-1);
				printf("]] ");
				END_ERR_FMT();
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
			START_ERR_FMT();
			printf(" [[ERR: operation '/' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 1) {
			if (stack.data[stack.size-2].data == 0) {
				START_ERR_FMT();
				printf(" [[ERR: modulo by zero: ");
				show_symbol(stack.size-2);
				printf(" ");
				show_symbol(stack.size-1);
				printf("]] ");
				END_ERR_FMT();
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
			START_ERR_FMT();
			printf(" [[ERR: operation '%%' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}

	// bitwise
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 1) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data &
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation '&' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 1) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data |
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation '|' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 1) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data ^
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation '^' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
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
			START_ERR_FMT();
			printf(" [[ERR: operation '~' expects (int), received: ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}

	// get
	if (fid == cmp++) {
		if (stack.size >= 1 && stack.data[stack.size-1].type == 1) {
			int32_t index = stack.size-2 - stack.data[stack.size-1].data;
			if (index < 0) {
				START_ERR_FMT();
				printf(" [[ERR: cannot get stack frame %d: out of bounds]] ",
						stack.data[stack.size-1].data);
				END_ERR_FMT();
				return;
			}
			symbol_t res = stack.data[index];
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			START_ERR_FMT();
			printf("ERR: operation 'get' expects (int), received: ");
			show_symbol(stack.size-1);
			printf("\n");
			END_ERR_FMT();
		}
	}

	// pop
	if (fid == cmp++) {
		if (stack.size >= 1) {
			pop_symbol_vec(&stack);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: cannot pop from stack: execution stack empty]] ");
			END_ERR_FMT();
		}
	}

	// output
	if (fid == cmp++) {
		if (stack.size >= 1 && stack.data[stack.size-1].type == 1) {
			printf("%c", stack.data[stack.size-1].data);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation 'print' expects (int), received: ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 1) {
			show_symbol(stack.size-1);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation 'show' expects (sym), received: ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}

	// logic
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 1) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data <
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation '<' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type ==
				stack.data[stack.size-2].type) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data ==
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation '==' expects (int, int) or (sym sym), "
					"received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type == 1 &&
				stack.data[stack.size-2].type == 1) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data >
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation '>' expects (int, int), received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (stack.size >= 2 && stack.data[stack.size-1].type ==
				stack.data[stack.size-2].type) {
			symbol_t res;
			res.type = 1;
			res.data = stack.data[stack.size-2].data !=
				stack.data[stack.size-1].data;
			pop_symbol_vec(&stack);
			pop_symbol_vec(&stack);
			push_symbol_vec(&stack, res);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation '!=' expects (int, int) or (sym sym), "
					"received: ");
			show_symbol(stack.size-2);
			printf(" ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (logic_stack.size > 0 && top_int_vec(&logic_stack) == 2) {
			push_int_vec(&logic_stack, 3);

		} else if (logic_stack.size > 0 && top_int_vec(&logic_stack) == 3) {
			push_int_vec(&logic_stack, 3);

		}else if (stack.size >= 1 && stack.data[stack.size-1].type == 1) {
			if (stack.data[stack.size-1].data) {
				pop_symbol_vec(&stack);
				push_int_vec(&logic_stack, 1);

			} else {
				pop_symbol_vec(&stack);
				push_int_vec(&logic_stack, 2);
			}

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation 'if' expects (int), received: ");
			show_symbol(stack.size-1);
			printf("]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (logic_stack.size > 0 && top_int_vec(&logic_stack) != 0) {
			int32_t state = top_int_vec(&logic_stack);
			if (state == 1) {
				pop_int_vec(&logic_stack);
				push_int_vec(&logic_stack, 2);

			} else if (state == 2) {
				pop_int_vec(&logic_stack);
				push_int_vec(&logic_stack, 1);
			}

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation 'else' expects nonzero logic state ]] ");
			END_ERR_FMT();
		}
	}
	if (fid == cmp++) {
		if (logic_stack.size > 0 && top_int_vec(&logic_stack) != 0) {
			pop_int_vec(&logic_stack);

		} else {
			START_ERR_FMT();
			printf(" [[ERR: operation 'then' expects nonzero logic state ]] ");
			END_ERR_FMT();
		}
	}
}

bool implementation_exists(int32_t fid) {
	if (fid < 0 || fid >= symbols.size) return false;
	return symbols.impl[fid].size > 0;
}

int exec_loop();

int exec(int32_t fid) {
	for (size_t i=0; i<symbols.impl[fid].size; ++i) {
		push_symbol_vec(&stack, symbols.impl[fid].data[i]);
		if (exec_loop()) return 1;
	}

	return 0;
}

// 0: execute
// 1: compile
// 2: just entered compile mode
int32_t exec_mode = 0;
int32_t compile_head = -1;

int exec_loop() {
	while (stack.size && stack.data[stack.size-1].type == 0) {
		int32_t fid = stack.data[stack.size-1].data;

		if (implementation_exists(fid)) {
			//printf(" [[impl exists!! %d]] ", fid);
			pop_symbol_vec(&stack);
			if (exec(fid)) return 1;

		} else if (fid < PRIMITIVE_FLOOR) {
			pop_symbol_vec(&stack);
			exec_primitive(fid);

		}else if (fid < 0 || fid >= symbols.size) {
			START_ERR_FMT();
			printf(" [[ERR: invalid function: id(%d)]] ", fid);
			END_ERR_FMT();

		} else {
			START_ERR_FMT();
			printf(" [[ERR: function not implemented: id(%d), sym(", fid);
			for (size_t j=symbols.indices[fid]; j<symbols.indices[fid+1]; ++j) {
				printf("%c", symbols.strings[j]);
			}
			printf(")]] ");
			END_ERR_FMT();
			return 1;
		}
	}

	return 0;
}

void process_symbol(symbol_t s) {
	if (exec_mode == 0) {
		if (logic_stack.size > 0) {
			int32_t state = top_int_vec(&logic_stack);
			if ((state == 2 || state == 3) && (s.type == 1 ||
						s.data < CONDITIONAL_FLOOR ||
						s.data >= CONDITIONAL_FLOOR + 3)) {
				return;
			}
		}

		push_symbol_vec(&stack, s);
		exec_loop();

	} else if(exec_mode == 1) {
		if (compile_head < 0 || compile_head >= symbols.size) {
			// TODO: decide how to fail
		} else {
			push_symbol_vec(symbols.impl+compile_head, s);
		}

	} else if(exec_mode == 2) {
		if (s.type == 1) {
			puts(" [[ ERR: cannot assign function body to non-symbol]] ");
			compile_head = -1;

		} else {
			compile_head = s.data;
			reset_symbol_vec(symbols.impl+compile_head);
		}

		exec_mode = 1;
	}
}

int main() {
	ERR.type = -1;

	stack = new_symbol_vec();
	symbols = new_string_vec();
	logic_stack = new_int_vec();
	push_int_vec(&logic_stack, 0);

	INIT_PRIMITIVES();

	while (1) {
		//printf("\x1b[2;31m%d\x1b[0m", top_int_vec(&logic_stack));
		printf("\x1b[2;37m%d >\x1b[0m ", stack.size); fflush(stdout);

		char *line = nullptr;
		size_t capacity;
		ssize_t bytes;
		if ((bytes = getline(&line, &capacity, stdin)) == -1) break;

		//printf("bytes: %d, last char: %d\n", bytes, line[bytes-1]);

		for (size_t i=0; i<bytes; ++i) if (line[i] == '[' || line[i] == ']')
			line[i] = ' ';

		{
			// 1: str
			// 2: int+
			// 3: int-
			size_t state = 0;
			size_t start = 0;

			int int_buf = 0;

			for (size_t i=0; i<bytes; ++i) {
				if (is_whitespace(line[i])) {
					if (state != 0) {
						symbol_t res;

						if (state == 1) {
							/*
							printf("<<processing string (");
							for (size_t j=start; j<i; ++j) printf("%c", line[j]);
							printf(")>>");
							*/
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
							//printf("<<processing int (%d)>>", int_buf);
							res.data = int_buf;
							res.type = 1;
						}

						//printf("<<exec mode: %d>>", exec_mode);
						process_symbol(res);
						//push_symbol_vec(&stack, res);
						//exec_loop();

					}

					state = 0;
					start = i+1;

				} else if (state == 0) {
					// switch to define mode
					if (line[i] == '{') {
						exec_mode = 2;
						
						state = 0;
						start = i+1;

					} else if (line[i] == '}') {
						exec_mode = 0;
						compile_head = -1;

						state = 0;
						start = i+1;

					} else if (line[i] == '-') {
						if (i+1 < bytes && !is_whitespace(line[i+1])) {
							int_buf = 0;
							state = 3;

						} else {
							state = 1;
						}

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

		//if (stack.size != 0) {
			//puts("[execution stack not empty]");
		//}
	}

	puts("execution complete!");
}
