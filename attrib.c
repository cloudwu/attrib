#include "attrib.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#define OPT_REG 0
#define OPT_ADD 1
#define OPT_SUB 2
#define OPT_MUL 3
#define OPT_DIV 4
#define OPT_NEG 5
#define OPT_SQR 6
#define OPT_END 7

static int opt_level[] = {
	-1,	// REG
	0,	// ADD
	0,	// SUB
	1,	// MUL
	1,	// DIV
	0,	// NEG 
	2,	// SQR
	-1, // END
};

#define MAX_LEVEL 8

#define OPT_SHIFT 8
#define OPT_MASK 0xff

union opt {
	float constant;
	uint32_t opt_reg;
};

struct attrib_expression {
	int reg;
	int depth;
	union opt stack[1];
};

struct attrib_e {
	int ecount;
	int ecap;
	struct attrib_expression ** exp;
	int rcount;
	int **depend;
};

struct attrib {
	struct attrib_e * exp;
	int cap;
	bool *dirty;
	float *reg;
	bool calc;
};

struct attrib *
attrib_new() {
	struct attrib * a = malloc(sizeof(*a));
	memset(a, 0, sizeof(*a));
	return a;
}

void 
attrib_delete(struct attrib *a) {
	free(a->dirty);
	free(a->reg);
	free(a);
}

void 
attrib_attach(struct attrib *a, struct attrib_e * e) {
	a->exp = e;
	int i;
	if (e->rcount > a->cap) {
		free(a->dirty);
		a->dirty = malloc(e->rcount * sizeof(bool));
		a->reg = realloc(a->reg, sizeof(float) * e->rcount);
		for (i=a->cap;i<e->rcount;i++) {
			a->reg[i] = 0;
		}
		a->cap = e->rcount;
	}
	for (i=0;i<e->rcount;i++) {
		a->dirty[i] = true;
	}
	a->calc = false;
}

float 
attrib_write(struct attrib *a, int r, float v) {
	assert(r>=0);
	float ret = 0;
	if (r>=a->cap) {
		a->dirty = realloc(a->dirty, sizeof(bool) * (r+1));
		a->reg = realloc(a->reg, sizeof(float) * (r+1));
		int i;
		for (i=a->cap;i<r;i++) {
			a->dirty[i] = false;
			a->reg[i] = 0;
		}
		a->dirty[i] = true;
		a->reg[i] = v;
		a->cap = r+1;
	} else {
		ret = a->reg[r];
		if (ret != v) {
			a->reg[r] = v;
			if (!a->dirty[r]) {
				a->dirty[r] = true;
				if (a->exp) {
					int i;
					int * depend = a->exp->depend[r];
					if (depend) {
						for (i=0;depend[i]>=0;i++) {
							a->dirty[depend[i]] = true;
						}
						a->calc = false;
					}
				}
			}
		}
	}
	return ret;
}

static void
_calc_exp(struct attrib_expression * exp, float *reg) {
	int max = exp->depth;
	float stack[max];
	int sp = 0;
	int i;
	for (i=0;exp->stack[i].opt_reg!=~OPT_END;i++) {
		uint32_t opt_reg = exp->stack[i].opt_reg;
		if ((int)opt_reg < 0) {
			opt_reg = ~opt_reg;
			int opt = opt_reg & OPT_MASK;
			switch(opt) {
			case OPT_REG:
				assert(sp<max);
				stack[sp++] = reg[opt_reg >> OPT_SHIFT];
				break;
			case OPT_ADD:
				assert(sp>=2);
				stack[sp-2] += stack[sp-1];
				--sp;
				break;
			case OPT_SUB:
				assert(sp>=2);
				stack[sp-2] -= stack[sp-1];
				--sp;
				break;
			case OPT_MUL:
				assert(sp>=2);
				stack[sp-2] *= stack[sp-1];
				--sp;
				break;
			case OPT_DIV:
				assert(sp>=2);
				stack[sp-2] /= stack[sp-1];
				--sp;
				break;
			case OPT_NEG:
				assert(sp>=1);
				stack[sp-1] = - stack[sp-1];
				break;
			case OPT_SQR:		
				assert(sp>=1);
				stack[sp-1] *= stack[sp-1];
				break;
			default:
				assert(0);
			}
		} else {
			assert(sp < max);
			stack[sp++] = exp->stack[i].constant;
		}
	}
	assert(sp == 1);
	reg[exp->reg] = stack[0];
}

