/******************************************************************************
 * 
 * NASTRAN 2 MBDyn bulk data conversion tool
 * 
 * Copyright (c) 2000
 * 
 * Author:         Pierangelo Masarati <masarati@aero.polimi.it>
 * Affiliation:    Dipartimento di Ingegneria Aerospaziale
 *                 Politecnico di Milano
 *                 via La Masa 34, 20156 Milano (Italy)
 *                 http://www.aero.polimi.it
 * 
 * This program is part of the MBDyn package
 * http://www.mbdyn.org
 * https://mbdyn.aero.polimi.it/~masarati/mbdyn-index.html
 * 
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <n2m.h>

int
do_conm2_cont(struct n2m_buffer *b, FILE *fout, struct n2m_buffer *form, int def)
{
	double I11 = 0., I21 = 0., I22 = 0., I31 = 0., I32 = 0., I33 = 0.;

	if (def == 1) {
		fprintf(fout, "%s, null;\n", form->buf);
		return 0;
	}
		
	I11 = get_double(b, NASTRAN_SECOND, &I11);
        I21 = get_double(b, NASTRAN_THIRD, &I21);
        I22 = get_double(b, NASTRAN_FOURTH, &I22);
        I31 = get_double(b, NASTRAN_FIFTH, &I31);
        I32 = get_double(b, NASTRAN_SIXTH, &I32);
        I33 = get_double(b, NASTRAN_SEVENTH, &I33);
		
	fprintf(fout,
		"%s, sym,\n", form->buf);
	fprintf(fout,
		"\t                     %14.7e, %14.7e, %14.7e,\n",
		I11, I21, I31);
	fprintf(fout,
		"\t                                     %14.7e, %14.7e,\n",
		I22, I32);
	fprintf(fout,
		"\t                                                     %14.7e;\n",
		I33);
	
	return 0;
}

int
do_conm2(struct n2m_buffer *b, FILE *fout, struct n2m_buffer *form)
{
	int EID, G, CID = 0;
	double M, X1, X2, X3;
	
	EID = get_int(b, NASTRAN_SECOND, NULL);
        G = get_int(b, NASTRAN_THIRD, NULL);
	CID = get_int(b, NASTRAN_FOURTH, &CID);
		
	M = get_double(b, NASTRAN_FIFTH, NULL);

	X1 = get_double(b, NASTRAN_FIFTH, NULL);
	X2 = get_double(b, NASTRAN_SIXTH, NULL);
	X3 = get_double(b, NASTRAN_SEVENTH, NULL);
	
	fprintf(fout, "\t%14.7e, # CONM2=%d, GRID=%d\n", M, EID, G);
	if (CID == 0 || CID == -1) {
		snprintf(form->buf, sizeof(form->buf),
			"\treference,   global");
	} else {
		snprintf(form->buf, sizeof(form->buf),
			"\treference, %8d", CID);
	}
	fprintf(fout, "%s, %14.7e, %14.7e, %14.7e,\n",
		form->buf, X1, X2, X3);

	return 0;
}

