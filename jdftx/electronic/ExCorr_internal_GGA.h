/*-------------------------------------------------------------------
Copyright 2012 Ravishankar Sundararaman, Kendra Letchworth Weaver

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

#ifndef JDFTX_ELECTRONIC_EXCORR_INTERNAL_GGA_H
#define JDFTX_ELECTRONIC_EXCORR_INTERNAL_GGA_H

#include <electronic/ExCorr_internal_LDA.h>

//! @addtogroup ExchangeCorrelation
//! @{
//! @file ExCorr_internal_GGA.h Shared CPU-GPU implementation of GGA functionals

//! Available GGA functionals 
enum GGA_Variant
{	GGA_X_PBE, //!< Perdew-Burke-Ernzerhof GGA exchange
	GGA_C_PBE, //!< Perdew-Burke-Ernzerhof GGA correlation
	GGA_X_PBEsol, //!< Perdew-Burke-Ernzerhof GGA exchange reparametrized for solids
	GGA_C_PBEsol, //!< Perdew-Burke-Ernzerhof GGA correlation reparametrized for solids
	GGA_X_PW91, //!< Perdew-Wang 1991 GGA exchange
	GGA_C_PW91, //!< Perdew-Wang 1991 GGA correlation
	GGA_X_wPBE_SR, //!< Short-ranged part of omega-PBE exchange (used in HSE06 hybrid)
	GGA_X_GLLBsc, //!< Semi-local part of GLLB-sc exchange (potential-only, equal to 2x PBEsol per-particle energy)
	GGA_X_LB94, //!< van Leeuwen-Baerends asymptotically-correct exchange potential correction (potential-only)
	GGA_KE_VW,  //!< von Weisacker gradient correction to Thomas Fermi LDA kinetic energy
	GGA_KE_PW91  //!< Teter GGA kinetic energy
};

//! Common interface to the compute kernels for GGA-like functionals
class FunctionalGGA : public Functional
{
public:
	FunctionalGGA(GGA_Variant variant, double scaleFac=1.0);
	bool needsSigma() const { return true; }
	bool needsLap() const { return false; }
	bool needsTau() const { return false; }
	bool hasExchange() const
	{	switch(variant)
		{	case GGA_X_PBE:
			case GGA_X_PBEsol:
			case GGA_X_PW91:
			case GGA_X_wPBE_SR:
			case GGA_X_GLLBsc:
			case GGA_X_LB94:
				return true;
			default:
				return false;
		}
	}
	bool hasCorrelation() const
	{	switch(variant)
		{	case GGA_C_PBE:
			case GGA_C_PBEsol:
			case GGA_C_PW91:
				return true;
			default:
				return false;
		}
	}
	bool hasKinetic() const
	{	switch(variant)
		{	case GGA_KE_VW:
			case GGA_KE_PW91:
				return true;	
			default:
				return false;
		}
	}
	bool hasEnergy() const
	{	switch(variant)
		{	case GGA_X_GLLBsc:
				return false;
			default:
				return true;
		}
	}
	
	//! The thread launchers and gpu kernels for all GGAs are generated by this function
	//! using the template specializations of GGA_calc, GGA_eval
	//! Note that lap and taus are unused by GGAs
	void evaluate(int N, std::vector<const double*> n, std::vector<const double*> sigma,
		std::vector<const double*> lap, std::vector<const double*> tau,
		double* E, std::vector<double*> E_n, std::vector<double*> E_sigma,
		std::vector<double*> E_lap, std::vector<double*> E_tau) const;

private:
	GGA_Variant variant;
};

//! Switch a function fTemplate templated over GGA variant, spin scaling behavior and spin count,
//! over all supported functionals with nCount being a compile-time constant
//! NOTE: The second argument to fTemplate must correspond to the spin-scaling behavior of each functional
//! (Used by the thread and gpu launchers of FunctionalGGA::evaluate)
//! (This is needed to switch from a run-time variant to a compile-time template argument)
#define SwitchTemplate_GGA(variant,nCount, fTemplate,argList) \
	switch(variant) \
	{	case GGA_X_PBE:     fTemplate< GGA_X_PBE,     true, nCount> argList; break; \
		case GGA_C_PBE:     fTemplate< GGA_C_PBE,    false, nCount> argList; break; \
		case GGA_X_PBEsol:  fTemplate< GGA_X_PBEsol,  true, nCount> argList; break; \
		case GGA_C_PBEsol:  fTemplate< GGA_C_PBEsol, false, nCount> argList; break; \
		case GGA_X_PW91:    fTemplate< GGA_X_PW91,    true, nCount> argList; break; \
		case GGA_C_PW91:    fTemplate< GGA_C_PW91,   false, nCount> argList; break; \
		case GGA_X_wPBE_SR: fTemplate< GGA_X_wPBE_SR, true, nCount> argList; break; \
		case GGA_X_GLLBsc:  fTemplate< GGA_X_GLLBsc,  true, nCount> argList; break; \
		case GGA_X_LB94:    fTemplate< GGA_X_LB94,    true, nCount> argList; break; \
		case GGA_KE_VW:     fTemplate< GGA_KE_VW,     true, nCount> argList; break; \
		case GGA_KE_PW91:   fTemplate< GGA_KE_PW91,   true, nCount> argList; break; \
		default: break; \
	}


//! GGA interface inner layer for spin-scaling functionals (specialized for each such functional):
//! Return energy density given rs and dimensionless gradient squared s2, and set gradients w.r.t rs and s2
template<GGA_Variant variant> __hostanddev__
double GGA_eval(double rs, double s2, double& e_rs, double& e_s2);

//! GGA interface inner layer for functionals that do not spin-scale (specialized for each such functional):
//! Return energy density given rs, zeta, g(zeta) and dimensionless gradient squared t2,
//! and set gradients w.r.t rs, zeta, g(zeta) and t2 (see PW91 ref for definitions)
template<GGA_Variant variant> __hostanddev__
double GGA_eval(double rs, double zeta, double g, double t2,
	double& e_rs, double& e_zeta, double& e_g, double& e_t2);

//! GGA interface outer layer: Accumulate GGA energy density (per unit volume)
//! and its derivatives w.r.t. density and sigma (gradient contractions)
//! Uses template specializations of the appropriate version of GGA_eval
template<GGA_Variant variant, bool spinScaling, int nCount> struct GGA_calc;

//! Specialization of GGA_calc for spin-scaling functionals (exchange and KE)
template<GGA_Variant variant, int nCount>
struct GGA_calc <variant, true, nCount>
{	__hostanddev__ static
	void compute(int i, array<const double*,nCount> n, array<const double*,2*nCount-1> sigma,
		double* E, array<double*,nCount> E_n, array<double*,2*nCount-1> E_sigma, double scaleFac)
	{	//Each spin component is computed separately:
		for(int s=0; s<nCount; s++)
		{	//Scale up s-density and gradient:
 			double ns = n[s][i] * nCount;
 			if(ns < nCutoff) continue;
			//Compute dimensionless quantities rs and s2:
			double rs = pow((4.*M_PI/3.)*ns, (-1./3));
			double s2_sigma = pow(ns, -8./3) * ((0.25*nCount*nCount) * pow(3.*M_PI*M_PI, -2./3));
			double s2 = s2_sigma * sigma[2*s][i];
			//Compute energy density and its gradients using GGA_eval:
			double e_rs, e_s2, e = GGA_eval<variant>(rs, s2, e_rs, e_s2);
			//Compute gradients if required:
			if(E_n[0])
			{	//Propagate s and rs gradients to n and sigma:
				double e_n = -(e_rs*rs + 8*e_s2*s2) / (3. * n[s][i]);
				double e_sigma = e_s2 * s2_sigma;
				//Convert form per-particle to per volume:
				E_n[s][i] += scaleFac*( n[s][i] * e_n + e );
				E_sigma[2*s][i] += scaleFac*( n[s][i] * e_sigma );
			}
			E[i] += scaleFac*( n[s][i] * e );
		}
	}
};

//! Specialization of GGA_calc for functionals that do not spin-scale (correlation)
template<GGA_Variant variant, int nCount>
struct GGA_calc <variant, false, nCount>
{	__hostanddev__ static
	void compute(int i, array<const double*,nCount> n, array<const double*,2*nCount-1> sigma,
		double* E, array<double*,nCount> E_n, array<double*,2*nCount-1> E_sigma, double scaleFac)
	{
		//Compute nTot and rs, and ignore tiny densities:
		double nTot = (nCount==1) ? n[0][i] : n[0][i]+n[1][i];
		if(nTot<nCutoff) return;
		double rs = pow((4.*M_PI/3.)*nTot, (-1./3));
		
		//Compute zeta, g(zeta) and dimensionless gradient squared t2:
		double zeta = (nCount==1) ? 0. : (n[0][i] - n[1][i])/nTot;
		double g = 0.5*(pow(1+zeta, 2./3) + pow(1-zeta, 2./3));
		double t2_sigma = (pow(M_PI/3, 1./3)/16.) * pow(nTot,-7./3) / (g*g);
		double t2 = t2_sigma * ((nCount==1) ? sigma[0][i] : sigma[0][i]+2*sigma[1][i]+sigma[2][i]);
		
		//Compute per-particle energy and derivatives:
		double e_rs, e_zeta, e_g, e_t2;
		double e = GGA_eval<variant>(rs, zeta, g, t2, e_rs, e_zeta, e_g, e_t2);
		
		//Compute and store final n/sigma derivatives if required
		if(E_n[0])
		{	double e_nTot = -(e_rs*rs + 7.*e_t2*t2) / (3.*nTot); //propagate rs and t2 derivatives to nTot
			double e_sigma = e_t2 * t2_sigma; //derivative w.r.t |DnTot|^2
			
			double g_zeta = (1./3) * //Avoid singularities at zeta = +/- 1:
				( (1+zeta > nCutoff ? pow(1+zeta, -1./3) : 0.)
				- (1-zeta > nCutoff ? pow(1-zeta, -1./3) : 0.) );
			e_zeta += (e_g - 2. * e_t2*t2 / g) * g_zeta;
			
			double E_nTot = e + nTot * e_nTot;
			E_n[0][i] += scaleFac*( E_nTot - e_zeta * (zeta-1) );
			E_sigma[0][i] += scaleFac*( (nTot * e_sigma));
			if(nCount>1)
			{	E_n[1][i] += scaleFac*( E_nTot - e_zeta * (zeta+1) );
				E_sigma[1][i] += scaleFac*( (nTot * e_sigma) * 2 );
				E_sigma[2][i] += scaleFac*( (nTot * e_sigma) );
			}
		}
		E[i] += scaleFac*( nTot * e ); //energy density per volume
	}
};


//---------------------- GGA exchange implementations ---------------------------

//! Slater exchange as a function of rs (PER PARTICLE):
__hostanddev__ double slaterExchange(double rs, double& e_rs)
{	
	double rsInvMinus = -1./rs;
	double e = rsInvMinus * (0.75*pow(1.5/M_PI, 2./3));
	e_rs = rsInvMinus * e;
	return e;
}

//! PBE GGA exchange [JP Perdew, K Burke, and M Ernzerhof, Phys. Rev. Lett. 77, 3865 (1996)]
__hostanddev__ double GGA_PBE_exchange(const double kappa, const double mu,
	double rs, double s2, double& e_rs, double& e_s2)
{	//Slater exchange:
	double eSlater_rs, eSlater = slaterExchange(rs, eSlater_rs);
	//PBE Enhancement factor:
	const double kappaByMu = kappa/mu;
	double frac = -1./(kappaByMu + s2);
	double F = 1+kappa + (kappa*kappaByMu) * frac;
	double F_s2 = (kappa*kappaByMu) * frac * frac;
	//GGA result:
	e_rs = eSlater_rs * F;
	e_s2 = eSlater * F_s2;
	return eSlater * F;
}

//! PBE GGA exchange [JP Perdew, K Burke, and M Ernzerhof, Phys. Rev. Lett. 77, 3865 (1996)]
template<> __hostanddev__ double GGA_eval<GGA_X_PBE>(double rs, double s2, double& e_rs, double& e_s2)
{	return GGA_PBE_exchange(0.804, 0.2195149727645171, rs, s2, e_rs, e_s2);
}

//! PBEsol GGA exchange [JP Perdew et al, Phys. Rev. Lett. 100, 136406 (2008)]
template<> __hostanddev__ double GGA_eval<GGA_X_PBEsol>(double rs, double s2, double& e_rs, double& e_s2)
{	return GGA_PBE_exchange(0.804, 10./81, rs, s2, e_rs, e_s2);
}


//! Functional form of the PW91 Exchange Enhancement factor (also used in some KE functionals)
//! Implements equation (8) in [JP Perdew et al, Phys. Rev. B 46, 6671 (1992)] and its gradients
//! P,Q,R,S,T,U are the parameters in order of appearance in that equation
__hostanddev__ double GGA_PW91_Enhancement(double s2, double& F_s2,
	const double P, const double Q, const double R, const double S, const double T, const double U)
{	//-- Transcendental pieces and derivatives:
	double s = sqrt(s2);
	double asinhTerm = P * s * asinh(Q*s);
	double asinhTerm_s2 = (s2 ? 0.5*(asinhTerm/s2 + (P*Q)/sqrt(1+(Q*Q)*s2)) : P*Q);
	double gaussTerm = S * exp(-T*s2);
	double gaussTerm_s2 = -T * gaussTerm;
	//-- Numerator and denominator of equation (8) and derivatives:
	double num = 1. + asinhTerm + s2*(R - gaussTerm);
	double num_s2 = asinhTerm_s2 + (R - gaussTerm) - s2 * gaussTerm_s2;
	double den = 1 + asinhTerm + U*s2*s2;
	double den_s2 = asinhTerm_s2 + 2*U*s2;
	//-- Put together enhancement factor and derivative w.r.t s2:
	F_s2 = (num_s2*den - num*den_s2)/(den*den);
	return num/den;
}

//! PW91 GGA exchange [JP Perdew et al, Phys. Rev. B 46, 6671 (1992)]
template<> __hostanddev__ double GGA_eval<GGA_X_PW91>(double rs, double s2, double& e_rs, double& e_s2)
{	//Slater exchange:
	double eSlater_rs, eSlater = slaterExchange(rs, eSlater_rs);
	//Enhancement factor:
	//-- Fit parameters in order of appearance in equation (8) of reference
	const double P = 0.19645; //starting at P for PW91 :D
	const double Q = 7.7956;
	const double R = 0.2743;
	const double S = 0.1508;
	const double T = 100.;
	const double U = 0.004;
	double F_s2, F = GGA_PW91_Enhancement(s2, F_s2, P,Q,R,S,T,U);
	//GGA result:
	e_rs = eSlater_rs * F;
	e_s2 = eSlater * F_s2;
	return eSlater * F;
}



//!Evaluate \f$ \int_0^\infty dy y^n e^{-Ay^2} \textrm{erfc}{By} \f$ and its derivatives
template<int n> __hostanddev__
double integralErfcGaussian(double A, double B, double& result_A, double& result_B);

template<> __hostanddev__
double integralErfcGaussian<1>(double A, double B, double& result_A, double& result_B)
{	double invApBsq = 1./(A + B*B);
	double invsqrtApBsq = sqrt(invApBsq); //(A+B^2) ^ (-1/2)
	double inv3sqrtApBsq = invsqrtApBsq * invApBsq; //(A+B^2) ^ (-3/2)
	double invA = 1./A;
	result_A = (B*(1.5*A + B*B)*inv3sqrtApBsq - 1.) * 0.5*invA*invA;
	result_B = -0.5 * inv3sqrtApBsq;
	return (1. - B*invsqrtApBsq) * 0.5*invA;
}

template<> __hostanddev__
double integralErfcGaussian<2>(double A, double B, double& result_A, double& result_B)
{	const double invsqrtPi = 1./sqrt(M_PI);
	double invApBsq = 1./(A + B*B);
	double invA = 1./A;
	double atanTerm = atan(sqrt(A)/B)*invA/sqrt(A);
	result_A = invsqrtPi * (B*(1.25+0.75*B*B*invA)*invApBsq*invApBsq*invA - 0.75*atanTerm/A);
	result_B = -invsqrtPi * invApBsq*invApBsq;
	return 0.5*invsqrtPi* (-B*invApBsq/A +  atanTerm);
}

template<> __hostanddev__
double integralErfcGaussian<3>(double A, double B, double& result_A, double& result_B)
{	double invApBsq = 1./(A + B*B);
	double invsqrtApBsq = sqrt(invApBsq); //(A+B^2) ^ (-1/2)
	double inv3sqrtApBsq = invsqrtApBsq * invApBsq; //(A+B^2) ^ (-3/2)
	double inv5sqrtApBsq = inv3sqrtApBsq * invApBsq; //(A+B^2) ^ (-5/2)
	double invA = 1./A;
	result_A = (-invA*invA + B*(0.375*inv5sqrtApBsq + invA*(0.5*inv3sqrtApBsq + invA*invsqrtApBsq))) * invA;
	result_B = -0.75*inv5sqrtApBsq;
	return (1. - B*(invsqrtApBsq + 0.5*A*inv3sqrtApBsq)) * 0.5*invA*invA;
}

template<> __hostanddev__
double integralErfcGaussian<5>(double A, double B, double& result_A, double& result_B)
{	double invApBsq = 1./(A + B*B);
	double invsqrtApBsq = sqrt(invApBsq); //(A+B^2) ^ (-1/2)
	double inv3sqrtApBsq = invsqrtApBsq * invApBsq; //(A+B^2) ^ (-3/2)
	double inv5sqrtApBsq = inv3sqrtApBsq * invApBsq; //(A+B^2) ^ (-5/2)
	double inv7sqrtApBsq = inv5sqrtApBsq * invApBsq; //(A+B^2) ^ (-7/2)
	double invA = 1./A;
	result_A = -3.*invA* (invA*invA*invA
		- B*(0.3125*inv7sqrtApBsq + invA*(0.375*inv5sqrtApBsq
			+ invA*(0.5*inv3sqrtApBsq + invA*invsqrtApBsq))));
	result_B = -1.875*inv7sqrtApBsq;
	return invA * (invA*invA - B*(0.375*inv5sqrtApBsq + invA*(0.5*inv3sqrtApBsq + invA*invsqrtApBsq)));
}



//! Short-ranged omega-PBE GGA exchange - used in the HSE06 hybrid functional
//! [J Heyd, G E Scuseria, and M Ernzerhof, J. Chem. Phys. 118, 3865 (2003)]
template<> __hostanddev__ double GGA_eval<GGA_X_wPBE_SR>(double rs, double sIn2, double& e_rs, double& e_sIn2)
{	//Slater exchange:
	double eSlater_rs, eSlater = slaterExchange(rs, eSlater_rs);
	//omega-PBE enhancement factor:
	const double omega = 0.11; //range-parameter recommended in the HSE06 paper
	
	//Scaling of s and its derivative:
	double s2 = sIn2, s2_sIn2 = 1.;
	const double sMax = 8.572844;
	if(sIn2 > 225.)
	{	s2 = sMax*sMax;
		s2_sIn2 = 0.;
	}
	else if(sIn2 > 1.)
	{	double sIn = sqrt(sIn2);
		double expTerm = exp(sIn-sMax);
		double s = sIn - log(1. + expTerm);
		double s_sIn = 1./(1.+expTerm);
		s2 = s*s;
		s2_sIn2 = s*s_sIn/sIn;
    }
	
	//Parametrization of PBE hole [J. Perdew and M. Ernzerhof, J. Chem. Phys. 109, 3313 (1998)]
	const double A = 1.0161144;
	const double B = -0.37170836;
	const double C = -0.077215461;
	const double D = 0.57786348;
	//-- Function h := s2 H using the Pade approximant eqn (A5) for H(s)
	const double H_a1 = 0.00979681;
	const double H_a2 = 0.0410834;
	const double H_a3 = 0.187440;
	const double H_a4 = 0.00120824;
	const double H_a5 = 0.0347188;
	double s = sqrt(s2);
	double hNum = s2*s2*(H_a1 + s2*H_a2);
	double hNum_s2 = s2*(2.*H_a1 + s2*(3.*H_a2));
	double hDenInv = 1./(1. + s2*s2*(H_a3 + s*H_a4 + s2*H_a5));
	double hDen_s2 = s2*(2.*H_a3 + s*(2.5*H_a4) + s2*(3.*H_a5));
	double h = hNum * hDenInv;
	double h_s2 = (hNum_s2 - hNum*hDenInv*hDen_s2) * hDenInv;
	//-- Function f := C (1 + s2 F) in terms of h using eqn (25)
	//-- and noting that eqn (14) => (4./9)*A*A + B - A*D = -1/2
	double f = C - 0.5*h - (1./27)*s2;
	double f_s2 = -0.5*h_s2 - (1./27);
	//-- Function g := E (1 + s2 G) in terms of f and h using eqns (A1-A3)
	double Dph = D+h, sqrtDph = sqrt(Dph);
	const double gExpArg_h = 2.25/A;
	double gExpArg = gExpArg_h*h, gErfcArg = sqrt(gExpArg);
	double gExpErfcTerm = exp(gExpArg) * erfc(gErfcArg);
	double gExpErfcTerm_h = (gExpErfcTerm - (1./sqrt(M_PI))/gErfcArg) * gExpArg_h;
	double g = -Dph*(0.2*f + Dph*((4./15)*B + Dph*((8./15)*A
		+ sqrtDph*(0.8*sqrt(M_PI))*(sqrt(A)*gExpErfcTerm-1.))));
	double g_h = -(0.2*f + Dph*((8./15)*B + Dph*(1.6*A
		+ sqrtDph*(0.8*sqrt(M_PI))*(sqrt(A)*(3.5*gExpErfcTerm+Dph*gExpErfcTerm_h)-3.5))));
	double g_f = -Dph*0.2;
	double g_s2 = g_h * h_s2 + g_f * f_s2;
	
	//Accumulate contributions from each gaussian-erfc-polynomial integral:
	//(The prefactor of -8/9 in the enhancement factor is included at the end)
	const double erfcArgPrefac = omega * pow(2./(9*M_PI),1./3); //prefactor to rs in the argument to erfc
	double erfcArg = erfcArgPrefac * rs;
	double I=0., I_s2=0., I_erfcArg=0.;
	//5 fit gaussians for the 1/y part of the hole from the HSE paper:
	const double a1 = -0.000205484, b1 = 0.006601306;
	const double a2 = -0.109465240, b2 = 0.259931140;
	const double a3 = -0.064078780, b3 = 0.520352224;
	const double a4 = -0.008181735, b4 = 0.118551043;
	const double a5 = -0.000110666, b5 = 0.046003777;
	double fit1_h, fit1_erfcArg, fit1 = integralErfcGaussian<1>(b1+h, erfcArg, fit1_h, fit1_erfcArg);
	double fit2_h, fit2_erfcArg, fit2 = integralErfcGaussian<1>(b2+h, erfcArg, fit2_h, fit2_erfcArg);
	double fit3_h, fit3_erfcArg, fit3 = integralErfcGaussian<2>(b3+h, erfcArg, fit3_h, fit3_erfcArg);
	double fit4_h, fit4_erfcArg, fit4 = integralErfcGaussian<2>(b4+h, erfcArg, fit4_h, fit4_erfcArg);
	double fit5_h, fit5_erfcArg, fit5 = integralErfcGaussian<3>(b5+h, erfcArg, fit5_h, fit5_erfcArg);
	I += a1*fit1 + a2*fit2 + a3*fit3 + a4*fit4 + a5*fit5;
	I_s2 += (a1*fit1_h + a2*fit2_h + a3*fit3_h + a4*fit4_h + a5*fit5_h) * h_s2;
	I_erfcArg += a1*fit1_erfcArg + a2*fit2_erfcArg + a3*fit3_erfcArg + a4*fit4_erfcArg + a5*fit5_erfcArg;
	//Analytical gaussian terms present in the PBE hole:
	double term1_h, term1_erfcArg, term1 = integralErfcGaussian<1>(D+h, erfcArg, term1_h, term1_erfcArg);
	double term2_h, term2_erfcArg, term2 = integralErfcGaussian<3>(D+h, erfcArg, term2_h, term2_erfcArg);
	double term3_h, term3_erfcArg, term3 = integralErfcGaussian<5>(D+h, erfcArg, term3_h, term3_erfcArg);
	I += B*term1 + f*term2 + g*term3;
	I_s2 += f_s2*term2 + g_s2*term3 + (B*term1_h + f*term2_h + g*term3_h) * h_s2;
	I_erfcArg += B*term1_erfcArg + f*term2_erfcArg + g*term3_erfcArg;
	//GGA result:
	e_rs = (-8./9)* (eSlater_rs * I + eSlater * I_erfcArg * erfcArgPrefac);
	e_sIn2 = (-8./9)* eSlater * I_s2 * s2_sIn2;
	return (-8./9)* eSlater * I;
}


//! GGA part of the GLLB-sc exchange potential (derived from PBEsol exchange per-particle energy)
//! (Specialize the outer interface to fetch the PBEsol energy as the potential)
template<int nCount> struct GGA_calc <GGA_X_GLLBsc, true, nCount>
{	__hostanddev__ static
	void compute(int i, array<const double*,nCount> n, array<const double*,2*nCount-1> sigma,
		double* E, array<double*,nCount> E_n, array<double*,2*nCount-1> E_sigma, double scaleFac)
	{	//Each spin component is computed separately:
		for(int s=0; s<nCount; s++)
		{	//Scale up s-density and gradient:
 			double ns = n[s][i] * nCount;
 			if(ns < nCutoff) continue;
			//Compute dimensionless quantities rs and s2:
			double rs = pow((4.*M_PI/3.)*ns, (-1./3));
			double s2_sigma = pow(ns, -8./3) * ((0.25*nCount*nCount) * pow(3.*M_PI*M_PI, -2./3));
			double s2 = s2_sigma * sigma[2*s][i];
			//Compute energy density and its gradients using GGA_eval:
			double e_rs, e_s2, e = GGA_eval<GGA_X_PBEsol>(rs, s2, e_rs, e_s2);
			//Compute gradients if required:
			if(E_n[0]) E_n[s][i] += scaleFac*( 2*e ); //NOTE: contributes only to potential (no concept of energy, will not FD test)
			E[i] += scaleFac*( n[s][i]*e ); //So that the computed total energy is approximately that of PBEsol (only for aesthetic reasons!)
		}
	}
};

//! van Leeuwen and Baerends asymptotically-correct exchange potential [PRA 49, 2421 (1994)]
//! (Specialize the outer interface to directly set only the potential)
template<int nCount> struct GGA_calc <GGA_X_LB94, true, nCount>
{	__hostanddev__ static
	void compute(int i, array<const double*,nCount> n, array<const double*,2*nCount-1> sigma,
		double* E, array<double*,nCount> E_n, array<double*,2*nCount-1> E_sigma, double scaleFac)
	{	if(!E_n[0]) return; //only computes potential, so no point proceeding if E_n not requested
		//Each spin component is computed separately:
		for(int s=0; s<nCount; s++)
		{	//Scale up s-density and gradient:
 			double ns = n[s][i] * nCount;
 			if(ns < nCutoff) continue;
			double nsCbrt = pow(ns, 1./3);
			//Compute dimensionless gradient x = nabla n / n^(4/3):
			double x = nCount*sqrt(sigma[2*s][i]) / (nsCbrt*ns);
			//Compute potential correction:
			const double beta = 0.05;
			E_n[s][i] += scaleFac*( -beta*nsCbrt * x*x / (1. + 3*beta*x*asinh(x)) );
		}
	}
};


//---------------------- GGA correlation implementations ---------------------------

//! The H0 function (equations 13,14) of PW91 and its derivatives.
//! Also the H fuunction (equations 7,8) of PBE.
//! The notation is a mixture, picking the shortest of both references:
//! using g from PW91 (phi in PBE) and gamma from PBE (beta^2/(2*alpha) in PW91).
__hostanddev__ double PW91_H0(const double gamma,
	double beta, double g3, double t2, double ecUnif,
	double& H0_beta, double& H0_g3, double& H0_t2, double& H0_ecUnif)
{
	const double betaByGamma = beta/gamma;
	//Compute A (PBE equation (8), PW91 equation (14)) and its derivatives:
	double expArg = ecUnif/(gamma*g3);
	double expTerm = exp(-expArg);
	double A_betaByGamma = 1./(expTerm-1);
	double A = betaByGamma * A_betaByGamma;
	double A_expArg = A * A_betaByGamma * expTerm;
	double A_ecUnif = A_expArg/(gamma*g3);
	double A_g3 = -A_expArg*expArg/g3;
	//Start with the innermost rational function
	double At2 = A*t2;
	double num = 1.+At2,          num_At2 = 1.;
	double den = 1.+At2*(1.+At2), den_At2 = 1.+2*At2;
	double frac = num/den,        frac_At2 = (num_At2*den-num*den_At2)/(den*den);
	//Log of the rational function
	double logArg = 1 + betaByGamma*t2*frac;
	double logTerm = log(logArg);
	double logTerm_betaByGamma = t2*frac/logArg;
	double logTerm_t2 = betaByGamma*(frac + t2*A*frac_At2)/logArg;
	double logTerm_A = betaByGamma*t2*t2*frac_At2/logArg;
	//Final expression:
	double H0_A = gamma*g3*logTerm_A;
	H0_beta = (gamma*g3*logTerm_betaByGamma + H0_A * A_betaByGamma)/gamma;
	H0_g3 = gamma*logTerm + H0_A * A_g3;
	H0_t2 = gamma*g3*logTerm_t2;
	H0_ecUnif = H0_A * A_ecUnif;
	return gamma*g3*logTerm;
}


//! PBE GGA correlation [JP Perdew, K Burke, and M Ernzerhof, Phys. Rev. Lett. 77, 3865 (1996)]
//! If beta depends on rs (as in revTPSS), beta_rs (=dbeta/drs) is propagated to e_rs
__hostanddev__ double GGA_PBE_correlation(const double beta, const double beta_rs,
	double rs, double zeta, double g, double t2,
	double& e_rs, double& e_zeta, double& e_g, double& e_t2)
{	
	//Compute uniform correlation energy and its derivatives:
	double ecUnif_rs, ecUnif_zeta;
	double ecUnif = LDA_eval<LDA_C_PW_prec>(rs, zeta, ecUnif_rs, ecUnif_zeta);
	
	//Compute gradient correction H:
	double g2=g*g, g3 = g*g2;
	double H_beta, H_g3, H_t2, H_ecUnif;
	const double gamma = (1. - log(2.))/(M_PI*M_PI);
	double H = PW91_H0(gamma, beta, g3, t2, ecUnif, H_beta, H_g3, H_t2, H_ecUnif);
	
	//Put together final results (propagate gradients):
	e_rs = ecUnif_rs + H_ecUnif*ecUnif_rs + H_beta*beta_rs;
	e_zeta = ecUnif_zeta + H_ecUnif * ecUnif_zeta;
	e_g = H_g3 * (3*g2);
	e_t2 = H_t2;
	return ecUnif + H;
}

//! PBE GGA correlation [JP Perdew, K Burke, and M Ernzerhof, Phys. Rev. Lett. 77, 3865 (1996)]
template<> __hostanddev__ double GGA_eval<GGA_C_PBE>(double rs, double zeta, double g, double t2,
	double& e_rs, double& e_zeta, double& e_g, double& e_t2)
{	return GGA_PBE_correlation(0.06672455060314922, 0., rs, zeta, g, t2, e_rs, e_zeta, e_g, e_t2);
}

//! PBEsol GGA correlation [JP Perdew et al, Phys. Rev. Lett. 100, 136406 (2008)]
template<> __hostanddev__ double GGA_eval<GGA_C_PBEsol>(double rs, double zeta, double g, double t2,
	double& e_rs, double& e_zeta, double& e_g, double& e_t2)
{	return GGA_PBE_correlation(0.046, 0., rs, zeta, g, t2, e_rs, e_zeta, e_g, e_t2);
}

//! PW91 GGA correlation [JP Perdew et al, Phys. Rev. B 46, 6671 (1992)]
template<> __hostanddev__
double GGA_eval<GGA_C_PW91>(double rs, double zeta, double g, double t2,
	double& e_rs, double& e_zeta, double& e_g, double& e_t2)
{	
	//Rasolt-Geldart function Cxc(rs) and its derivatives:
	//Numerator (I pulled in the prefactor of 1e-3 into these)
	const double Cxc0 = 2.568e-3;
	const double aCxc = 2.3266e-2;
	const double bCxc = 7.389e-6;
	double numCxc     = Cxc0 + rs*(aCxc + rs*(bCxc));
	double numCxc_rs  = aCxc + rs*(2*bCxc);
	//Denominator
	const double cCxc = 8.723;
	const double dCxc = 0.472;
	double denCxc     = 1. + rs*(cCxc + rs*(dCxc + rs*(1e4*bCxc)));
	double denCxc_rs  = cCxc + rs*(2*dCxc + rs*(3*1e4*bCxc));
	//Pade form:
	double Cxc    = numCxc/denCxc;
	double Cxc_rs = (numCxc_rs*denCxc - numCxc*denCxc_rs)/(denCxc*denCxc);
	
	//Compute uniform correlation energy and its derivatives:
	double ecUnif_rs, ecUnif_zeta;
	double ecUnif = LDA_eval<LDA_C_PW>(rs, zeta, ecUnif_rs, ecUnif_zeta);
	
	//Compute gradient correction H1 and its derivatives:
	//Difference between C coefficients:
	const double nu = (16./M_PI) * pow(3.*M_PI*M_PI, 1./3);
	const double Cx = -1.667e-3;
	double nuCdiff = nu*(Cxc - Cxc0 - (3./7)*Cx);
	double nuCdiff_rs = nu * Cxc_rs;
	//Exponential cutoff:
	const double sigma = 100 * pow(16./(3*M_PI*M_PI), 2./3); //coefficient in exponent (converting ks/kF to rs)
	double g2=g*g, g3=g*g2, g4=g2*g2;
	double expCutoff = exp(-sigma*g4*rs*t2);
	//Put together correction:
	double H1 = nuCdiff * g3 * t2 *  expCutoff;
	double H1_rs = nuCdiff_rs  * g3 * t2 *  expCutoff - (sigma*g4*t2)*H1;
	double H1_g = nuCdiff * (3*g2) * t2 *  expCutoff - (sigma*4*g3*rs*t2)*H1;
	double H1_t2 = nuCdiff * g3 *  expCutoff - (sigma*g4*rs)*H1;
	
	//Compute gradient correction H0 and its derivatives:
	const double beta = nu*(Cxc0-Cx);
	const double alpha = 0.09;
	const double gamma = (beta*beta)/(2.*alpha);
	double H0_beta, H0_g3, H0_t2, H0_ecUnif;
	double H0 = PW91_H0(gamma, beta, g3, t2, ecUnif, H0_beta, H0_g3, H0_t2, H0_ecUnif);
	
	//Put together final results (propagate gradients):
	e_rs = ecUnif_rs + H1_rs + H0_ecUnif * ecUnif_rs;
	e_zeta = ecUnif_zeta + H0_ecUnif * ecUnif_zeta;
	e_g = H1_g + H0_g3 * (3*g2);
	e_t2 = H1_t2 + H0_t2;
	return ecUnif + H1 + H0;
}

//---------------------- GGA kinetic energy implementations ---------------------------

//! Thomas Fermi kinetic energy as a function of rs (PER PARTICLE):
__hostanddev__ double TFKinetic(double rs, double& e_rs)
{	
	double rsInv = 1./rs;
	const double KEprefac = 9.0/20.0 * pow(1.5*M_PI*M_PI, 1./3.);
	double e = KEprefac * pow(rsInv, 2.);
	e_rs = -2.0 * e * rsInv;
	return e;
}

//! von Weisacker gradient correction to Thomas Fermi LDA kinetic energy (with correct gradient expansion parameter lambda)
template<> __hostanddev__ double GGA_eval<GGA_KE_VW>(double rs, double s2, double& e_rs, double& e_s2)
{	
	//const double lambda = 1.0/9.0; //proper coefficient in gradient expansion <Sov. Phys. -- JETP 5,64 (1957)>
	const double lambda = 1.0; //original Von Weisacker correction <Z. Phys. 96, 431 (1935)>
	//const double lambda = 1.0/5.0; //Empirical "best fit" < J. Phys. Soc. Jpn. 20, 1051, (1965)>

	double rsInv = 1./rs;
	const double prefac = lambda * 3.0/4.0 * pow(1.5*M_PI*M_PI, 1./3.);
	e_s2 = prefac * pow(rsInv, 2.);
	double e = e_s2 * s2;
	e_rs = -2.0 * e * rsInv;
	return e;
}

//! PW91k GGA kinetic energy [PRB 46, 6671-6687 (1992)] parameterized by Lembarki and Chermette [PRA 50, 5328-5331 (1994)]
template<> __hostanddev__ double GGA_eval<GGA_KE_PW91>(double rs, double s2, double& e_rs, double& e_s2)
{	//Thomas-Fermi Kinetic Energy:
	double eTF_rs, eTF = TFKinetic(rs, eTF_rs);
	//Enhancement factor of the PW91 form:
	//-- Fit parameters (in order of appearance in equation (8) of PW91 Exchange functional reference)
	const double P = 0.093907;
	const double Q = 76.320;
	const double R = 0.26608;
	const double S = 0.0809615;
	const double T = 100.; 
	const double U = 0.57767e-4;
	double F_s2, F = GGA_PW91_Enhancement(s2, F_s2, P,Q,R,S,T,U);
	//GGA result:
	e_rs = eTF_rs * F;
	e_s2 = eTF * F_s2;
	return eTF * F;
}

//! @}
#endif // JDFTX_ELECTRONIC_EXCORR_INTERNAL_GGA_H