static void
_calc(struct attrib *a) {
	struct attrib_e *e = a->exp;
	int i;
	for (i=0;i<e->ecount;i++) {
		struct attrib_expression * exp = e->exp[i];
		int reg = exp->reg;
		if (!a->dirty[reg]) {
			continue;
		}
		_calc_exp(exp, a->reg);
	}
}

float 
attrib_read(struct attrib *a, int r) {
	assert(r>=0);
	if (r>=a->cap)
		return 0;
	if (!a->calc) {
		if (a->exp) {
			_calc(a);
			memset(a->dirty, false, sizeof(bool) * a->cap);
			a->calc = true;
		}
	}
	return a->reg[r];
}

struct attrib_e * 
attrib_enew() {
	struct attrib_e * a = malloc(sizeof(struct attrib_e));
	memset(a, 0, sizeof(*a));
	return a;
}

void
attrib_edelete(struct attrib_e *a) {
	int i;
	for (i=0;i<a->ecount;i++) {
		free(a->exp[i]);
	}
	free(a->exp);
	for (i=0;i<a->rcount;i++) {
		free(a->depend[i]);
	}
	free(a->depend);
	free(a);
}

static int
_read_int(const char **exp) {
	const char * p = *exp;
	int n = 0;
	while (*p >= '0' && *p <='9') {
		n = n * 10 + (*p - '0');
		++p;
	}
	*exp = p;
	return n;
}

static float
_read_const(const char **exp) {
	const char * p = *exp;
	int n = 0;
	while (*p >= '0' && *p <='9') {
		n = n * 10 + (*p - '0');
		++p;
	}
	float d = 0;
	float dn = 0.1f;
	if (*p == '.') {
		++p;
		while (*p >= '0' && *p <='9') {
			d += dn * (*p - '0');
			dn *= 0.1f;
			++p;
		}
	}
	*exp = p;
	return d + n;
}

static int
_compile(const char * expression, int max, union opt * os, const char ** err, int *depth) {
	int ts[max];
	bool neg = true;
	int osp = 0;
	int tsp = 0;
	int base = 0;
	int n = 0;
	for (;;) {
		if (osp >= max || tsp >= max)
			return 0;
		char c = *expression;
		switch (c) {
		case 'R':
			++expression;
			c = *expression;
			if (c < '0' || c>'9') {
				*err = "Register syntax error";
				return -1;
			}
			os[osp++].opt_reg = ~(_read_int(&expression) << OPT_SHIFT | OPT_REG);
			++n;
			if (n>*depth) {
				*depth = n;
			}
			neg = false;
			break;
		case '0': case '1': case '2': case '3': case '4': case '5':
		case '6': case '7':	case '8': case '9': case '.': {
			float v = _read_const(&expression);
			++n;
			if (n>*depth) {
				*depth = n;
			}
			os[osp++].constant = v;
			neg = false;
			break;
		}
		case '(':
			base += MAX_LEVEL;
			neg = true;
			++expression;
			break;
		case ')':
			base -= MAX_LEVEL;
			if (base < 0) {
				*err = "Open bracket mismatch";
				return -1;
			}
			neg = false;
			++expression;
			break;
		case '\0': case '+': case '-': case '*': case '/': case '^': {
			++expression;
			int opt = 0;
			switch(c) {
			case '+':
				opt = OPT_ADD;
				neg = true;
				break;
			case '-':
				if (neg) {
					opt = OPT_NEG;
					neg = false;					
				} else {
					opt = OPT_SUB;
					neg = true;
				}
				break;
			case '*':
				opt = OPT_MUL;
				neg = true;
				break;
			case '/':
				opt = OPT_DIV;
				neg = true;
				break;
			case '^':
				opt = OPT_SQR;
				neg = true;
				break;
			case '\0':
				opt = OPT_END;
				if (base != 0) {
					*err = "Close bracket mismatch";
					return -1;
				}
				break;
			default:
				assert(0);
			}
			if (tsp == 0) {
				ts[tsp++] = base + opt;
				if (opt == OPT_END) {
					if (osp == 0) {
						*err = "Empty expression";
						return -1;
					}
					if (n != 1) { 
						*err = "Operator syntax error";
						return -1;
					}
					return osp;
				}
			} else {
				int level = base + opt_level[opt];
				for (;;) {
					int to = ts[tsp-1] % MAX_LEVEL;
					int tl = ts[tsp-1] - to + opt_level[to];
					if (level > tl) {
						ts[tsp++] = base + opt;
						break;
					} else {
						switch(to) {
						case OPT_ADD:
						case OPT_SUB:
						case OPT_MUL:
						case OPT_DIV:
							n--;
							break;
						case OPT_NEG:
						case OPT_SQR:
							break;
						default:
							assert(0);
						}
						os[osp++].opt_reg = ~to;
						--tsp;
						if (tsp == 0) {
							if (opt == OPT_END) {
								if (n != 1) { 
									*err = "Operator syntax error";
									return -1;
								}
								return osp;
							}
							ts[tsp++] = base + opt;
							break;
						} 
					}
				}
			}
			break;
		}
		default:
			*err = "Invalid charactor";
			return -1;
		}
	}
}

