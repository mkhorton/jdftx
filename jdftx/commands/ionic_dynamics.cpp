/*-------------------------------------------------------------------
Copyright 2011 Ravishankar Sundararaman, Deniz Gunceler, Kendra Letchworth-Weaver

This file is part of JDFTx.

JDFTx is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

JDFTx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with JDFTx.  If not, see <http://www.gnu.org/licenses/>.
-------------------------------------------------------------------*/

#include <commands/command.h>
#include <commands/ParamList.h>
#include <electronic/Everything.h>
#include <electronic/IonicDynamicsParams.h>
#include <core/Units.h>


EnumStringMap<IonicDynamicsParams::StatMethod> statMethodMap
(	IonicDynamicsParams::StatNone, "None",
	IonicDynamicsParams::Berendsen, "Berendsen",
	IonicDynamicsParams::NoseHoover, "NoseHoover"
);

//An enum entry for each configurable option of IonicDynamicsParams
enum IonicDynamicsParamsMember
{	IDPM_dt,
	IDPM_nSteps,
	IDPM_statMethod,
	IDPM_T0,
	IDPM_P0,
	IDPM_stress0,
	IDPM_tDampT,
	IDPM_tDampP,
	IDPM_chainLengthT,
	IDPM_chainLengthP,
	IDPM_B0,
	IDPM_Delim //!< delimiter to detect end of input
};

EnumStringMap<IonicDynamicsParamsMember> idpmMap
(	IDPM_dt, "dt",
	IDPM_nSteps, "nSteps",
	IDPM_statMethod, "statMethod",
	IDPM_T0, "T0",
	IDPM_P0, "P0",
	IDPM_stress0, "stress0",
	IDPM_tDampT, "tDampT",
	IDPM_tDampP, "tDampP",
	IDPM_chainLengthT, "chainLengthT",
	IDPM_chainLengthP, "chainLengthP",
	IDPM_B0, "B0"
);

EnumStringMap<IonicDynamicsParamsMember> idpmDescMap
(	IDPM_dt, "time step [fs]",
	IDPM_nSteps, "number of molecular dynamics steps",
	IDPM_statMethod, statMethodMap.optionList() + " (method for thermostat and/or barostat)",
	IDPM_T0, "thermostat temperature or temperature for initial velocities [Kelvin]",
	IDPM_P0, "barostat pressure [bar] (default NAN => no hydrostatic barostat)",
	IDPM_stress0, "barostat stress components xx yy zz yz zx xy [bar] (default NANs => no anisotropic barostat)",
	IDPM_tDampT, "thermostat damping time [fs]",
	IDPM_tDampP, "barostat damping time [fs]",
	IDPM_chainLengthT, "Nose-Hoover chain length for thermostat",
	IDPM_chainLengthP, "Nose-Hoover chain length for barostat",
	IDPM_B0, "Characteristic bulk modulus [bar] for Berendsen barostat (damping ~ B0 * tDampP)"
);

struct CommandIonicDynamics : public Command
{
	CommandIonicDynamics() : Command("ionic-dynamics", "jdftx/Ionic/Optimization")
	{	format = "<time-step> <total-time> [<kT>=0.001] [<alpha>=0.0]";
		comments = "Applies Verlet molecular dynamics\n"
			   "Takes the time in femtoseconds and kT is in Hartree.\n"
			   "Give <alpha> if you want to the system to equilibrate with a heat bath at <kT>.\n"
			   "Also make sure to turn the symmetries off if the initial velocities don't satisfy the symmetry\n"
			   "conditions. Initial velocities will be assigned randomly if they are not given.";
			   
		format = "<key1> <value1> <key2> <value2> ...";
		comments = "Born-Oppenheimer molecular dynamics, controlled by keys:"
		+ addDescriptions(idpmMap.optionList(), linkDescription(idpmMap, idpmDescMap))
		+ "\n\nAny number of these key-value pairs may be specified in any order.\n\n"
			"Note that nSteps must be non-zero to activate dynamics.\n"
			"Default mode is NVE; specify statMethod to add a thermostat or barostat";
	}

	void process(ParamList& pl, Everything& e)
	{	IonicDynamicsParams& idp = e.ionicDynParams;
		const double nanVal = NAN;
		while(true)
		{	IonicDynamicsParamsMember key;
			pl.get(key, IDPM_Delim, idpmMap, "key");
			switch(key)
			{	case IDPM_dt: pl.get(idp.dt, 1., "dt", true); idp.dt *= fs; break;
				case IDPM_nSteps: pl.get(idp.nSteps, 0, "nSteps", true); break;
				case IDPM_statMethod: pl.get(idp.statMethod, IonicDynamicsParams::StatNone, statMethodMap, "statMethod", true); break;
				case IDPM_T0: pl.get(idp.T0, nanVal, "T0", true); idp.T0 *= Kelvin; break;
				case IDPM_P0: pl.get(idp.P0, nanVal, "P0", true); idp.P0 *= Bar; break;
				case IDPM_stress0:
				{	pl.get(idp.stress0(0,0), nanVal, "stress0_xx", true);
					pl.get(idp.stress0(1,1), nanVal, "stress0_yy", true);
					pl.get(idp.stress0(2,2), nanVal, "stress0_zz", true);
					pl.get(idp.stress0(1,2), nanVal, "stress0_yz", true);
					pl.get(idp.stress0(2,0), nanVal, "stress0_zx", true);
					pl.get(idp.stress0(0,1), nanVal, "stress0_xy", true);
					idp.stress0(2,1) = idp.stress0(1,2);
					idp.stress0(0,2) = idp.stress0(2,0);
					idp.stress0(1,0) = idp.stress0(0,1);
					idp.stress0 *= Bar;
					break;
				}
				case IDPM_tDampT: pl.get(idp.tDampT, 50., "tDampT", true); idp.tDampT *= fs; break;
				case IDPM_tDampP: pl.get(idp.tDampP, 100., "tDampP", true); idp.tDampP *= fs; break;
				case IDPM_chainLengthT: pl.get(idp.chainLengthT, 3, "chainLengthT", true); break;
				case IDPM_chainLengthP: pl.get(idp.chainLengthP, 3, "chainLengthP", true); break;
				case IDPM_B0: pl.get(idp.B0, nanVal, "B0", true); idp.B0 *= Bar; break;
				case IDPM_Delim: return; //end of input
			}
		}
		if((not std::isnan(idp.P0)) and (not std::isnan(trace(idp.stress0))))
			throw("Cannot specify both P0 (hydrostatic) and stress0 (anisotropic) barostats");
	}

