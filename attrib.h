#ifndef ATTRIB_CALC_H
#define ATTRIB_CALC_H

struct attrib;
struct attrib_e;

struct attrib_e * attrib_enew();
const char * attrib_epush(struct attrib_e * a, int r, const char * expression);
const char * attrib_einit(struct attrib_e * a);
void attrib_edelete(struct attrib_e *);
void attrib_edump(struct attrib_e *);

struct attrib *attrib_new();
void attrib_delete(struct attrib *);
void attrib_attach(struct attrib *, struct attrib_e *);
float attrib_write(struct attrib *, int r, float v);
float attrib_read(struct attrib *, int r);

#endif