static struct attrib_expression *
attrib_compile(const char * expression, const char ** err) {
	int max = 64;
	int n;
	for (;;) {
		union opt os[max];
		int depth = 0;
		n = _compile(expression, max, os, err, &depth);
		if (n<0) {
			return NULL;
		}
		if (n>0) {
			struct attrib_expression * ae = malloc(sizeof(*ae) + sizeof(union opt) * n);
			ae->reg = -1;
			memcpy(ae->stack, os, sizeof(union opt) * n);
			ae->stack[n].opt_reg = ~OPT_END;
			ae->depth = depth;
			return ae;
		}
		max *= 2;
	}
}

const char *
attrib_epush(struct attrib_e * a, int r, const char * expression) {
	int i;
	for (i=0;i<a->ecount;i++) {
		if (a->exp[i]->reg == r) {
			return "Duplicate expression";
		}
	}
	const char * err;
	struct attrib_expression * ae = attrib_compile(expression, &err);
	if (ae == NULL) {
		return err;
	}
	ae->reg = r;
	if (a->ecount >= a->ecap) {
		a->ecap = (a->ecap + 1) * 2;
		a->exp = realloc(a->exp, a->ecap * sizeof(struct attrib_expression *));
	}
	a->exp[a->ecount++] = ae;

	return NULL;
}

static int
_topo_sort(struct attrib_expression ** exp, int n) {
	struct attrib_expression *tmp[n];
	int i,j;
	int max = 0;
	for (i=0;i<n;i++) {
		if (exp[i]->reg > max) {
			max = exp[i]->reg;
		}
		for (j=0;exp[i]->stack[j].opt_reg != ~OPT_END;j++) {
			uint32_t opt_reg = exp[i]->stack[j].opt_reg;
			if ((int)opt_reg < 0) {
				opt_reg = ~opt_reg;
				int reg = opt_reg >> OPT_SHIFT;
				if (reg >= max) {
					max = reg;
				}
			}
		}
	}
	++max;
	bool regi[max];
	memset(regi, 0, sizeof(regi));
	for (i=0;i<n;i++) {
		regi[exp[i]->reg] = true;
	}
	int count = n;
	int p = 0;
	int last_count = n;
	do {
		for (i=0;i<n;i++) {
			if (exp[i]) {
				for (j=0;exp[i]->stack[j].opt_reg != ~OPT_END;j++) {
					uint32_t opt_reg = exp[i]->stack[j].opt_reg;
					if ((int)opt_reg < 0) {
						opt_reg = ~opt_reg;
						int opt = opt_reg & OPT_MASK;
						int reg = opt_reg >> OPT_SHIFT;
						if (opt == OPT_REG) {
							if (regi[reg]) {
								goto _next;
							}
						}
					}
				}
				tmp[p++] = exp[i];
				regi[exp[i]->reg] = false;
				exp[i] = NULL;
				--count;
			}
		_next:
			continue;
		}
		if (count == last_count) {
			return -1;
		}
	} while (count > 0);
	memcpy(exp, tmp, n * sizeof(struct attrib_expression *));
	return max;
}