	void printStatus(Everything& e, int iRep)
	{	const IonicDynamicsParams& idp = e.ionicDynParams;
		logPrintf(" \\\n\tdt         %lg", idp.dt/fs);
		logPrintf(" \\\n\tnSteps     %d", idp.nSteps);
		logPrintf(" \\\n\tstatMethod %s", statMethodMap.getString(idp.statMethod));
		logPrintf(" \\\n\tT0         %lg", idp.T0/Kelvin);
		logPrintf(" \\\n\tP0         %lg", idp.P0/Bar);
		logPrintf(" \\\n\tstress0 %lg %lg %lg  %lg %lg %lg", 
			idp.stress0(0,0)/Bar, idp.stress0(1,1)/Bar, idp.stress0(2,2)/Bar,
			idp.stress0(1,2)/Bar, idp.stress0(2,0)/Bar, idp.stress0(0,1)/Bar);
		logPrintf(" \\\n\ttDampT       %lg", idp.tDampT/fs);
		logPrintf(" \\\n\ttDampP       %lg", idp.tDampP/fs);
		logPrintf(" \\\n\tchainLengthT %d", idp.chainLengthT);
		logPrintf(" \\\n\tchainLengthP %d", idp.chainLengthP);
		logPrintf(" \\\n\tB0           %lg", idp.B0/Bar);
	}
}
commandIonicDynamics;


struct CommandLjOverride : public Command
{
	CommandLjOverride() : Command("lj-override", "jdftx/Ionic/Optimization")
	{	format = "<rCut>";
		comments =
			"Replace electronic DFT by a Lennard-Jones only potential with cutoff <rCut> in Angstroms.\n"
			"This potential will use LJ parameters, sigma = 2^(5/6) R0 and epsilon = C6/(128 R0^6),\n"
			"where R0 and C6 are DFT-D2 parameters for each species (adjustable by command setVDW).\n"
			"This is not for real calculations, but a quick way to debug ionic-minimize,\n"
			"lattice-minimize or ionic-dynamics. Tip: set elec-cutoff to a small value to\n"
			"speed up electronic initialization (which is not bypassed for code simplicity).";
	}
	
	void process(ParamList& pl, Everything& e)
	{	pl.get(e.iInfo.ljOverride, 0., "rCut", true);
		e.iInfo.ljOverride *= Angstrom; //convert to atomic units
	}
	
	void printStatus(Everything& e, int iRep)
	{	logPrintf("%lg", e.iInfo.ljOverride/Angstrom);
	}
}
commandLjOverride;

//---- Thermostat / barostat velocities ----
struct CommandStatVelocity : public Command
{
	CommandStatVelocity(string statName) : Command(statName+"-velocity", "jdftx/Ionic/Optimization")
	{	format = "<v1> <v2> ...";
		comments =
			"Read "+statName+" internal velocities for continuing ionic dynamics.\n"
			"This command is automatically dumped with ionpos from dynamics simulations\n"
			"using Nose-Hoover chains that involve "+statName+" internal velocities.";
	}
	
	void process(ParamList& pl, std::vector<double>& target)
	{	target.clear();
		while(true)
		{	double v = NAN;
			pl.get(v, v, "v");
			if(std::isnan(v)) break;
			target.push_back(v);
		}
	}
	
	void printStatus(const std::vector<double>& target)
	{	for(const double& v: target) logPrintf("%lg ", v);
	}
};

struct CommandThermostatVelocity : public CommandStatVelocity
{	CommandThermostatVelocity() : CommandStatVelocity("thermostat") {}
	void process(ParamList& pl, Everything& e) { CommandStatVelocity::process(pl, e.iInfo.thermostat); }
	void printStatus(Everything& e, int iRep) {	CommandStatVelocity::printStatus(e.iInfo.thermostat); }
}
commandThermostatVelocity;

struct CommandBarostatVelocity : public CommandStatVelocity
{	CommandBarostatVelocity() : CommandStatVelocity("barostat") {}
	void process(ParamList& pl, Everything& e) { CommandStatVelocity::process(pl, e.iInfo.barostat); }
	void printStatus(Everything& e, int iRep) {	CommandStatVelocity::printStatus(e.iInfo.barostat); }
}
commandBarostatVelocity;
