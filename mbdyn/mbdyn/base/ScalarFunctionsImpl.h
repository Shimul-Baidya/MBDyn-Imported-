/* 
 * HmFe (C) is a FEM analysis code. 
 *
 * Copyright (C) 1996-2003
 *
 * Marco Morandini  <morandini@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
/*
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2003
 * 
 * This code is a partial merge of HmFe and MBDyn.
 *
 * Pierangelo Masarati  <masarati@aero.polimi.it>
 * Paolo Mantegazza     <mantegazza@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 * 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef ScalarFunctionsImpl_hh
#define ScalarFunctionsImpl_hh

#include<vector>

#include "ScalarFunctions.h"



class ConstScalarFunction : public DifferentiableScalarFunction {
private:
	const doublereal y;
public:
	ConstScalarFunction(const doublereal v);
	virtual ~ConstScalarFunction();
	virtual doublereal operator()(const doublereal x) const;
	virtual doublereal ComputeDiff(const doublereal t, const unsigned int order = 1) const;
};

class LinearScalarFunction : public DifferentiableScalarFunction {
private:
	doublereal m;
	doublereal y0;
public:
	LinearScalarFunction(
		const doublereal y_i,
		const doublereal y_f,
		const doublereal t_i,
		const doublereal t_f);
	virtual ~LinearScalarFunction();
	virtual doublereal operator()(const doublereal x) const;
	virtual doublereal ComputeDiff(const doublereal t, const unsigned int order = 1) const;
};

class PowScalarFunction : public DifferentiableScalarFunction {
private:
	const doublereal pw;
public:
	PowScalarFunction(const doublereal p);
	virtual ~PowScalarFunction();
	virtual doublereal operator()(const doublereal x) const;
	virtual doublereal ComputeDiff(const doublereal t, const unsigned int order = 1) const;
};

class LogScalarFunction : public DifferentiableScalarFunction {
private:
	const doublereal mul_const;
public:
	LogScalarFunction(const doublereal ml);
	virtual ~LogScalarFunction();
	virtual doublereal operator()(const doublereal x) const;
	virtual doublereal ComputeDiff(const doublereal t, const unsigned int order = 1) const;
};

class SumScalarFunction : public DifferentiableScalarFunction {
private:
	const DifferentiableScalarFunction *const a1;
	const DifferentiableScalarFunction *const a2;
public:
	SumScalarFunction(
		const BasicScalarFunction *const b1,
		const BasicScalarFunction *const b2
	);
	virtual ~SumScalarFunction();
	virtual doublereal operator()(const doublereal x) const;
	virtual doublereal ComputeDiff(const doublereal t, const unsigned int order = 1) const;
};

class MulScalarFunction : public DifferentiableScalarFunction {
private:
	const DifferentiableScalarFunction *const a1;
	const DifferentiableScalarFunction *const a2;
public:
	MulScalarFunction(
		const BasicScalarFunction *const b1,
		const BasicScalarFunction *const b2
	);
	virtual ~MulScalarFunction();
	virtual doublereal operator()(const doublereal x) const;
	virtual doublereal ComputeDiff(const doublereal t, const unsigned int order = 1) const;
};

class CubicSplineScalarFunction : public DifferentiableScalarFunction {
private:
	std::vector<doublereal> Y_i;
	std::vector<doublereal> X_i;
	std::vector<doublereal> b, c, d;
public:
	CubicSplineScalarFunction(
		const std::vector<doublereal> y_i,
		const std::vector<doublereal> x_i);
	virtual ~CubicSplineScalarFunction();
	virtual doublereal operator()(const doublereal x) const;
	virtual doublereal ComputeDiff(const doublereal t, const unsigned int order = 1) const;
};

class MultiLinearScalarFunction : public DifferentiableScalarFunction {
private:
	std::vector<doublereal> Y_i;
	std::vector<doublereal> X_i;
public:
	MultiLinearScalarFunction(
		const std::vector<doublereal> y_i,
		const std::vector<doublereal> x_i);
	virtual ~MultiLinearScalarFunction();
	virtual doublereal operator()(const doublereal x) const;
	virtual doublereal ComputeDiff(const doublereal t, const unsigned int order = 1) const;
};




#endif //ScalarFunctionsImpl_hh
