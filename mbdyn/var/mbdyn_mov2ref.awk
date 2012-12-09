# $Header$
#
# MBDyn (C) is a multibody analysis code. 
# http://www.mbdyn.org
# 
# Copyright (C) 1996-2012
# 
# Pierangelo Masarati	<masarati@aero.polimi.it>
# Paolo Mantegazza	<mantegazza@aero.polimi.it>
# 
# Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
# via La Masa, 34 - 20156 Milano, Italy
# http://www.aero.polimi.it
# 
# Changing this copyright notice is forbidden.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation (version 2 of the License).
# 
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# usage: awk -f mbdyn_mov2ref.awk [ -v labelsFile=LABELS_FILE ] [ -v reference=REF ] INPUT_FILE
#
# INPUT_FILE is a .mov file
# LABELS_FILE is a file containing pairs NODE_LABEL REFERENCE_LABEL
# REF is the name of the reference all generated references will be referred to

BEGIN {
	first_line = 1;
	first_record = 1;
	N = 0;

	if (labelsFile) {
		while (getline < labelsFile == 1) {
			mlabel[$1] = $2;
		}
	}
}

$1 == first_label && first_record == 1 {
	first_record = 0;
}

first_record == 1 {
	if (first_line == 1) {
		first_label = $1;
		first_line = 0;
	}
	N++;
	label[N] = $1;
}

{
	data[$1, "x1"] = $2;
	data[$1, "x2"] = $3;
	data[$1, "x3"] = $4;

	data[$1, "t1"] = $5;
	data[$1, "t2"] = $6;
	data[$1, "t3"] = $7;

	data[$1, "v1"] = $8;
	data[$1, "v2"] = $9;
	data[$1, "v3"] = $10;

	data[$1, "w1"] = $11;
	data[$1, "w2"] = $12;
	data[$1, "w3"] = $13;
}

END {
	print "# generated by " ENVIRON["USERNAME"] " from file \"" FILENAME "\" using mbdyn_mov2ref.awk on " strftime();
	print "";

	if (reference) {
		prefix = "\treference, " reference ", ";
	} else {
		prefix = "\t";
	}

	for (n = 1; n <= N; n++) {
		if (labelsFile) {
			l = mlabel[label[n]];
			if (l == "") {
				print "# no mapping available for node " label[n];
				l = label[n];
			} else {
				print "# node " label[n] " mapped to " l;
			}
		} else {
			l = label[n];
		}
		print "reference: " l ",";
		print prefix data[label[n], "x1"] ", " data[label[n], "x2"] ", " data[label[n], "x3"] ", "
		print prefix "vector, " data[label[n], "t1"] ", " data[label[n], "t2"] ", " data[label[n], "t3"] ", "
		print prefix data[label[n], "v1"] ", " data[label[n], "v2"] ", " data[label[n], "v3"] ", "
		print prefix data[label[n], "w1"] ", " data[label[n], "w2"] ", " data[label[n], "w3"] ";"
		print "";
	}
}
