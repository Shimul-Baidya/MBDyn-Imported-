/* $Header$ */
/*
 * MBDyn (C) is a multibody analysis code.
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2008
 *
 * Pierangelo Masarati	<masarati@aero.polimi.it>
 * Paolo Mantegazza	<mantegazza@aero.polimi.it>
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

/* joint */

#ifdef HAVE_CONFIG_H
#include "mbconfig.h"           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#include "Rot.hh"
#include "strings.h"

#include "joint.h"
#include "dataman.h"
#include "loadable.h"

#include "tpldrive.h"

#include "accj.h"      /* Vincoli di accelerazione imposta */
#include "beamslider.h"
#include "brake.h"
#include "distance.h"
#include "drvdisp.h"
#include "drvhinge.h"
#include "drvj.h"      /* Vincoli di velocita' imposta */
#include "genj.h"
#include "gimbal.h"
#include "impdisp.h"
#include "imporj.h"
#include "inplanej.h"  /* Vincoli di giacitura nel piano */
#include "inline.h"
#include "kinj.h"
#include "modal.h"
#if 0 /* No longer supported */
#include "planedj.h"
#endif
#include "planej.h"
#include "point_contact.h"
#include "prismj.h"    /* Vincolo prismatico */
#include "rodj.h"      /* Aste elastiche */
#include "spherj.h"
#include "totalequation.h"
#include "totalj.h"
#include "univj.h"
#include "vehj.h"      /* Giunti deformabili */
#include "vehj2.h"     /* "" */
#include "vehj3.h"     /* "" */

#define MBDYN_X_COMPATIBLE_INPUT

/* Joint - begin */

Joint::Joint(unsigned int uL, const DofOwner* pDO,
	flag fOut)
: Elem(uL, fOut),
ElemGravityOwner(uL, fOut),
ElemWithDofs(uL, pDO, fOut),
InitialAssemblyElem(uL, fOut)
{
	NO_OP;
}

Joint::~Joint(void)
{
	NO_OP;
}


/* Output specifico dei vincoli */
std::ostream&
Joint::Output(std::ostream& out, const char* /* sJointName */ ,
	unsigned int uLabel,
	const Vec3& FLocal, const Vec3& MLocal,
	const Vec3& FGlobal, const Vec3& MGlobal) const
{
#if 0
   /* Modificare le dimensioni del campo per il nome in base
    * ai nomi dei vincoli futuri */
   ASSERT(strlen(sJointName) <= 16);

   /* Nota: non c'e' *std::endl* perche' i vincoli possono aggiungere outut
    * ulteriore a quello comune a tutti */
   return out << sJointName << std::setw(16+8-strlen(sJointName)) << uLabel << " "
     << FLocal << " " << MLocal << " " << FGlobal << " " << MGlobal;
#endif

	return out
		<< std::setw(8) << uLabel
		<< " " << FLocal << " " << MLocal
		<< " " << FGlobal << " " << MGlobal;
}

/* Inverse Dynamics update */

void Joint::Update(const VectorHandler& XCurr, int iOrder)
{ 
	silent_cerr(psElemNames[GetElemType()] << "(" << GetLabel() << "): "
		"Elem::Update() for inverse dynamics not implemented yet" << std::endl);
};

/* Joint - end */


/* Legge un vincolo */