static void
_insert_depend(struct attrib_e *a, int r1, int r2) {
	int * d = a->depend[r1];
	if (d == NULL) {
		d = malloc(sizeof(int) * (a->rcount + 1));
		memset(d, -1, sizeof(int) * (a->rcount + 1));
		a->depend[r1] = d;
	}
	d[r2] = 0;
}

static void
_mark_depend(int ** depend, int * root, bool *mark) {
	int i;
	for (i=0;root[i]>=0;i++) {
		int idx = root[i];
		if (mark[idx])
			continue;
		mark[idx] = true;
		int *branch = depend[idx];
		if (branch) {
			_mark_depend(depend, branch, mark);
		}
	}
}

static void
_gen_depend(struct attrib_e *a) {
	int i,j;
	for (i=0;i<a->ecount;i++) {
		int r = a->exp[i]->reg;
		for (j=0;a->exp[i]->stack[j].opt_reg != ~OPT_END;j++) {
			uint32_t opt_reg = a->exp[i]->stack[j].opt_reg;
			if ((int)opt_reg < 0) {
				opt_reg = ~opt_reg;
				int opt = opt_reg & OPT_MASK;
				int reg = opt_reg >> OPT_SHIFT;
				if (opt == OPT_REG) {
					_insert_depend(a, reg, r);
				}
			}
		}
	}
	for (i=0;i<a->rcount;i++) {
		int * d = a->depend[i];
		if (d) {
			int p = 0;
			for (j=0;j<a->rcount;j++) {
				if (d[j] == 0) {
					d[p++] = j;
				}
			}
			d[p] = -1;
		}
	}
	for (i=0;i<a->rcount;i++) {
		bool mark[a->rcount];
		memset(mark, 0, sizeof(bool) * a->rcount);
		int * root = a->depend[i];
		if (root) {
			_mark_depend(a->depend, root, mark);
			int p = 0;
			for (j=0;j<a->rcount;j++) {
				if (mark[j]) {
					root[p++] = j;
				}
			}
			root[p] = -1;
		}
	}
}

const char *
attrib_einit(struct attrib_e * a) {
	int rcount = _topo_sort(a->exp, a->ecount);
	if (rcount < 0 ) {
		return "Detected circular reference in topo sort";
	}
	assert(a->rcount == 0);
	a->rcount = rcount;
	a->depend = malloc(rcount * sizeof(int *));
	memset(a->depend, 0, rcount * sizeof(int *));
	_gen_depend(a);

	return NULL;
}

#include <stdio.h>

static void
_dumpe(struct attrib_expression *ae) {
	int i=0;
	while (ae->stack[i].opt_reg != ~OPT_END) {
		uint32_t opt_reg = ae->stack[i].opt_reg;
		if ((int)opt_reg < 0) {
			opt_reg = ~opt_reg;
			int opt = opt_reg & OPT_MASK;
			int reg = opt_reg >> OPT_SHIFT;
			switch(opt) {
			case OPT_REG:
				printf("R%d ", reg);
				break;
			case OPT_ADD:
				printf("+ ");
				break;
			case OPT_SUB:
				printf("- ");
				break;
			case OPT_MUL:
				printf("* ");
				break;
			case OPT_DIV:
				printf("/ ");
				break;
			case OPT_NEG:
				printf("_ ");
				break;
			case OPT_SQR:
				printf("^ ");
			}
		} else {
			printf("%g ",ae->stack[i].constant);
		}
		++i;
	}
	printf("\n");
}

void
attrib_edump(struct attrib_e *a) {
	int i;
	for (i=0;i<a->ecount;i++) {
		printf("R%d = ",a->exp[i]->reg);
		_dumpe(a->exp[i]);
	}
	int j;
	for (i=0;i<a->rcount;i++) {
		int * d = a->depend[i];
		if (d) {
			printf("R%d : ", i);
			for (j=0;d[j]>=0;j++) {
				printf("R%d ", d[j]);
			}
			printf("\n");
		}
	}
}
