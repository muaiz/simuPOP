
/**
 *  $File: sandbox.cpp $
 *  $LastChangedDate: 2010-06-04 13:29:09 -0700 (Fri, 04 Jun 2010) $
 *  $Rev: 3579 $
 *
 *  This file is part of simuPOP, a forward-time population genetics
 *  simulation environment. Please visit http://simupop.sourceforge.net
 *  for details.
 *
 *  Copyright (C) 2004 - 2010 Bo Peng (bpeng@mdanderson.org)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "sandbox.h"

namespace simuPOP {


double InfSitesSelector::indFitness(Population & pop, Individual * ind) const
{
	if (m_mode == MULTIPLICATIVE) {
		return randomSelMulFitnessExt(ind->genoBegin(), ind->genoEnd());
	} else if (m_mode == ADDITIVE) {
		if (m_additive)
			return randomSelAddFitness(ind->genoBegin(), ind->genoEnd());
		else
			return randomSelAddFitnessExt(ind->genoBegin(), ind->genoEnd());
	} else if (m_mode == EXPONENTIAL) {
		if (m_additive)
			return randomSelExpFitness(ind->genoBegin(), ind->genoEnd());
		else
			return randomSelExpFitnessExt(ind->genoBegin(), ind->genoEnd());
	}
	return 0;
}


bool InfSitesSelector::apply(Population & pop) const
{
	m_newMutants.clear();
	if (!BaseSelector::apply(pop))
		return false;
	// output NEW mutant...
	if (!m_newMutants.empty() && !noOutput()) {
		ostream & out = getOstream(pop.dict());
		vectoru::const_iterator it = m_newMutants.begin();
		vectoru::const_iterator it_end = m_newMutants.end();
		for (; it != it_end; ++it) {
			SelCoef s = m_selFactory[*it];
			out << *it << '\t' << s.first << '\t' << s.second << '\n';
		}
		closeOstream();
	}
	return true;
}


InfSitesSelector::SelCoef InfSitesSelector::getFitnessValue(int mutant) const
{
	int sz = m_selDist.size();
	double s = 0;
	double h = 0.5;

	if (sz == 0) {
		// call a function
		const pyFunc & func = m_selDist.func();
		PyObject * res;
		if (func.numArgs() == 0)
			res = func("()");
		else {
			DBG_FAILIF(func.arg(0) != "loc", ValueError,
				"Only parameter loc is accepted for this user-defined function.");
			res = func("(i)", mutant);
		}
		if (PyNumber_Check(res)) {
			s = PyFloat_AsDouble(res);
		} else if (PySequence_Check(res)) {
			int sz = PySequence_Size(res);
			DBG_FAILIF(sz == 0, RuntimeError, "Function return an empty list.");
			PyObject * item = PySequence_GetItem(res, 0);
			s = PyFloat_AsDouble(item);
			Py_DECREF(item);
			if (sz > 1) {
				item = PySequence_GetItem(res, 1);
				h = PyFloat_AsDouble(item);
				Py_DECREF(item);
			}
		}
		Py_DECREF(res);
		if (m_additive && h != 0.5)
			m_additive = false;
		return SelCoef(s, h);
	}

	int mode = static_cast<int>(m_selDist[0]);
	if (mode == CONSTANT) {
		// constant
		s = m_selDist[1];
		if (m_selDist.size() > 2)
			h = m_selDist[2];
	} else {
		// a gamma distribution
		s = getRNG().randGamma(m_selDist[1], m_selDist[2]);
		if (m_selDist.size() > 3)
			h = m_selDist[3];
	}
	m_selFactory[mutant] = SelCoef(s, h);
	m_newMutants.push_back(mutant);
	if (m_additive && h != 0.5)
		m_additive = false;
	return SelCoef(s, h);
}


double InfSitesSelector::randomSelAddFitness(GenoIterator it, GenoIterator it_end) const
{
	double s = 0;

	for (; it != it_end; ++it) {
		if (*it == 0)
			continue;
		SelMap::iterator sit = m_selFactory.find(static_cast<unsigned int>(*it));
		if (sit == m_selFactory.end())
			s += getFitnessValue(*it).first / 2.;
		else
			s += sit->second.first / 2;
	}
	return 1 - s > 0 ? 1 - s : 0;
}


double InfSitesSelector::randomSelExpFitness(GenoIterator it, GenoIterator it_end) const
{
	double s = 0;

	for (; it != it_end; ++it) {
		if (*it == 0)
			continue;
		SelMap::iterator sit = m_selFactory.find(static_cast<unsigned int>(*it));
		if (sit == m_selFactory.end())
			s += getFitnessValue(*it).first / 2.;
		else
			s += sit->second.first / 2;
	}
	return exp(-s);
}


double InfSitesSelector::randomSelMulFitnessExt(GenoIterator it, GenoIterator it_end) const
{
	MutCounter cnt;

	for (; it != it_end; ++it) {
		if (*it == 0)
			continue;
		MutCounter::iterator mit = cnt.find(*it);
		if (mit == cnt.end())
			cnt[*it] = 1;
		else
			++mit->second;
	}

	double s = 1;
	MutCounter::iterator mit = cnt.begin();
	MutCounter::iterator mit_end = cnt.end();
	for (; mit != mit_end; ++mit) {
		SelMap::iterator sit = m_selFactory.find(mit->first);
		if (sit == m_selFactory.end()) {
			SelCoef sf = getFitnessValue(mit->first);
			if (mit->second == 1)
				s *= 1 - sf.first * sf.second;
			else
				s *= 1 - sf.first;
		} else {
			if (mit->second == 1)
				s *= 1 - sit->second.first * sit->second.second;
			else
				s *= 1 - sit->second.first;
		}
	}
	return s;
}


double InfSitesSelector::randomSelAddFitnessExt(GenoIterator it, GenoIterator it_end) const
{
	MutCounter cnt;

	for (; it != it_end; ++it) {
		if (*it == 0)
			continue;
		MutCounter::iterator mit = cnt.find(*it);
		if (mit == cnt.end())
			cnt[*it] = 1;
		else
			++mit->second;
	}

	double s = 0;
	MutCounter::iterator mit = cnt.begin();
	MutCounter::iterator mit_end = cnt.end();
	for (; mit != mit_end; ++mit) {
		SelMap::iterator sit = m_selFactory.find(mit->first);
		if (sit == m_selFactory.end()) {
			SelCoef sf = getFitnessValue(mit->first);
			if (mit->second == 1)
				s += sf.first * sf.second;
			else
				s += sf.first;
		} else {
			if (mit->second == 1)
				s += sit->second.first * sit->second.second;
			else
				s += sit->second.first;
		}
	}
	return 1 - s > 0 ? 1 - s : 0;
}


double InfSitesSelector::randomSelExpFitnessExt(GenoIterator it, GenoIterator it_end) const
{
	MutCounter cnt;

	for (; it != it_end; ++it) {
		if (*it == 0)
			continue;
		MutCounter::iterator mit = cnt.find(*it);
		if (mit == cnt.end())
			cnt[*it] = 1;
		else
			++mit->second;
	}

	double s = 0;
	MutCounter::iterator mit = cnt.begin();
	MutCounter::iterator mit_end = cnt.end();
	for (; mit != mit_end; ++mit) {
		SelMap::iterator sit = m_selFactory.find(mit->first);
		if (sit == m_selFactory.end()) {
			SelCoef sf = getFitnessValue(mit->first);
			if (mit->second == 1)
				s += sf.first * sf.second;
			else
				s += sf.first;
		} else {
			if (mit->second == 1)
				s += sit->second.first * sit->second.second;
			else
				s += sit->second.first;
		}
	}
	return exp(-s);
}


bool InfSitesMutator::apply(Population & pop) const
{
	const matrixi & ranges = m_ranges.elems();
	vectoru width(ranges.size());

	width[0] = ranges[0][1] - ranges[0][0];
	for (size_t i = 1; i < width.size(); ++i)
		width[i] = ranges[i][1] - ranges[i][0] + width[i - 1];

	ULONG ploidyWidth = width.back();
	ULONG indWidth = pop.ploidy() * ploidyWidth;

	ostream * out = NULL;
	if (!noOutput())
		out = &getOstream(pop.dict());

	subPopList subPops = applicableSubPops(pop);
	subPopList::const_iterator sp = subPops.begin();
	subPopList::const_iterator spEnd = subPops.end();
	for (; sp != spEnd; ++sp) {
		DBG_FAILIF(sp->isVirtual(), ValueError, "This operator does not support virtual subpopulation.");
		for (size_t indIndex = 0; indIndex < pop.subPopSize(sp->subPop()); ++indIndex) {
			ULONG loc = 0;
			while (true) {
				// using a geometric distribution to determine mutants
				loc += getRNG().randGeometric(m_rate);
				if (loc > indWidth)
					break;
				Individual & ind = pop.individual(indIndex);
				int p = (loc - 1) / ploidyWidth;
				// chromosome and position on chromosome?
				ULONG mutLoc = (loc - 1) - p * ploidyWidth;
				size_t ch = 0;
				for (size_t reg = 0; reg < width.size(); ++reg) {
					if (mutLoc < width[reg]) {
						ch = reg;
						break;
					}
				}
				mutLoc += ranges[ch][0];
				if (ch > 0)
					mutLoc -= width[ch - 1];

				if (m_model == 1) {
					// under an infinite-site model
					if (m_mutants.find(mutLoc) != m_mutants.end()) {
						// there is an existing allele
						if (find(pop.genoBegin(false), pop.genoEnd(false), ToAllele(mutLoc)) != pop.genoEnd(false)) {
							// hit an exiting locus, return
							if (out)
								(*out) << pop.gen() << '\t' << mutLoc << '\t' << indIndex << "\t2\n";
							continue;
						}
						// if there is no existing mutant, new mutant is allowed
					} else
						m_mutants.insert(mutLoc);
				}
				GenoIterator geno = ind.genoBegin(p, ch);
				size_t nLoci = pop.numLoci(ch);
				if (*(geno + nLoci - 1) != 0) {
					// if the number of mutants at this individual exceeds reserved numbers
					DBG_DO(DBG_MUTATOR, cerr << "Adding 10 loci to region " << ch << endl);
					vectorf added(10);
					for (size_t j = 0; j < 10; ++j)
						added[j] = nLoci + j + 1;
					vectoru addedChrom(10, ch);
					pop.addLoci(addedChrom, added);
					// individual might be shifted...
					ind = pop.individual(indIndex);
					geno = ind.genoBegin(p, ch);
					nLoci += 10;
				}
				// find the first non-zero location
				for (size_t j = 0; j < nLoci; ++j) {

					if (*(geno + j) == 0) {
						// record mutation here
						DBG_FAILIF(mutLoc >= ModuleMaxAllele, RuntimeError,
							"Location can not be saved because it exceed max allowed allele.");
						*(geno + j) = ToAllele(mutLoc);
						if (out)
							(*out) << pop.gen() << '\t' << mutLoc << '\t' << indIndex << "\t0\n";

						break;
					} else if (static_cast<ULONG>(*(geno + j)) == mutLoc) {
						// back mutation
						//  from A b c d 0
						//  to   d b c d 0
						//  to   d b c 0 0
						for (size_t k = j + 1; k < nLoci; ++k)
							if (*(geno + k) == 0) {
								*(geno + j) = *(geno + k - 1);
								*(geno + k - 1) = 0;
								if (out)
									(*out) << pop.gen() << '\t' << mutLoc << '\t' << indIndex << "\t1\n";
								break;
							}
						DBG_DO(DBG_MUTATOR, cerr << "Back mutation happens at generation " << pop.gen() << " on individual " << indIndex << endl);
						break;
					}
				}
			}   // while
		}       // each individual
	}           // each subpopulation
	if (out)
		closeOstream();
	return true;
}


void InfSitesRecombinator::transmitGenotype1(const Individual & parent, Individual & offspring, int ploidy) const
{
}


void InfSitesRecombinator::transmitGenotype2(const Individual & parent, Individual & offspring, int ploidy) const
{
}


bool InfSitesRecombinator::applyDuringMating(Population & pop, Population & offPop,
		RawIndIterator offspring,
		Individual * dad, Individual * mom) const
{
	// if offspring does not belong to subPops, do nothing, but does not fail.
	if (!applicableToAllOffspring() && !applicableToOffspring(offPop, offspring))
		return true;

	initializeIfNeeded(*offspring);

	// standard genotype transmitter
	if (m_rate == 0) {
		for (int ch = 0; static_cast<UINT>(ch) < pop.numChrom(); ++ch) {
			copyChromosome(*mom, getRNG().randBit(), *offspring, 0, ch);
			copyChromosome(*dad, getRNG().randBit(), *offspring, 1, ch);
		}
	} else if (m_rate == 0.5) {
	} else if (m_rate < 1e-4) {
		transmitGenotype1(*mom, *offspring, 0);
		transmitGenotype1(*dad, *offspring, 1);
	} else {
		transmitGenotype2(*mom, *offspring, 0);
		transmitGenotype2(*dad, *offspring, 1);
	}
	return true;
}

}