Elem *
ReadJoint(DataManager* pDM,
	MBDynParser& HP,
	const DofOwner* pDO,
	unsigned int uLabel)
{
	DEBUGCOUTFNAME("ReadJoint");

	const char* sKeyWords[] = {
		"distance",
		"distance" "with" "offset",
		"clamp",
		"coincidence",
		"spherical" "hinge",
		"pin",
		"spherical" "pin",
		"universal" "hinge",			// deprecated
		"universal" "rotation",			// deprecated
		"universal" "pin",			// deprecated
		"cardano" "pin",
		"cardano" "hinge",
		"cardano" "rotation",
		"plane" "hinge",
		"revolute" "hinge",
		"revolute" "rotation",
		"plane" "pin",
		"revolute" "pin",
		"axial" "rotation",
		"plane" "displacement",
		"plane" "displacement" "pin",
		"in" "plane",
		"in" "line",
		"rod",
		"rod" "with" "offset",
		"deformable" "hinge",
		"deformable" "displacement" "hinge",	// deprecated
		"deformable" "displacement" "joint",
		"deformable" "joint",
		"invariant" "deformable" "hinge",
		"invariant" "deformable" "displacement" "joint",
		"invariant" "deformable" "joint",
		"linear" "velocity",
		"angular" "velocity",
		"linear" "acceleration",
		"angular" "acceleration",
		"prismatic",
		"drive" "hinge",
		"drive" "displacement",
		"drive" "displacement" "pin",
		"imposed" "displacement",
		"imposed" "displacement" "pin",
		"imposed" "orientation",
		"total" "equation",
		"total" "internal" "reaction",
		"total" "joint",
		"total" "pin" "joint",
		"kinematic",
		"beam" "slider",
		"brake",
		"gimbal" "rotation",
		"modal",
		"point" "contact",

		NULL
	};

	/* enum delle parole chiave */
	enum KeyWords {
		UNKNOWN = -1,

		DISTANCE = 0,
		DISTANCEWITHOFFSET,
		CLAMP,
		COINCIDENCE,
		SPHERICALHINGE,
		PIN,
		SPHERICALPIN,
		UNIVERSALHINGE,			// deprecated
		UNIVERSALROTATION,		// deprecated
		UNIVERSALPIN,			// deprecated
		CARDANOHINGE,
		CARDANOROTATION,
		CARDANOPIN,
		PLANEHINGE,
		REVOLUTEHINGE,
		REVOLUTEROTATION,
		PLANEPIN,
		REVOLUTEPIN,
		AXIALROTATION,
		PLANEDISPLACEMENT,
		PLANEDISPLACEMENTPIN,
		INPLANE,
		J_INLINE,
		ROD,
		RODWITHOFFSET,
		DEFORMABLEHINGE,
		DEFORMABLEDISPHINGE,		// deprecated
		DEFORMABLEDISPJOINT,
		DEFORMABLEJOINT,
		INVARIANTDEFORMABLEHINGE,
		INVARIANTDEFORMABLEDISPJOINT,
		INVARIANTDEFORMABLEJOINT,
		LINEARVELOCITY,
		ANGULARVELOCITY,
		LINEARACCELERATION,
		ANGULARACCELERATION,
		PRISMATIC,
		DRIVEHINGE,
		DRIVEDISPLACEMENT,
		DRIVEDISPLACEMENTPIN,
		IMPOSEDDISPLACEMENT,
		IMPOSEDDISPLACEMENTPIN,
		IMPOSEDORIENTATION,
		TOTALEQUATION,
		TOTALINTERNALREACTION,
		TOTALJOINT,
		TOTALPINJOINT,
		KINEMATIC,
		BEAMSLIDER,
		BRAKE,
		GIMBALROTATION,
		MODAL,
		POINT_SURFACE_CONTACT,
		
		LASTKEYWORD
	};

	/* tabella delle parole chiave */
	KeyTable K(HP, sKeyWords);

	/* lettura del tipo di vincolo */
	KeyWords CurrKeyWord = KeyWords(HP.IsKeyWord());

#ifdef DEBUG
	if (CurrKeyWord >= 0) {
		std::cout << "joint type: " << sKeyWords[CurrKeyWord] << std::endl;
	}
#endif // DEBUG

	Elem* pEl = NULL;

	switch (CurrKeyWord) {

	/* vincolo di distanza */
	case DISTANCE:
		{
		/* lettura dei dati specifici */
		/* due nodi e tipo di drive, con dati specifici */

		bool bOffset(false);

		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		Vec3 f1(0.);
		if (HP.IsKeyWord("position")) {
			f1 = HP.GetPosRel(ReferenceFrame(pNode1));
			bOffset = true;
		}

		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		Vec3 f2(0.);
		if (HP.IsKeyWord("position")) {
			f2 = HP.GetPosRel(ReferenceFrame(pNode2));
			bOffset = true;
		}

		DriveCaller* pDC = NULL;
		if (HP.IsKeyWord("from" "nodes")) {
			doublereal l = (pNode2->GetXCurr()
				+ pNode2->GetRCurr()*f2
				- pNode1->GetXCurr()
				- pNode1->GetRCurr()*f1).Norm();

			pedantic_cout("Distance(" << uLabel << "): "
				"length from nodes = " << l << std::endl);

			SAFENEWWITHCONSTRUCTOR(pDC, ConstDriveCaller, ConstDriveCaller(l));
		} else {
			pDC = HP.GetDriveCaller();
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		/* allocazione e costruzione */
		if (bOffset) {
			SAFENEWWITHCONSTRUCTOR(pEl,
				DistanceJointWithOffset,
				DistanceJointWithOffset(uLabel, pDO,
					pNode1, pNode2, f1, f2, pDC, fOut));
		} else {
			SAFENEWWITHCONSTRUCTOR(pEl,
				DistanceJoint,
				DistanceJoint(uLabel, pDO,
					pNode1, pNode2, pDC, fOut));
		}

		std::ostream& out = pDM->GetLogFile();
		out << "distance: " << uLabel
			<< " " << pNode1->GetLabel()
			<< " ", f1.Write(out, " ")
			<< " " << pNode2->GetLabel()
			<< " ", f2.Write(out, " ")
			<< std::endl;

		} break;

	/* vincolo di distanza con offset */
	case DISTANCEWITHOFFSET:
		{
		/* lettura dei dati specifici */
		/* due nodi e tipo di drive, con dati specifici */

#ifdef MBDYN_X_COMPATIBLE_INPUT
		pedantic_cerr("Joint(" << uLabel << "): \"distance with offset\" "
			"is deprecated; use \"distance\" instead" << std::endl);
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		Vec3 f1(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
			       << HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			f1 = HP.GetPosRel(ReferenceFrame(pNode1));
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* !MBDYN_X_COMPATIBLE_INPUT */

		DEBUGCOUT("Offset 1: " << f1 << std::endl);


		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		Vec3 f2(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			f2 = HP.GetPosRel(ReferenceFrame(pNode2));
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* !MBDYN_X_COMPATIBLE_INPUT */

		DEBUGCOUT("Offset 2: " << f2 << std::endl);


		/* Legge e costruisce il drivecaller */
		if (!HP.IsArg()) {
			silent_cerr("line " << HP.GetLineData()
				<< ": driver data expected" << std::endl);
			throw DataManager::ErrGeneric();
		}

		DriveCaller* pDC = NULL;
		if (HP.IsKeyWord("from" "nodes")) {
			doublereal l = (pNode2->GetXCurr()
				- pNode1->GetXCurr()
				+ pNode2->GetRCurr()*f2
				- pNode1->GetRCurr()*f1).Norm();

			pedantic_cout("DistanceWithOffset(" << uLabel << "): "
				"length from nodes = " << l << std::endl);

			SAFENEWWITHCONSTRUCTOR(pDC, ConstDriveCaller, ConstDriveCaller(l));
		} else {
			pDC = HP.GetDriveCaller();
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		/* allocazione e costruzione */
		SAFENEWWITHCONSTRUCTOR(pEl,
			DistanceJointWithOffset,
			DistanceJointWithOffset(uLabel, pDO, pNode1, pNode2,
				f1, f2, pDC, fOut));

		std::ostream& out = pDM->GetLogFile();
		out << "distance: " << uLabel
			<< " " << pNode1->GetLabel()
			<< " ", f1.Write(out, " ")
			<< " " << pNode2->GetLabel()
			<< " ", f2.Write(out, " ")
			<< std::endl;

		/* scrittura dei dati specifici */
		} break;

	/* vincolo di incastro */
	case CLAMP:
		{
		/* lettura dei dati specifici */

		/* nodo collegato */
		StructNode* pNode = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		/* posizione (vettore di 3 elementi) */
		ReferenceFrame RF(pNode);
		/* stessa posizione del nodo */
		Vec3 X0(pNode->GetXCurr());
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			if (!HP.IsKeyWord("node")) {
				/* posizione arbitraria */
				X0 = HP.GetPosAbs(RF);
			}
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* !MBDYN_X_COMPATIBLE_INPUT */

		DEBUGCOUT("X0 =" << std::endl << X0 << std::endl);

		/* sistema di riferimento (trucco dei due vettori) */
		/* stessa giacitura del nodo */
		Mat3x3 R0(pNode->GetRCurr());
		if (HP.IsKeyWord("orientation")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"orientation\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			if (!HP.IsKeyWord("node")) {
				/* giacitura arbitraria */
				R0 = HP.GetRotAbs(RF);
			}
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* !MBDYN_X_COMPATIBLE_INPUT */

		DEBUGCOUT("R0 =" << std::endl << R0 << std::endl);

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		/* allocazione e costruzione */
		SAFENEWWITHCONSTRUCTOR(pEl,
			ClampJoint,
			ClampJoint(uLabel, pDO, pNode, X0, R0, fOut));
		std::ostream& out = pDM->GetLogFile();
		out << "clamp: " << uLabel
			<< " " << pNode->GetLabel()
			<< " " << Vec3(0.)
			<< " " << Mat3x3(1.)
			<< " " << pNode->GetLabel()
			<< " " << Vec3(0.)
			<< " " << Mat3x3(1.)
			<< std::endl;
		} break;

	case PIN:
	case SPHERICALPIN:
		{
		/* lettura dei dati specifici */

		/* nodo collegato */
		StructNode* pNode = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		ReferenceFrame RF(pNode);

		Vec3 d(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			d = HP.GetPosRel(RF);
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		/* currently unused */
		if (HP.IsKeyWord("orientation")) {
			(void)HP.GetRotRel(RF);
		}

		DEBUGCOUT("Node reference frame d:" << std::endl << d << std::endl);

		/* posizione (vettore di 3 elementi) */
		Vec3 X0(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			X0 = HP.GetPosAbs(AbsRefFrame);
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		/* currently unused */
		if (HP.IsKeyWord("orientation")) {
			(void)HP.GetRotAbs(AbsRefFrame);
		}


		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		/* allocazione e creazione */
		SAFENEWWITHCONSTRUCTOR(pEl,
			PinJoint,
			PinJoint(uLabel, pDO, pNode, X0, d, fOut));

		std::ostream& out = pDM->GetLogFile();
		out << "sphericalpin: " << uLabel
			<< " " << pNode->GetLabel()
			<< " " << d
			<< " " << Mat3x3(1.)
			<< " " << pNode->GetLabel()
			<< " " << d
			<< " " << Mat3x3(1.)
			<< std::endl;
		} break;

	/* vincolo di cerniera piana (PLANEHINGE)
	 * eventualmente con velocita' di rotazione imposta (AXIALROTATION) */
	case SPHERICALHINGE:
	case PLANEHINGE:
	case REVOLUTEHINGE:
	case BRAKE:
	case REVOLUTEROTATION:
	case UNIVERSALHINGE:
	case UNIVERSALROTATION:
	case CARDANOHINGE:
	case CARDANOROTATION:
	case AXIALROTATION:
	case GIMBALROTATION:
	case PLANEDISPLACEMENT:
		{
		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		ReferenceFrame RF(pNode1);
		Vec3 d1(0.);

#ifndef MBDYN_X_COMPATIBLE_INPUT
		if (HP.IsKeyWord("position")) {
			d1 = HP.GetPosRel(RF);
		}
#else /* MBDYN_X_COMPATIBLE_INPUT */
		switch (CurrKeyWord) {
		case REVOLUTEROTATION:
		case UNIVERSALROTATION:
		case CARDANOROTATION:
		case GIMBALROTATION:
			if (HP.IsKeyWord("position")) {
				/* currently ignored */
				(void)HP.GetPosRel(RF);
			} else {
				pedantic_cerr("Joint(" << uLabel << "): "
					"missing keyword \"position\" at line "
					<< HP.GetLineData() << std::endl);
			}
			break;

		default:
			if (!HP.IsKeyWord("position")) {
				pedantic_cerr("Joint(" << uLabel << "): "
					"missing keyword \"position\" at line "
					<< HP.GetLineData() << std::endl);
			}
			d1 = HP.GetPosRel(RF);
			DEBUGCOUT("Node 1 reference frame d1:" << std::endl
				<< d1 << std::endl);
			break;
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		Mat3x3 R1h(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R1h = HP.GetRotRel(RF);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R1h = HP.GetRotRel(RF);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}


		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		/* Stessa cosa per il nodo 2 */

		RF = ReferenceFrame(pNode2);
		Vec3 d2(0.);
#ifndef MBDYN_X_COMPATIBLE_INPUT
		if (HP.IsKeyWord("position")) {
			d2 = HP.GetPosRel(RF);
		}
#else /* MBDYN_X_COMPATIBLE_INPUT */
		switch (CurrKeyWord) {
		case REVOLUTEROTATION:
		case UNIVERSALROTATION:
		case CARDANOROTATION:
		case GIMBALROTATION:
			if (HP.IsKeyWord("position")) {
				/* currently ignored */
				(void)HP.GetPosRel(RF);
			} else {
				pedantic_cerr("Joint(" << uLabel << "): "
					"missing keyword \"position\" at line "
					<< HP.GetLineData() << std::endl);
			}
			break;

		default:
			if (!HP.IsKeyWord("position")) {
				pedantic_cerr("Joint(" << uLabel << "): "
					"missing keyword \"position\" at line "
					<< HP.GetLineData() << std::endl);
			}
			d2 = HP.GetPosRel(RF);
			DEBUGCOUT("Node 2 reference frame d2:" << std::endl
				<< d2 << std::endl);
			break;
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		Mat3x3 R2h(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R2h = HP.GetRotRel(RF);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R2h = HP.GetRotRel(RF);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}


		DriveCaller* pDC = NULL;
		if (CurrKeyWord == AXIALROTATION) {
			pDC = HP.GetDriveCaller();
		}

		OrientationDescription od = UNKNOWN_ORIENTATION_DESCRIPTION;
		switch (CurrKeyWord) {
		case GIMBALROTATION:
			od = ReadOptionalOrientationDescription(pDM, HP);
			break;

		default:
			break;
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		switch (CurrKeyWord) {

		/* allocazione e creazione cerniera sferica */
		case SPHERICALHINGE:
			{
			SAFENEWWITHCONSTRUCTOR(pEl,
				SphericalHingeJoint,
				SphericalHingeJoint(uLabel, pDO,
					pNode1, pNode2,
					d1, R1h, d2, R2h, fOut));
			std::ostream& out = pDM->GetLogFile();
			out << "sphericalhinge: " << uLabel
				<< " " << pNode1->GetLabel()
				<< " " << d1
				<< " " << R1h
				<< " " << pNode2->GetLabel()
				<< " " << d2
				<< " " << R2h
				<< std::endl;
			} break;

		/* allocazione e creazione cerniera piana */
		case PLANEHINGE:
			silent_cerr("line " << HP.GetLineData()
				<< ": deprecated \"plane hinge\" joint name;"
				<< " use \"revolute hinge\" instead" << std::endl);
		case REVOLUTEHINGE:
			{
			bool calcInitdTheta = true;
			doublereal initDTheta = 0.;
			if (HP.IsKeyWord("initial" "theta")) {
				initDTheta = HP.GetReal();
				calcInitdTheta = false;
			}
			doublereal r = 0.;
			doublereal preload = 0.;
			BasicFriction *bf = 0;
			BasicShapeCoefficient *bsh = 0;
			if (HP.IsKeyWord("friction")) {
				r = HP.GetReal();
				if (HP.IsKeyWord("preload")) {
					preload = HP.GetReal();
				}
				bf = ParseFriction(HP,pDM);
				bsh = ParseShapeCoefficient(HP);
			}
			SAFENEWWITHCONSTRUCTOR(pEl,
				PlaneHingeJoint,
				PlaneHingeJoint(uLabel, pDO, pNode1, pNode2,
					d1, d2, R1h, R2h, fOut,
					calcInitdTheta, initDTheta,
					r, preload, bsh, bf));
			std::ostream& out = pDM->GetLogFile();
			out << "revolutehinge: " << uLabel
				<< " " << pNode1->GetLabel()
				<< " " << d1
				<< " " << R1h
				<< " " << pNode2->GetLabel()
				<< " " << d2
				<< " " << R2h
				<< std::endl;
			} break;

		case BRAKE:
			{
			if (!HP.IsKeyWord("friction")) {
				silent_cerr("missing keyword \"friction\" at line "
					<< HP.GetLineData() << std::endl);
					throw ErrGeneric();
			}
			doublereal r = HP.GetReal();

			doublereal preload = 0.;
			if (HP.IsKeyWord("preload")) {
				preload = HP.GetReal();
			}

			BasicFriction * bf = ParseFriction(HP,pDM);
			BasicShapeCoefficient * bsh = ParseShapeCoefficient(HP);

#if 0
			// might be useful for a body sliding on a plane...
			bool isForce(false);
			Vec3 Dir(0.);
#endif

			DriveCaller *pDC = HP.GetDriveCaller();

			SAFENEWWITHCONSTRUCTOR(pEl,
				Brake,
				Brake(uLabel, pDO, pNode1, pNode2,
					d1, d2, R1h, R2h, fOut,
					r, preload, bsh, bf,
					/* isForce, Dir, */ pDC));
			} break;

		/* allocazione e creazione cerniera piana senza vincolo in pos. */
		case REVOLUTEROTATION:
			{
			SAFENEWWITHCONSTRUCTOR(pEl,
				PlaneRotationJoint,
				PlaneRotationJoint(uLabel, pDO,
					pNode1, pNode2, R1h, R2h, fOut));
			} break;

		/* allocazione e creazione cerniera universale */
		case UNIVERSALHINGE:
		case CARDANOHINGE:
			{
			SAFENEWWITHCONSTRUCTOR(pEl,
				UniversalHingeJoint,
				UniversalHingeJoint(uLabel, pDO,
					pNode1, pNode2,
					d1, d2, R1h, R2h, fOut));
			std::ostream& out = pDM->GetLogFile();
			out << "universalhinge: " << uLabel
				<< " " << pNode1->GetLabel()
				<< " " << d1
				<< " " << R1h
				<< " " << pNode2->GetLabel()
				<< " " << d2
				<< " " << R2h
				<< std::endl;
			} break;

		/* allocazione e creazione cerniera universale senza vincolo pos. */
		case UNIVERSALROTATION:
		case CARDANOROTATION:
			{
			SAFENEWWITHCONSTRUCTOR(pEl,
				UniversalRotationJoint,
				UniversalRotationJoint(uLabel, pDO,
					pNode1, pNode2, R1h, R2h, fOut));
			std::ostream& out = pDM->GetLogFile();
			out << "universalrotation: " << uLabel
				<< " " << pNode1->GetLabel()
				<< " " << Vec3(0.)
				<< " " << R1h
				<< " " << pNode2->GetLabel()
				<< " " << Vec3(0.)
				<< " " << R2h
				<< std::endl;
			} break;

		/* allocazione e creazione cerniera piana
		 * con velocita' di rotazione imposta */
		case AXIALROTATION:
			{
			doublereal r = 0.;
			doublereal preload = 0.;
			BasicFriction *bf = 0;
			BasicShapeCoefficient *bsh = 0;
			if (HP.IsKeyWord("friction")) {
				r = HP.GetReal();
				if (HP.IsKeyWord("preload")) {
					preload = HP.GetReal();
				}
				bf = ParseFriction(HP,pDM);
				bsh = ParseShapeCoefficient(HP);
			}
			SAFENEWWITHCONSTRUCTOR(pEl,
				AxialRotationJoint,
				AxialRotationJoint(uLabel, pDO,
					pNode1, pNode2,
					d1, d2, R1h, R2h, pDC,
					fOut,
					r, preload, bsh, bf));
			} break;

		/* allocazione e creazione vincolo gimbal */
		case GIMBALROTATION:
			{
			SAFENEWWITHCONSTRUCTOR(pEl,
				GimbalRotationJoint,
				GimbalRotationJoint(uLabel, pDO,
					pNode1, pNode2, R1h, R2h, od, fOut));
			} break;

		/* allocazione e creazione pattino */
		case PLANEDISPLACEMENT:
			silent_cerr("PlaneDispJoint(" << uLabel << "): "
				"unsupported; "
				"use an InPlane and a RevoluteRotation"
				<< std::endl);
			throw ErrGeneric();

		default:
			ASSERTMSG(0, "You shouldn't have reached this point");
			throw DataManager::ErrGeneric();
		}

		} break;

	case UNIVERSALPIN:
	case CARDANOPIN:
	case PLANEPIN:
	case REVOLUTEPIN:
	case PLANEDISPLACEMENTPIN:
		{
		/* nodo collegato */
		StructNode* pNode = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		ReferenceFrame RF(pNode);
		Vec3 d(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			d = HP.GetPosRel(RF);
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		DEBUGCOUT("Node reference frame d:" << std::endl << d << std::endl);

		Mat3x3 Rh(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			Rh = HP.GetRotRel(RF);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			Rh = HP.GetRotRel(RF);
			DEBUGCOUT("Hinge Rotation matrix Rh:" << std::endl << Rh << std::endl);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}


		Vec3 X0(0.);
#ifndef MBDYN_X_COMPATIBLE_INPUT
		if (HP.IsKeyWord("position"))
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		{
			X0 = HP.GetPosAbs(AbsRefFrame);
		}
		DEBUGCOUT("Absolute X:" << std::endl << X0 << std::endl);

		Mat3x3 R0(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R0 = HP.GetRotAbs(AbsRefFrame);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R0 = HP.GetRotAbs(AbsRefFrame);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}

		DEBUGCOUT("Absolute R:" << std::endl << R0 << std::endl);


		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		switch (CurrKeyWord) {

		/* allocazione e creazione cerniera piana */
		case PLANEPIN:
			silent_cerr("deprecated \"plane pin\" joint name;"
				<< " use \"revolute pin\" instead" << std::endl);
		case REVOLUTEPIN:
			{
			bool calcInitdTheta = true;
			doublereal initDTheta = 0.;
			if (HP.IsKeyWord("initial" "theta")) {
				initDTheta = HP.GetReal();
				calcInitdTheta = false;
			}
			SAFENEWWITHCONSTRUCTOR(pEl,
				PlanePinJoint,
				PlanePinJoint(uLabel, pDO, pNode,
					X0, R0, d, Rh, fOut,
					calcInitdTheta, initDTheta));
			} break;

		case UNIVERSALPIN:
		case CARDANOPIN:
			{
			SAFENEWWITHCONSTRUCTOR(pEl,
				UniversalPinJoint,
				UniversalPinJoint(uLabel, pDO, pNode,
					X0, R0, d, Rh, fOut));
			std::ostream& out = pDM->GetLogFile();
			out << "universalpin: " << uLabel
				<< " " << pNode->GetLabel()
				<< " " << d
				<< " " << Rh
				<< " " << pNode->GetLabel()
				<< " " << pNode->GetRCurr().Transpose()*(X0-pNode->GetXCurr())
				<< " " << R0*(pNode->GetRCurr().Transpose())
				<< std::endl;
			} break;

		/* allocazione e creazione cerniera piana */
		case PLANEDISPLACEMENTPIN:
			silent_cerr("PlaneDispJoint(" << uLabel << "): "
				"unsupported; "
				"use an \"inplane\" and a \"revolute rotation\" instead"
				<< std::endl);
			throw ErrGeneric();

		default:
			ASSERTMSG(0, "You shouldn't have reached this point");
			throw DataManager::ErrGeneric();
		}

		} break;

	case INPLANE:
		{
		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		ReferenceFrame RF(pNode1);
		Vec3 p(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			p = HP.GetPosRel(RF);
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		DEBUGCOUT("Node 1 reference frame p:" << std::endl << p << std::endl);

		Vec3 v(HP.GetVecRel(RF));
		doublereal d = v.Dot();
		if (d <= DBL_EPSILON) {
			silent_cerr("null direction at line " << HP.GetLineData()
				<< std::endl);
			throw DataManager::ErrGeneric();
		}
		v /= sqrt(d);


		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		Vec3 q(0.);
		bool bOffset(false);

		if (HP.IsArg()) {
			if (HP.IsKeyWord("offset")) {
				bOffset = true;
				q = HP.GetPosRel(ReferenceFrame(pNode2));
				DEBUGCOUT("Node 2 reference frame q:" << std::endl
					<< p << std::endl);
			}
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		if (bOffset) {
			SAFENEWWITHCONSTRUCTOR(pEl,
				InPlaneWithOffsetJoint,
				InPlaneWithOffsetJoint(uLabel, pDO,
					pNode1, pNode2, v, p, q, fOut));
		} else {
			SAFENEWWITHCONSTRUCTOR(pEl,
				InPlaneJoint,
				InPlaneJoint(uLabel, pDO,
					pNode1, pNode2, v, p, fOut));
		}
		std::ostream& out = pDM->GetLogFile();
		Vec3 relrot(Vec3(0., 0., 1.).Cross(v));
		relrot *= std::asin(relrot.Norm());
		out << "inplane: " << uLabel
			<< " " << pNode1->GetLabel()
			<< " " << p
			<< " " << (pNode1->GetRCurr().Transpose())*RotManip::Rot(relrot)
			<< " " << pNode2->GetLabel()
			<< " " << q
			<< " " << Mat3x3(1.)
			<< std::endl;

		} break;

	case J_INLINE:
		{
		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		ReferenceFrame RF(pNode1);
		Vec3 p(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			p = HP.GetPosRel(RF);
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		DEBUGCOUT("Node 1 reference frame p:" << std::endl << p << std::endl);

		Mat3x3 R(Eye3);
		if (HP.IsKeyWord("orientation")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"orientation\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			R = HP.GetRotRel(RF);
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */


		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		Vec3 q(0.);
		bool bOffset(false);

		if (HP.IsArg()) {
			if (HP.IsKeyWord("offset")) {
				bOffset = true;
				q = HP.GetPosRel(ReferenceFrame(pNode2));
				DEBUGCOUT("Node 2 reference frame q:" << std::endl << p << std::endl);
			}
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		if (bOffset) {
			SAFENEWWITHCONSTRUCTOR(pEl,
				InLineWithOffsetJoint,
				InLineWithOffsetJoint(uLabel, pDO,
					pNode1, pNode2, R, p, q, fOut));
		} else {
			SAFENEWWITHCONSTRUCTOR(pEl,
				InLineJoint,
				InLineJoint(uLabel, pDO,
					pNode1, pNode2, R, p, fOut));
		}
		std::ostream& out = pDM->GetLogFile();
		out << "inline: " << uLabel
			<< " " << pNode1->GetLabel()
			<< " " << p
			<< " " << R
			<< " " << pNode2->GetLabel()
			<< " " << q
			<< " " << Mat3x3(1.)
			<< std::endl;

		} break;

	/* asta con pin alle estremita' */
	case ROD:
		{

		/*
		 * lettura dei dati specifici:
		 * due nodi, lunghezza iniziale e tipo di legame elastico
		 */

		bool bOffset(false);

		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		DEBUGCOUT("Linked to Node " << pNode1->GetLabel() << std::endl);

		Vec3 f1(0.);
		if (HP.IsKeyWord("position")) {
			f1 = HP.GetPosRel(ReferenceFrame(pNode1));
			bOffset = true;
		}

		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		DEBUGCOUT("Linked to Node " << pNode2->GetLabel() << std::endl);

		Vec3 f2(0.);
		if (HP.IsKeyWord("position")) {
			f2 = HP.GetPosRel(ReferenceFrame(pNode2));
			bOffset = true;
		}

		/* Lunghezza iniziale */
		doublereal dL = 0.;
		bool bFromNodes(false);
		if (HP.IsKeyWord("from" "nodes")) {
			bFromNodes = true;
			DEBUGCOUT("Initial length will be computed from nodes position" << std::endl);
		} else {
			dL = HP.GetReal();
			DEBUGCOUT("Initial length = " << dL << std::endl);
		}

		/* Se si tratta di Rod con Offset, legge gli offset e poi passa
		 * al tipo di legame costitutivo */
#ifdef MBDYN_X_COMPATIBLE_INPUT
		if (HP.IsKeyWord("offset")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"offset\" at line " << HP.GetLineData()
				<< " is deprecated; use \"position\" instead"
				<< std::endl);
			if (bOffset) {
				silent_cerr("Joint(" << uLabel << "): "
					"offsets already defined "
					"at line " << HP.GetLineData()
					<< std::endl);
				throw ErrGeneric();
			}

			bOffset = true;
			f1 = HP.GetPosRel(ReferenceFrame(pNode1));
			DEBUGCOUT("Offset 1: " << f1 << std::endl);

			f2 = HP.GetPosRel(ReferenceFrame(pNode2));
			DEBUGCOUT("Offset 2: " << f2 << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		if (bFromNodes) {
			Vec3 v = pNode2->GetXCurr()-pNode1->GetXCurr();
			if (bOffset) {
				v += pNode2->GetRCurr()*f2-pNode1->GetRCurr()*f1;
			}
			dL = v.Norm();

			if (bOffset) {
				pedantic_cout("RodWithOffset(" << uLabel << "): "
					"length from nodes = " << dL << std::endl);
			} else {
				pedantic_cout("Rod(" << uLabel << "): "
					"length from nodes = " << dL << std::endl);
			}

			DEBUGCOUT("Initial length = " << dL << std::endl);
		}

		/* Legame costitutivo */
		ConstLawType::Type CLType = ConstLawType::UNKNOWN;
		ConstitutiveLaw1D* pCL = HP.GetConstLaw1D(CLType);

		if (pCL->iGetNumDof() != 0) {
			silent_cerr("line " << HP.GetLineData() << ": "
				"rod joint does not support "
				"dynamic constitutive laws yet"
				<< std::endl);
			throw ErrGeneric();
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		std::ostream& out = pDM->GetLogFile();
		out << "rod: " << uLabel
			<< " " << pNode1->GetLabel()
			<< " ", f1.Write(out, " ")
			<< " " << pNode2->GetLabel()
			<< " ", f2.Write(out, " ")
			<< std::endl;

		if (bOffset) {
			SAFENEWWITHCONSTRUCTOR(pEl,
				RodWithOffset,
				RodWithOffset(uLabel, pDO, pCL,
					pNode1, pNode2, f1, f2, dL, fOut));
		} else {
			switch (CLType) {
			case ConstLawType::VISCOUS:
			case ConstLawType::VISCOELASTIC:
				SAFENEWWITHCONSTRUCTOR(pEl,
					ViscoElasticRod,
					ViscoElasticRod(uLabel, pDO, pCL,
						pNode1, pNode2, dL, fOut));
				break;

			default:
				SAFENEWWITHCONSTRUCTOR(pEl,
					Rod,
					Rod(uLabel, pDO, pCL,
						pNode1, pNode2, dL, fOut));
				break;
			}
		}

		} break;

	case RODWITHOFFSET:
		{

		/*
		 * lettura dei dati specifici:
		 * due nodi, lunghezza iniziale e tipo di legame elastico.
		 * Nota: prima gli offset erano un optional dei dati del rod normale;
		 * questo e' lo stesso rod with offset, ma con un input piu' simile
		 * a quello standard.
		 */

#ifdef MBDYN_X_COMPATIBLE_INPUT
		pedantic_cerr("Joint(" << uLabel << "): \"rod with offset\" "
			"is deprecated; use \"rod\" instead" << std::endl);
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		Vec3 f1(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			f1 = HP.GetPosRel(ReferenceFrame(pNode1));
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		DEBUGCOUT("Linked to Node " << pNode1->GetLabel()
			<< "with offset " << f1 << std::endl);


		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		Vec3 f2(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			f2 = HP.GetPosRel(ReferenceFrame(pNode2));
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		DEBUGCOUT("Linked to Node " << pNode2->GetLabel()
			<< "with offset " << f2 << std::endl);

		/* Lunghezza iniziale */
		doublereal dL = 0.;
		if (HP.IsKeyWord("from" "nodes")) {
			dL = (pNode2->GetXCurr()
				+ pNode2->GetRCurr()*f2
				- pNode1->GetXCurr()
				- pNode1->GetRCurr()*f1).Norm();

			pedantic_cout("RodWithOffset(" << uLabel << "): "
				"length from nodes = " << dL << std::endl);

		} else {
			dL = HP.GetReal();
		}
		DEBUGCOUT("Initial length = " << dL << std::endl);

		/* Legame costitutivo */
		ConstLawType::Type CLType = ConstLawType::UNKNOWN;
		ConstitutiveLaw1D* pCL = HP.GetConstLaw1D(CLType);

		if (pCL->iGetNumDof() != 0) {
			silent_cerr("line " << HP.GetLineData() << ": "
				"\"rod with offset\" joint does not support "
				"dynamic constitutive laws yet"
				<< std::endl);
			throw ErrGeneric();
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		std::ostream& out = pDM->GetLogFile();
		out << "rod: " << uLabel
			<< " " << pNode1->GetLabel()
			<< " ", f1.Write(out, " ")
			<< " " << pNode2->GetLabel()
			<< " ", f2.Write(out, " ")
			<< std::endl;

		SAFENEWWITHCONSTRUCTOR(pEl,
			RodWithOffset,
			RodWithOffset(uLabel, pDO, pCL,
				pNode1, pNode2, f1, f2, dL, fOut));

		} break;

	case DEFORMABLEHINGE:
	case DEFORMABLEDISPHINGE:
	case DEFORMABLEDISPJOINT:
	case INVARIANTDEFORMABLEHINGE:
	case INVARIANTDEFORMABLEDISPJOINT:
		{
		/* lettura dei dati specifici */

		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		ReferenceFrame RF(pNode1);

		/* Offset if displacement hinge */
		Vec3 f1(Zero3);
		switch (CurrKeyWord) {
		case DEFORMABLEDISPHINGE:
		case DEFORMABLEDISPJOINT:
		case INVARIANTDEFORMABLEDISPJOINT:
			if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
				NO_OP;
			} else {
				pedantic_cerr("Joint(" << uLabel << "): "
					"missing keyword \"position\" at line "
					<< HP.GetLineData() << std::endl);
			}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
				f1 = HP.GetPosRel(RF);
#ifndef MBDYN_X_COMPATIBLE_INPUT
			}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			break;

		default:
			break;
		}

		Mat3x3 R1(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R1 = HP.GetRotRel(RF);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R1 = HP.GetRotRel(RF);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}


		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		RF = ReferenceFrame(pNode2);

		/* Offset if displacement hinge */
		Vec3 f2(Zero3);
		switch (CurrKeyWord) {
		case DEFORMABLEDISPHINGE:
		case DEFORMABLEDISPJOINT:
		case INVARIANTDEFORMABLEDISPJOINT:
			if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
				NO_OP;
			} else {
				pedantic_cerr("Joint(" << uLabel << "): "
					"missing keyword \"position\" at line "
					<< HP.GetLineData() << std::endl);
			}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
				f2 = HP.GetPosRel(ReferenceFrame(pNode2));
#ifndef MBDYN_X_COMPATIBLE_INPUT
			}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			break;

		default:
			break;
		}

		Mat3x3 R2(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R2 = HP.GetRotRel(RF);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R2 = HP.GetRotRel(RF);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}


		/* Legame costitutivo */
		ConstLawType::Type CLType;
		ConstitutiveLaw3D* pCL = HP.GetConstLaw3D(CLType);

		if (pCL->iGetNumDof() != 0) {
			silent_cerr("line " << HP.GetLineData() << ": "
				"\"deformable hinge\" joint does not support "
				"dynamic constitutive laws yet"
				<< std::endl);
			throw ErrGeneric();
		}

		OrientationDescription od = UNKNOWN_ORIENTATION_DESCRIPTION;
		switch (CurrKeyWord) {
		case DEFORMABLEHINGE:
		case INVARIANTDEFORMABLEHINGE:
			od = ReadOptionalOrientationDescription(pDM, HP);
			break;

		default:
			break;
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		const char *sJointLogName = 0;

		switch (CLType) {
		case ConstLawType::ELASTIC:
			switch (CurrKeyWord) {
			case DEFORMABLEHINGE:
				SAFENEWWITHCONSTRUCTOR(pEl,
					ElasticHingeJoint,
					ElasticHingeJoint(uLabel, pDO, pCL,
						pNode1, pNode2,
						R1, R2, od, fOut));
				sJointLogName = "deformablehinge";
				break;

			case INVARIANTDEFORMABLEHINGE:
				SAFENEWWITHCONSTRUCTOR(pEl,
					ElasticHingeJointInv,
					ElasticHingeJointInv(uLabel, pDO, pCL,
						pNode1, pNode2,
						R1, R2, od, fOut));
				sJointLogName = "deformablehinge";
				break;

			case DEFORMABLEDISPHINGE:
			case DEFORMABLEDISPJOINT:
				SAFENEWWITHCONSTRUCTOR(pEl,
					ElasticDispJoint,
					ElasticDispJoint(uLabel, pDO, pCL,
						pNode1, pNode2,
						f1, f2, R1, R2, fOut));

				sJointLogName = "deformabledisplacementjoint";
				break;

			case INVARIANTDEFORMABLEDISPJOINT:
#if 0
				silent_cerr("\"invariant deformable displacement joint\" "
					"at line " << HP.GetLineData()
					<< " not implemented yet" << std::endl);
				throw ErrGeneric();
#endif
#if 1
				SAFENEWWITHCONSTRUCTOR(pEl,
					ElasticDispJointInv,
					ElasticDispJointInv(uLabel, pDO, pCL,
						pNode1, pNode2,
						f1, f2, R1, R2, fOut));

				sJointLogName = "deformabledisplacementjoint";
#endif
				break;
			default:
				ASSERTMSG(0, "You shouldn't have reached this point");
				throw DataManager::ErrGeneric();
				break;
			}
			break;

		case ConstLawType::VISCOUS:
			switch (CurrKeyWord) {
			case DEFORMABLEHINGE:
				SAFENEWWITHCONSTRUCTOR(pEl,
					ViscousHingeJoint,
					ViscousHingeJoint(uLabel, pDO, pCL,
						pNode1, pNode2,
						R1, R2, od, fOut));
				sJointLogName = "deformablehinge";
				break;

			case INVARIANTDEFORMABLEHINGE:
				SAFENEWWITHCONSTRUCTOR(pEl,
					ViscousHingeJointInv,
					ViscousHingeJointInv(uLabel, pDO, pCL,
						pNode1, pNode2,
						R1, R2, od, fOut));
				sJointLogName = "deformablehinge";
				break;

			case DEFORMABLEDISPHINGE:
			case DEFORMABLEDISPJOINT:
				SAFENEWWITHCONSTRUCTOR(pEl,
					ViscousDispJoint,
					ViscousDispJoint(uLabel, pDO, pCL,
						pNode1, pNode2,
						f1, f2, R1, R2, fOut));
				sJointLogName = "deformabledisplacementjoint";
				break;

			case INVARIANTDEFORMABLEDISPJOINT:
				silent_cerr("\"invariant deformable displacement joint\" "
					"at line " << HP.GetLineData()
					<< " not implemented yet" << std::endl);
				throw ErrGeneric();
#if 0
				SAFENEWWITHCONSTRUCTOR(pEl,
					ViscousDispJoint,
					ViscousDispJoint(uLabel, pDO, pCL,
						pNode1, pNode2,
						f1, f2, R1, R2, fOut));
				sJointLogName = "deformabledisplacementjoint";
#endif
				break;
			default:
				ASSERTMSG(0, "You shouldn't have reached this point");
				throw DataManager::ErrGeneric();
				break;
			}
			break;

		case ConstLawType::VISCOELASTIC:
			switch (CurrKeyWord) {
			case DEFORMABLEHINGE:
				SAFENEWWITHCONSTRUCTOR(pEl,
					ViscoElasticHingeJoint,
					ViscoElasticHingeJoint(uLabel, pDO, pCL,
						pNode1, pNode2,
						R1, R2, od, fOut));
				sJointLogName = "deformablehinge";
				break;

			case INVARIANTDEFORMABLEHINGE:
				SAFENEWWITHCONSTRUCTOR(pEl,
					ViscoElasticHingeJointInv,
					ViscoElasticHingeJointInv(uLabel, pDO, pCL,
						pNode1, pNode2,
						R1, R2, od, fOut));
				sJointLogName = "deformablehinge";
				break;

			case DEFORMABLEDISPHINGE:
			case DEFORMABLEDISPJOINT:
				SAFENEWWITHCONSTRUCTOR(pEl,
					ViscoElasticDispJoint,
					ViscoElasticDispJoint(uLabel,
						pDO, pCL,
						pNode1, pNode2,
						f1, f2, R1, R2, fOut));
				sJointLogName = "deformabledisplacementjoint";
				break;

			case INVARIANTDEFORMABLEDISPJOINT:
				silent_cerr("\"invariant deformable displacement joint\" "
					"at line " << HP.GetLineData()
					<< " not implemented yet" << std::endl);
				throw ErrGeneric();
#if 0
				SAFENEWWITHCONSTRUCTOR(pEl,
					ViscoElasticDispJointInv,
					ViscoElasticDispJointInv(uLabel,
						pDO, pCL,
						pNode1, pNode2,
						f1, f2, R1, R2, fOut));
				sJointLogName = "deformabledisplacementjoint";
#endif
				break;
			default:
				ASSERTMSG(0, "You shouldn't have reached this point");
				throw DataManager::ErrGeneric();
				break;
			}
			break;

		default:
			ASSERTMSG(0, "You shouldn't have reached this point");
			throw DataManager::ErrGeneric();
		}

		std::ostream& out = pDM->GetLogFile();
		out << sJointLogName << ": " << uLabel
			<< " " << pNode1->GetLabel()
			<< " " << f1
			<< " " << R1
			<< " " << pNode2->GetLabel()
			<< " " << f2
			<< " " << R2
			<< std::endl;
		} break;

	case DEFORMABLEJOINT:
	case INVARIANTDEFORMABLEJOINT:
		{
		/* lettura dei dati specifici */

		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		/* Offset if displacement hinge */
		Vec3 f1(HP.GetPosRel(ReferenceFrame(pNode1)));

		Mat3x3 R1(Eye3);
		if (HP.IsKeyWord("hinge")) {
			R1 = HP.GetRotRel(ReferenceFrame(pNode1));
		}

		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		/* Offset */
		Vec3 f2(HP.GetPosRel(ReferenceFrame(pNode2)));

		Mat3x3 R2(Eye3);
		if (HP.IsKeyWord("hinge")) {
			R2 = HP.GetRotRel(ReferenceFrame(pNode2));
		}

		/* Legame costitutivo */
		ConstLawType::Type CLType;
		ConstitutiveLaw6D* pCL = HP.GetConstLaw6D(CLType);

		if (pCL->iGetNumDof() != 0) {
			silent_cerr("line " << HP.GetLineData() << ": "
				"\"deformable joint\" does not support "
				"dynamic constitutive laws yet"
				<< std::endl);
			throw ErrGeneric();
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		switch (CLType) {
		case ConstLawType::ELASTIC:
			switch (CurrKeyWord) {
			case DEFORMABLEJOINT:
				SAFENEWWITHCONSTRUCTOR(pEl,
					ElasticJoint,
					ElasticJoint(uLabel, pDO, pCL,
						pNode1, pNode2,
						f1, f2, R1, R2, fOut));
				break;

			case INVARIANTDEFORMABLEJOINT:
				silent_cerr("\"invariant deformable joint\" "
					"at line " << HP.GetLineData()
					<< " not implemented yet" << std::endl);
				throw ErrGeneric();
#if 0
				SAFENEWWITHCONSTRUCTOR(pEl,
					ElasticJointInv,
					ElasticJointInv(uLabel, pDO, pCL,
						pNode1, pNode2,
						f1, f2, R1, R2, fOut));
#endif
				break;
			default:
				ASSERTMSG(0, "You shouldn't have reached this point");
				throw DataManager::ErrGeneric();
				break;
			}

			break;

#if 0
		/* not implemented yet */
		case ConstLawType::VISCOUS:
			SAFENEWWITHCONSTRUCTOR(pEl,
				ViscousJoint,
				ViscousJoint(uLabel, pDO, pCL,
					pNode1, pNode2,
					f1, f2, R1, R2,
					fOut));
			break;

		case ConstLawType::VISCOELASTIC:
			SAFENEWWITHCONSTRUCTOR(pEl,
				ViscoElasticJoint,
				ViscoElasticJoint(uLabel, pDO, pCL,
					pNode1, pNode2,
					f1, f2, R1, R2,
					fOut));
			break;
#endif

		default:
			ASSERTMSG(0, "You shouldn't have reached this point");
			throw DataManager::ErrGeneric();
		}

		std::ostream& out = pDM->GetLogFile();
		out << "deformablejoint: " << uLabel
			<< " " << pNode1->GetLabel()
			<< " " << f1
			<< " " << R1
			<< " " << pNode2->GetLabel()
			<< " " << f2
			<< " " << R2
			<< std::endl;

		} break;

	case LINEARVELOCITY:
	case ANGULARVELOCITY:
		{
		/* nodo collegato */
		StructNode* pNode = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		Vec3 Dir(HP.GetVecRel(ReferenceFrame(pNode)));
		doublereal d = Dir.Dot();
		ASSERT(d > 0.);
		if (d > 0.) {
			Dir /= sqrt(d);
		} else {
			silent_cerr("direction is null" << std::endl);
			throw ErrGeneric();
		}

		DriveCaller* pDC = HP.GetDriveCaller();

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		switch (CurrKeyWord) {
		case LINEARVELOCITY:
			SAFENEWWITHCONSTRUCTOR(pEl,
				LinearVelocityJoint,
				LinearVelocityJoint(uLabel, pDO,
					pNode, Dir, pDC, fOut));
			break;

		case ANGULARVELOCITY:
			SAFENEWWITHCONSTRUCTOR(pEl,
				AngularVelocityJoint,
				AngularVelocityJoint(uLabel, pDO,
					pNode, Dir, pDC, fOut));
			break;

		default:
			ASSERTMSG(0, "You shouldn't have reached this point");
			throw DataManager::ErrGeneric();
		}
		} break;

	case LINEARACCELERATION:
	case ANGULARACCELERATION:
		{
		/* nodo collegato */
		StructNode* pNode = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		Vec3 Dir(HP.GetVecRel(ReferenceFrame(pNode)));
		doublereal d = Dir.Dot();
		ASSERT(d > 0.);
		if (d > 0.) {
			Dir /= sqrt(d);
		} else {
			silent_cerr("direction is null" << std::endl);
			throw ErrGeneric();
		}

		DriveCaller* pDC = HP.GetDriveCaller();

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		switch (CurrKeyWord) {
		case LINEARACCELERATION:
			SAFENEWWITHCONSTRUCTOR(pEl,
				LinearAccelerationJoint,
				LinearAccelerationJoint(uLabel, pDO,
					pNode, Dir, pDC, fOut));
			break;

		case ANGULARACCELERATION:
			SAFENEWWITHCONSTRUCTOR(pEl,
				AngularAccelerationJoint,
				AngularAccelerationJoint(uLabel, pDO,
					pNode, Dir, pDC, fOut));
			break;

		default:
			ASSERTMSG(0, "You shouldn't have reached this point");
			throw DataManager::ErrGeneric();
		}
		} break;

	case PRISMATIC:
		{
		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		ReferenceFrame RF(pNode1);

		Mat3x3 R1h(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R1h = HP.GetRotRel(RF);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R1h = HP.GetRotRel(RF);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}

		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		RF = ReferenceFrame(pNode2);

		/* Stessa cosa per il nodo 2 */

		Mat3x3 R2h(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R2h = HP.GetRotRel(RF);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R2h = HP.GetRotRel(RF);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			PrismaticJoint,
			PrismaticJoint(uLabel, pDO, pNode1, pNode2,
				R1h, R2h, fOut));

		std::ostream& out = pDM->GetLogFile();
		out << "prismatic: " << uLabel
			<< " " << pNode1->GetLabel()
			<< " " << Vec3(0.)
			<< " " << R1h
			<< " " << pNode2->GetLabel()
			<< " " << Vec3(0.)
			<< " " << R2h
			<< std::endl;
		} break;

	case DRIVEHINGE:
		{
		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		ReferenceFrame RF(pNode1);

		Mat3x3 R1h(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R1h = HP.GetRotRel(RF);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R1h = HP.GetRotRel(RF);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}


		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		RF = ReferenceFrame(pNode2);

		/* Stessa cosa per il nodo 2 */

		Mat3x3 R2h(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R2h = HP.GetRotRel(RF);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R2h = HP.GetRotRel(RF);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}


		TplDriveCaller<Vec3>* pDC = ReadDC3D(pDM, HP);

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			DriveHingeJoint,
			DriveHingeJoint(uLabel, pDO, pDC,
				pNode1, pNode2, R1h, R2h, fOut));

		} break;

	case DRIVEDISPLACEMENT:
		{
		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		ReferenceFrame RF(pNode1);

		Vec3 f1(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			f1 = HP.GetPosRel(ReferenceFrame(pNode1));
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		RF = ReferenceFrame(pNode2);

		/* Stessa cosa per il nodo 2 */
		Vec3 f2(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			f2 = HP.GetPosRel(ReferenceFrame(pNode2));
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		TplDriveCaller<Vec3>* pDC = ReadDC3D(pDM, HP);

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			DriveDisplacementJoint,
			DriveDisplacementJoint(uLabel, pDO, pDC,
				pNode1, pNode2, f1, f2, fOut));
		} break;

	case DRIVEDISPLACEMENTPIN:
		{
		/* nodo collegato */
		StructNode* pNode = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		ReferenceFrame RF(pNode);

		Vec3 f(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			f = HP.GetPosRel(ReferenceFrame(pNode));
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		/* Stessa cosa per il terreno */
		Vec3 x(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			x = HP.GetPosAbs(AbsRefFrame);
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		TplDriveCaller<Vec3>* pDC = ReadDC3D(pDM, HP);

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			DriveDisplacementPinJoint,
			DriveDisplacementPinJoint(uLabel, pDO, pDC,
				pNode, f, x, fOut));
		} break;

	case IMPOSEDDISPLACEMENT:
		{
		/* nodo collegato 1 */
		StructNode* pNode1 = dynamic_cast<StructNode *>(pDM->ReadNode(HP, Node::STRUCTURAL));
		ReferenceFrame RF(pNode1);

		Vec3 f1(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			f1 = HP.GetPosRel(ReferenceFrame(pNode1));
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		/* nodo collegato 2 */
		StructNode* pNode2 = dynamic_cast<StructNode *>(pDM->ReadNode(HP, Node::STRUCTURAL));
		RF = ReferenceFrame(pNode2);

		/* Stessa cosa per il nodo 2 */
		Vec3 f2(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			f2 = HP.GetPosRel(ReferenceFrame(pNode2));
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		Vec3 e1 = HP.GetVecRel(ReferenceFrame(pNode1));

		DriveCaller* pDC = ReadDriveData(pDM, HP, false);

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			ImposedDisplacementJoint,
			ImposedDisplacementJoint(uLabel, pDO, pDC,
				pNode1, pNode2, f1, f2, e1, fOut));
		} break;

	case IMPOSEDDISPLACEMENTPIN:
		{
		/* nodo collegato */
		StructNode* pNode = dynamic_cast<StructNode *>(pDM->ReadNode(HP, Node::STRUCTURAL));
		ReferenceFrame RF(pNode);

		Vec3 f(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			f = HP.GetPosRel(ReferenceFrame(pNode));
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		/* Stessa cosa per il terreno */
		Vec3 x(0.);
		if (HP.IsKeyWord("position")) {
#ifdef MBDYN_X_COMPATIBLE_INPUT
			NO_OP;
		} else {
			pedantic_cerr("Joint(" << uLabel << "): "
				"missing keyword \"position\" at line "
				<< HP.GetLineData() << std::endl);
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */
			x = HP.GetPosAbs(AbsRefFrame);
#ifndef MBDYN_X_COMPATIBLE_INPUT
		}
#endif /* MBDYN_X_COMPATIBLE_INPUT */

		Vec3 e = HP.GetVecAbs(AbsRefFrame);

		DriveCaller* pDC = ReadDriveData(pDM, HP, false);

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			ImposedDisplacementPinJoint,
			ImposedDisplacementPinJoint(uLabel, pDO, pDC,
				pNode, f, x, e, fOut));
		} break;

	case IMPOSEDORIENTATION:
		{
		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		ReferenceFrame RF(pNode1);

		Mat3x3 R1h(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R1h = HP.GetRotRel(RF);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R1h = HP.GetRotRel(RF);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}


		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		RF = ReferenceFrame(pNode2);

		/* Stessa cosa per il nodo 2 */

		Mat3x3 R2h(Eye3);
		if (HP.IsKeyWord("orientation")) {
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R2h = HP.GetRotRel(RF);
#ifdef MBDYN_X_COMPATIBLE_INPUT
		} else if (HP.IsKeyWord("hinge")) {
			pedantic_cerr("Joint(" << uLabel << "): "
				"keyword \"hinge\" at line " << HP.GetLineData()
				<< " is deprecated; use \"orientation\" instead"
				<< std::endl);
			DEBUGCOUT("Hinge orientation matrix is supplied" << std::endl);
			R2h = HP.GetRotRel(RF);
#endif /* MBDYN_X_COMPATIBLE_INPUT */
		}

		bool bActive[3];
		for (unsigned i = 0; i < 3; i++) {
			if (HP.IsKeyWord("inactive")) {
				bActive[i] = false;

			} else if (HP.IsKeyWord("active")) {
				bActive[i] = true;

			} else {
				bActive[i] = HP.GetBool();
			}
		}

		TplDriveCaller<Vec3>* pDC = ReadDC3D(pDM, HP);

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			ImposedOrientationJoint,
			ImposedOrientationJoint(uLabel, pDO, bActive, pDC,
				pNode1, pNode2, R1h, R2h, fOut));

		} break;

	case TOTALEQUATION:
		{
		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		ReferenceFrame RF(pNode1);

		Vec3 f1(0.);
		if (HP.IsKeyWord("position")) {
			f1 = HP.GetPosRel(ReferenceFrame(pNode1));
		}

		Mat3x3 R1h(Eye3);
		if (HP.IsKeyWord("position" "orientation")) {
			DEBUGCOUT("Position orientation matrix is supplied" << std::endl);
			R1h = HP.GetRotRel(RF);
		}

		Mat3x3 R1hr(Eye3);
		if (HP.IsKeyWord("rotation" "orientation")) {
			DEBUGCOUT("Rotation orientation matrix is supplied" << std::endl);
			R1hr = HP.GetRotRel(RF);
		}

		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		RF = ReferenceFrame(pNode2);

		Vec3 f2(0.);
		if (HP.IsKeyWord("position")) {
			f2 = HP.GetPosRel(ReferenceFrame(pNode2));
		}

		Mat3x3 R2h(Eye3);
		if (HP.IsKeyWord("position" "orientation")) {
			DEBUGCOUT("Position orientation matrix is supplied" << std::endl);
			R2h = HP.GetRotRel(RF);
		}

		Mat3x3 R2hr(Eye3);
		if (HP.IsKeyWord("rotation" "orientation")) {
			DEBUGCOUT("Rotation orientation matrix is supplied" << std::endl);
			R2hr = HP.GetRotRel(RF);
		}

		bool bXActive[3] = { false, false, false };
		TplDriveCaller<Vec3>* pXDC[3] = {0, 0, 0};
		if (HP.IsKeyWord("position" "constraint")) {
			for (unsigned i = 0; i < 3; i++) {
				if (HP.IsKeyWord("inactive")) {
					bXActive[i] = false;

				} else if (HP.IsKeyWord("active")) {
					bXActive[i] = true;

				} else {
					if (HP.IsArg()) {
						bXActive[i] = HP.GetBool();
						continue;
					}

					silent_cerr("TotalEquation(" << uLabel << "): "
						"invalid status for position component #" << i + 1
						<< " at line " << HP.GetLineData() << std::endl);
					throw ErrGeneric();
				}
			}

			pXDC[0] = ReadDC3D(pDM, HP);

		} else {
			SAFENEW(pXDC[0], ZeroTplDriveCaller<Vec3>);
		}

		bool bTActive[3] = { false, false, false };
		TplDriveCaller<Vec3>* pTDC[3] = {0, 0, 0};
		if (HP.IsKeyWord("orientation" "constraint")) {
			for (unsigned i = 0; i < 3; i++) {
				if (HP.IsKeyWord("inactive")) {
					bTActive[i] = false;

				} else if (HP.IsKeyWord("active")) {
					bTActive[i] = true;

				} else {
					if (HP.IsArg()) {
						bTActive[i] = HP.GetBool();
						continue;
					}

					silent_cerr("TotalEquation(" << uLabel << "): "
						"invalid status for position component #" << i + 1
						<< " at line " << HP.GetLineData() << std::endl);
					throw ErrGeneric();
				}
			}

			pTDC[0] = ReadDC3D(pDM, HP);

		} else {
			SAFENEW(pTDC[0], ZeroTplDriveCaller<Vec3>);
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			TotalEquation,
			TotalEquation(uLabel, pDO,
				bXActive, pXDC,
				bTActive, pTDC,
				pNode1, f1, R1h, R1hr,
				pNode2, f2, R2h, R2hr,
				fOut));

		std::ostream& out = pDM->GetLogFile();
		out << "totaljoint: " << uLabel
			<< " " << pNode1->GetLabel()
			<< " " << f1
			<< " " << R1h
			<< " " << pNode2->GetLabel()
			<< " " << f2
			<< " " << R2h
			<< std::endl;
		} break;

	case TOTALINTERNALREACTION:
		{
		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		ReferenceFrame RF(pNode1);

		Vec3 f1(0.);
		if (HP.IsKeyWord("position")) {
			f1 = HP.GetPosRel(ReferenceFrame(pNode1));
		}

		Mat3x3 R1h(Eye3);
		if (HP.IsKeyWord("position" "orientation")) {
			DEBUGCOUT("Position orientation matrix is supplied" << std::endl);
			R1h = HP.GetRotRel(RF);
		}

		Mat3x3 R1hr(Eye3);
		if (HP.IsKeyWord("rotation" "orientation")) {
			DEBUGCOUT("Rotation orientation matrix is supplied" << std::endl);
			R1hr = HP.GetRotRel(RF);
		}

		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		RF = ReferenceFrame(pNode2);

		Vec3 f2(0.);
		if (HP.IsKeyWord("position")) {
			f2 = HP.GetPosRel(ReferenceFrame(pNode2));
		}

		Mat3x3 R2h(Eye3);
		if (HP.IsKeyWord("position" "orientation")) {
			DEBUGCOUT("Position orientation matrix is supplied" << std::endl);
			R2h = HP.GetRotRel(RF);
		}

		Mat3x3 R2hr(Eye3);
		if (HP.IsKeyWord("rotation" "orientation")) {
			DEBUGCOUT("Rotation orientation matrix is supplied" << std::endl);
			R2hr = HP.GetRotRel(RF);
		}

		bool bXActive[3] = { false, false, false };
		if (HP.IsKeyWord("position" "constraint")) {
			for (unsigned i = 0; i < 3; i++) {
				if (HP.IsKeyWord("inactive")) {
					bXActive[i] = false;

				} else if (HP.IsKeyWord("active")) {
					bXActive[i] = true;

				} else {
					if (HP.IsArg()) {
						bXActive[i] = HP.GetInt();
						continue;
					}

					silent_cerr("TotalReaction(" << uLabel << "): "
						"invalid status for position component #" << i + 1
						<< " at line " << HP.GetLineData() << std::endl);
					throw ErrGeneric();
				}
			}
		}

		bool bTActive[3] = { false, false, false };
		if (HP.IsKeyWord("orientation" "constraint")) {
			for (unsigned i = 0; i < 3; i++) {
				if (HP.IsKeyWord("inactive")) {
					bTActive[i] = false;

				} else if (HP.IsKeyWord("active")) {
					bTActive[i] = true;

				} else {
					if (HP.IsArg()) {
						bTActive[i] = HP.GetBool();
						continue;
					}

					silent_cerr("TotalReaction(" << uLabel << "): "
						"invalid status for position component #" << i + 1
						<< " at line " << HP.GetLineData() << std::endl);
					throw ErrGeneric();
				}
			}

		}
		
		TotalEquation * tot_eq_pt = 0;
		if (HP.IsKeyWord("total" "equation")) {
			unsigned int tot_eq_j_label = HP.GetInt();
			Elem* el_pt = pDM->pFindElem(Elem::JOINT, tot_eq_j_label);
			tot_eq_pt = dynamic_cast<TotalEquation*>(el_pt);
			if (tot_eq_pt == 0) {
				silent_cerr("TotalEquation" "(" << tot_eq_j_label << ") "
					"needed by TotalReaction(" << uLabel << ") "
					"at line " << HP.GetLineData()
					<< " is not defined"
					<< std::endl);
				throw ErrGeneric();
			}
		} else {
			silent_cerr("Total Reaction(" << uLabel << ") "
				"needs a reference to a corresponding " 
				"TotalEquation at line " << HP.GetLineData() << 
				std::endl);
		}


		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			TotalReaction,
			TotalReaction(uLabel, pDO,
				bXActive, 
				bTActive, 
				pNode1, f1, R1h, R1hr,
				pNode2, f2, R2h, R2hr,
				tot_eq_pt,
				fOut));

		std::ostream& out = pDM->GetLogFile();
		out << "totaljoint: " << uLabel
			<< " " << pNode1->GetLabel()
			<< " " << f1
			<< " " << R1h
			<< " " << pNode2->GetLabel()
			<< " " << f2
			<< " " << R2h
			<< std::endl;
		} break;

	case TOTALJOINT:
		{
		/* nodo collegato 1 */
		StructNode* pNode1 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		ReferenceFrame RF(pNode1);

		Vec3 f1(0.);
		if (HP.IsKeyWord("position")) {
			f1 = HP.GetPosRel(ReferenceFrame(pNode1));
		}

		Mat3x3 R1h(Eye3);
		if (HP.IsKeyWord("position" "orientation")) {
			DEBUGCOUT("Position orientation matrix is supplied" << std::endl);
			R1h = HP.GetRotRel(RF);
		}

		Mat3x3 R1hr(Eye3);
		if (HP.IsKeyWord("rotation" "orientation")) {
			DEBUGCOUT("Rotation orientation matrix is supplied" << std::endl);
			R1hr = HP.GetRotRel(RF);
		}

		/* nodo collegato 2 */
		StructNode* pNode2 = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		RF = ReferenceFrame(pNode2);

		Vec3 f2(0.);
		if (HP.IsKeyWord("position")) {
			f2 = HP.GetPosRel(ReferenceFrame(pNode2));
		}

		Mat3x3 R2h(Eye3);
		if (HP.IsKeyWord("position" "orientation")) {
			DEBUGCOUT("Position orientation matrix is supplied" << std::endl);
			R2h = HP.GetRotRel(RF);
		}

		Mat3x3 R2hr(Eye3);
		if (HP.IsKeyWord("rotation" "orientation")) {
			DEBUGCOUT("Rotation orientation matrix is supplied" << std::endl);
			R2hr = HP.GetRotRel(RF);
		}

		bool bXActive[3] = { false, false, false };
		bool bVActive[3] = { false, false, false };
		TplDriveCaller<Vec3>* pXDC[3] = {0, 0, 0};
		if (HP.IsKeyWord("position" "constraint")) {
			for (unsigned i = 0; i < 3; i++) {
				if (HP.IsKeyWord("inactive")) {
					bXActive[i] = false;
					bVActive[i] = false;

				} else if (HP.IsKeyWord("position") || HP.IsKeyWord("active")) {
					bXActive[i] = true;
					bVActive[i] = false;

				} else if (HP.IsKeyWord("velocity")) {
					bVActive[i] = true;
					bXActive[i] = false;

				} else {
					if (HP.IsArg()) {
						bool bActive = HP.GetBool();
						bXActive[i] = bActive;
						bVActive[i] = bActive;
						continue;
					}

					silent_cerr("TotalJoint(" << uLabel << "): "
						"invalid status for position component #" << i + 1
						<< " at line " << HP.GetLineData() << std::endl);
					throw ErrGeneric();
				}
			}

			pXDC[0] = ReadDC3D(pDM, HP);

			if (pDM->bIsInverseDynamics()) {
				pXDC[1] = ReadDC3D(pDM, HP);
				pXDC[2] = ReadDC3D(pDM, HP);
			}

		} else {
			SAFENEW(pXDC[0], ZeroTplDriveCaller<Vec3>);
		}

		bool bTActive[3] = { false, false, false };
		bool bWActive[3] = { false, false, false };
		TplDriveCaller<Vec3>* pTDC[3] = {0, 0, 0};
		if (HP.IsKeyWord("orientation" "constraint")) {
			for (unsigned i = 0; i < 3; i++) {
				if (HP.IsKeyWord("inactive")) {
					bTActive[i] = false;
					bWActive[i] = false;

				} else if (HP.IsKeyWord("rotation") || HP.IsKeyWord("active")) {
					bTActive[i] = true;
					bWActive[i] = false;
				
				} else if (HP.IsKeyWord("angular" "velocity")) {
					bTActive[i] = false;
					bWActive[i] = true;

				} else {
					if (HP.IsArg()) {
						bool bActive = HP.GetInt();
						bTActive[i] = bActive;
						bWActive[i] = bActive;
						continue;
					}

					silent_cerr("TotalJoint(" << uLabel << "): "
						"invalid status for position component #" << i + 1
						<< " at line " << HP.GetLineData() << std::endl);
					throw ErrGeneric();
				}
			}

			pTDC[0] = ReadDC3D(pDM, HP);

			if (pDM->bIsInverseDynamics()) {
				pTDC[1] = ReadDC3D(pDM, HP);
				pTDC[2] = ReadDC3D(pDM, HP);
			}

		} else {
			SAFENEW(pTDC[0], ZeroTplDriveCaller<Vec3>);
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			TotalJoint,
			TotalJoint(uLabel, pDO,
				bXActive, bVActive, pXDC,
				bTActive, bWActive, pTDC,
				pNode1, f1, R1h, R1hr,
				pNode2, f2, R2h, R2hr,
				fOut));

		std::ostream& out = pDM->GetLogFile();
		out << "totaljoint: " << uLabel
			<< " " << pNode1->GetLabel()
			<< " " << f1
			<< " " << R1h
			<< " " << pNode2->GetLabel()
			<< " " << f2
			<< " " << R2h
			<< std::endl;
		} break;

	case TOTALPINJOINT:
		{
		/* nodo collegato */
		StructNode* pNode = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);
		ReferenceFrame RF(pNode);

		Vec3 fn(0.);
		if (HP.IsKeyWord("position")) {
			fn = HP.GetPosRel(RF);
		}

		Mat3x3 Rnh(Eye3);
		if (HP.IsKeyWord("position" "orientation")) {
			DEBUGCOUT("Position orientation matrix is supplied" << std::endl);
			Rnh = HP.GetRotRel(RF);
		}

		Mat3x3 Rnhr(Eye3);
		if (HP.IsKeyWord("rotation" "orientation")) {
			DEBUGCOUT("Rotation orientation matrix is supplied" << std::endl);
			Rnhr = HP.GetRotRel(RF);
		}

		Vec3 Xc(0.);
		if (HP.IsKeyWord("position")) {
			Xc = HP.GetPosAbs(AbsRefFrame);
		}

		Mat3x3 Rch(Eye3);
		if (HP.IsKeyWord("position" "orientation")) {
			DEBUGCOUT("Position orientation matrix is supplied" << std::endl);
			Rch = HP.GetRotAbs(AbsRefFrame);
		}

		Mat3x3 Rchr(Eye3);
		if (HP.IsKeyWord("rotation" "orientation")) {
			DEBUGCOUT("Rotation orientation matrix is supplied" << std::endl);
			Rchr = HP.GetRotAbs(AbsRefFrame);
		}

		bool bXActive[3] = { false, false, false };
		TplDriveCaller<Vec3>* pXDC[3] = {0, 0, 0};
		if (HP.IsKeyWord("position" "constraint")) {
			for (unsigned i = 0; i < 3; i++) {
				if (HP.IsKeyWord("inactive")) {
					bXActive[i] = false;

				} else if (HP.IsKeyWord("active")) {
					bXActive[i] = true;

				} else {
					if (HP.IsArg()) {
						bXActive[i] = HP.GetBool();
						continue;
					}

					silent_cerr("TotalJoint(" << uLabel << "): "
						"invalid status for position component #" << i + 1
						<< " at line " << HP.GetLineData() << std::endl);
					throw ErrGeneric();
				}
			}

			pXDC[0] = ReadDC3D(pDM, HP);

			if (pDM->bIsInverseDynamics()) {
				pXDC[1] = ReadDC3D(pDM, HP);
				pXDC[2] = ReadDC3D(pDM, HP);
			}

		} else {
			SAFENEW(pXDC[0], ZeroTplDriveCaller<Vec3>);
		}

		bool bTActive[3] = { false, false, false };
		TplDriveCaller<Vec3>* pTDC[3] = {0, 0, 0};
		if (HP.IsKeyWord("orientation" "constraint")) {
			for (unsigned i = 0; i < 3; i++) {
				if (HP.IsKeyWord("inactive")) {
					bTActive[i] = false;

				} else if (HP.IsKeyWord("active")) {
					bTActive[i] = true;

				} else {
					if (HP.IsArg()) {
						bTActive[i] = HP.GetBool();
						continue;
					}

					silent_cerr("TotalJoint(" << uLabel << "): "
						"invalid status for position component #" << i + 1
						<< " at line " << HP.GetLineData() << std::endl);
					throw ErrGeneric();
				}
			}

			pTDC[0] = ReadDC3D(pDM, HP);

			if (pDM->bIsInverseDynamics()) {
				pTDC[1] = ReadDC3D(pDM, HP);
				pTDC[2] = ReadDC3D(pDM, HP);
			}

		} else {
			SAFENEW(pTDC[0], ZeroTplDriveCaller<Vec3>);
		}

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			TotalPinJoint,
			TotalPinJoint(uLabel, pDO,
				bXActive, pXDC,
				bTActive, pTDC,
				Xc, Rch, Rchr,
				pNode, fn, Rnh, Rnhr,
				fOut));
		} break;

	case KINEMATIC:
		{
		/* nodo collegato */
		StructNode* pNode = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		DriveCaller* pDC = HP.GetDriveCaller();

		Kinematics* pK = NULL;
		SAFENEWWITHCONSTRUCTOR(pK, KinematicsTest, KinematicsTest(pDC));

		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);

		SAFENEWWITHCONSTRUCTOR(pEl,
			KinJoint,
			KinJoint(uLabel, pDO, pNode, pK, fOut));
		} break;

	case BEAMSLIDER:
		{
		/* Corpo slittante */
		StructNode* pNode = (StructNode*)pDM->ReadNode(HP, Node::STRUCTURAL);

		ReferenceFrame RF(pNode);
		Vec3 f(HP.GetPosRel(RF));
		DEBUGCOUT("Linked to Node " << pNode->GetLabel()
			<< "with offset " << f << std::endl);

		Mat3x3 R = Eye3;
		if (HP.IsKeyWord("hinge")) {
			R = HP.GetRotRel(RF);
		}
		DEBUGLCOUT(MYDEBUG_INPUT,
			"Slider rotation matrix: " << std::endl << R << std::endl);

		/* Slider type */
		BeamSliderJoint::Type sliderType = BeamSliderJoint::SPHERICAL;
		if (HP.IsKeyWord("type")) {
			if (HP.IsKeyWord("spherical")) {
				sliderType = BeamSliderJoint::SPHERICAL;
			} else if (HP.IsKeyWord("classic")) {
				sliderType = BeamSliderJoint::CLASSIC;
			} else if (HP.IsKeyWord("spline")) {
				sliderType = BeamSliderJoint::SPLINE;
			} else {
				silent_cerr("unknown slider type at line "
					<< HP.GetLineData() << std::endl);
				throw ErrGeneric();
			}
		}

		unsigned int nB = HP.GetInt();
		if (nB < 1) {
			silent_cerr("At least one beam is required at line "
				<< HP.GetLineData() << std::endl);
			throw ErrGeneric();
		}

		BeamConn **bc = NULL;
		SAFENEWARR(bc, BeamConn *, nB);

		const StructNode* pLastNode = NULL;
		for (unsigned int i = 0; i < nB; i++) {
			/* trave di appoggio */
			unsigned int uBeam = HP.GetInt();

			Elem* p = (Elem*)(pDM->pFindElem(Elem::BEAM, uBeam));
			if (p == NULL) {
				silent_cerr(" at line " << HP.GetLineData()
					<< ": Beam3(" << uBeam << ") "
					"not defined" << std::endl);
				throw DataManager::ErrGeneric();
			}
			Beam *pBeam = dynamic_cast<Beam *>(p);
			if (pBeam == 0) {
				silent_cerr(" at line " << HP.GetLineData()
					<< ": Beam(" << uBeam << ") "
					"is not a Beam3" << std::endl);
				throw DataManager::ErrGeneric();
			}

			/* Nodo 1: */

			/* Offset del punto rispetto al nodo */
			const StructNode* pNode1 = pBeam->pGetNode(1);
			RF = ReferenceFrame(pNode1);
			Vec3 f1;
			Mat3x3 R1 = Eye3;
			if (i != 0) {
				if (pNode1 != pLastNode) {
					silent_cerr("line " << HP.GetLineData() << ": "
						"Beam(" << pBeam->GetLabel() << ")"
							".Node1(" << pNode1->GetLabel() << ") "
						"and Beam(" << bc[i-1]->pGetBeam()->GetLabel() << ")"
							".Node3(" << pLastNode->GetLabel() << ") "
						"do not match" << std::endl);
					throw ErrGeneric();
				}

				if (HP.IsKeyWord("same")) {
					f1 = bc[i-1]->Getf(3);
				} else {
					f1 = HP.GetPosRel(RF);
					/* FIXME: allow tolerance? */
					if (!f1.IsExactlySame(bc[i-1]->Getf(3))) {
						silent_cerr("line " << HP.GetLineData() << ": "
							"Beam(" << pBeam->GetLabel() << ").f1 "
							"and Beam(" << bc[i-1]->pGetBeam()->GetLabel() << ").f3 "
							"do not match" << std::endl);
						throw ErrGeneric();
					}
				}
				DEBUGLCOUT(MYDEBUG_INPUT, "Node 1 offset: " << f1 << std::endl);

				if (HP.IsKeyWord("hinge")) {
					if (HP.IsKeyWord("same")) {
						R1 = bc[i-1]->GetR(3);
					} else {
						R1 = HP.GetRotRel(RF);
						/* FIXME: allow tolerance? */
						if (!R1.IsExactlySame(bc[i-1]->GetR(3))) {
							silent_cerr("line " << HP.GetLineData() << ": "
								"Beam(" << pBeam->GetLabel() << ").R1 "
								"and Beam(" << bc[i-1]->pGetBeam()->GetLabel() << ").R3 "
								"do not match" << std::endl);
							throw ErrGeneric();
						}
					}
					DEBUGLCOUT(MYDEBUG_INPUT, "Node 1 rotation matrix: "
						<< std::endl << R1 << std::endl);
				}

			} else {
				f1 = HP.GetPosRel(RF);
				if (HP.IsKeyWord("hinge")) {
					R1 = HP.GetRotRel(RF);
					DEBUGLCOUT(MYDEBUG_INPUT, "Node 1 rotation matrix: "
						<< std::endl << R1 << std::endl);
				}
			}

			/* Nodo 2: */

			/* Offset del punto rispetto al nodo */
			const StructNode* pNode2 = pBeam->pGetNode(2);

			RF = ReferenceFrame(pNode2);
			Vec3 f2(HP.GetPosRel(RF));
			DEBUGLCOUT(MYDEBUG_INPUT, "Node 2 offset: " << f2 << std::endl);

			Mat3x3 R2(Eye3);
			if (HP.IsKeyWord("hinge")) {
				R2 = HP.GetRotRel(RF);
				DEBUGLCOUT(MYDEBUG_INPUT,
					"Node 2 rotation matrix: " << std::endl
					<< R2 << std::endl);
			}

			/* Nodo 3: */

			/* Offset del punto rispetto al nodo */
			const StructNode* pNode3 = pBeam->pGetNode(3);

			RF = ReferenceFrame(pNode3);
			Vec3 f3(HP.GetPosRel(RF));
			DEBUGLCOUT(MYDEBUG_INPUT, "Node 3 offset: " << f3 << std::endl);

			Mat3x3 R3(Eye3);
			if (HP.IsKeyWord("hinge")) {
				R3 = HP.GetRotRel(RF);
				DEBUGLCOUT(MYDEBUG_INPUT,
					"Node 3 rotation matrix: " << std::endl
					<< R3 << std::endl);
			}

			pLastNode = pNode3;

			bc[i] = NULL;
			SAFENEWWITHCONSTRUCTOR(bc[i], BeamConn,
				BeamConn(pBeam, f1, f2, f3, R1, R2, R3));
		}

		unsigned uIB = 1;
		if (HP.IsKeyWord("initial" "beam")) {
			uIB = HP.GetInt();

			if (uIB < 1 || uIB > nB) {
				silent_cerr("illegal initial beam " << uIB
					<< " at line " << HP.GetLineData()
					<< std::endl);
				throw ErrGeneric();
			}
		}

		unsigned uIN = 2;
		if (HP.IsKeyWord("initial" "node")) {
			uIN = HP.GetInt();

			if (uIN < 1 || uIN > 3) {
				silent_cerr("illegal initial node " << uIN
					<< " at line " << HP.GetLineData() << std::endl);
				throw ErrGeneric();
			}
		}

		doublereal dL = 0.;
		if (HP.IsKeyWord("smearing")) {
			dL = HP.GetReal();
			if (dL < 0. || dL > .4) {
				silent_cerr("illegal smearing factor " << dL << "; "
					"using default" << std::endl);
				dL = 0.;
			}
		}


		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);
		SAFENEWWITHCONSTRUCTOR(pEl, BeamSliderJoint,
			BeamSliderJoint(uLabel, pDO,
				pNode,
				sliderType,
				nB, bc,
				uIB, uIN, dL,
				f, R, fOut));
		} break;

	case MODAL:
		pEl = ReadModal(pDM, HP, pDO, uLabel);
		break;

	case POINT_SURFACE_CONTACT:  {
		/* leggo i due nodi */
		/* nodo collegato */
		StructNode* pNode1 = dynamic_cast<StructNode *>(pDM->ReadNode(HP, Node::STRUCTURAL));


		/* nodo superficie*/
		StructNode* pSup = dynamic_cast<StructNode *>(pDM->ReadNode(HP, Node::STRUCTURAL));
	
		/* leggo posizione e direzione della superficie nel sistema del nodo*/
		Vec3 SupDirection = HP.GetVecRel(ReferenceFrame(pSup));

	
		/* Normalizzo l'orientazione del terreno */
		doublereal d = SupDirection.Dot();
		if (d <= DBL_EPSILON) {
			silent_cerr("PointSurfaceContact(" << uLabel << "): "
				"invalid direction at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric();
		}
		SupDirection /= sqrt(d);

		double ElasticStiffness = HP.GetReal();
	
		flag fOut = pDM->fReadOutput(HP, Elem::JOINT);
 		
 		SAFENEWWITHCONSTRUCTOR(pEl,
			PointSurfaceContact,
			PointSurfaceContact(uLabel, pDO,
				pNode1, pSup, SupDirection, ElasticStiffness, fOut));
						
	} break;
	


	/* Aggiungere qui altri vincoli */

			

	default:
		{
		const char *s = HP.GetString();

		const LoadableCalls *c = pDM->GetLoadableElemModule(s);

		if (c == 0) {
			silent_cerr("unknown joint type \"" << s << "\" in joint " << uLabel
				<< " at line " << HP.GetLineData() << std::endl);
			throw DataManager::ErrGeneric();
		}

		SAFENEWWITHCONSTRUCTOR(pEl, LoadableElem,
			LoadableElem(uLabel, pDO, c, pDM, HP));
		pDM->OutputOpen(OutputHandler::LOADABLE);
		} break;
	}

	/* Se non c'e' il punto e virgola finale */
	if (pEl == NULL) {
		DEBUGCERR("");
		silent_cerr("error in allocation of element "
			<< uLabel << std::endl);

		throw ErrMemory();
	}

	if (HP.IsKeyWord("initial" "state")) {
		pEl->ReadInitialState(HP);
		/* FIXME: what if fails? */
	}

	if (HP.IsArg()) {
		silent_cerr("semicolon expected at line " << HP.GetLineData()
			<< std::endl);
		throw DataManager::ErrGeneric();
	}

	return pEl;
} /* ReadJoint() */
