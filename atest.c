#include <stdio.h>
#include <stdlib.h>
#include "attrib.h"

int
main() {
	struct attrib_e * ae = attrib_enew();
	const char * exp[] = {
		"-(-R1*(2+R3/(1.2-0.3)))",
		"R3^",
		"R3+R0",
	};
	int i;
	for (i=0;i<sizeof(exp)/sizeof(exp[0]);i++) {
		const char * err = attrib_epush(ae,i, exp[i]);
		if (err) {
			printf("%s : %s\n",exp[i], err);
			exit(1);
		} 
	}
	const char * err = attrib_einit(ae);
	if (err) {
		printf("%s\n",err);
		exit(1);
	} 
	attrib_edump(ae);


	struct attrib * a = attrib_new();

	attrib_write(a, 3 , 2.0f);

	attrib_attach(a, ae);
	for (i=0;i<=3;i++) {
		printf("R%d = %g\n",i,attrib_read(a,i));
	}

	attrib_delete(a);
	attrib_edelete(ae);
	return 0;
}
