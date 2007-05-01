/***************************************************************************
 *   Copyright (C) 2004 by Bo Peng                                         *
 *   bpeng@rice.edu
 *                                                                         *
 *   $LastChangedDate$
 *   $Rev$                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _TAGGER_H
#define _TAGGER_H
/**
\file
\brief head file of class tagger: public Operator
*/
#include "operator.h"

const string TAG_InheritFields[2] = {"paternal_tag", "maternal_tag"};
const string TAG_ParentsFields[2] = {"father_idx", "mother_idx"};

namespace simuPOP
{
	/**
	\brief  tagger is a during mating operator that
	tag individual with various information. Potential usages are
	1. record parenting information to track pedigree.
	2. tag a individual/allele and monitor its spread in the population
	   etc.
	3...

	@author Bo Peng
	*/

	class tagger: public Operator
	{

		public:
			/// constructor. default to be always active but no output.
			tagger( int begin=0, int end=-1, int step=1, vectorl at=vectorl(),
				int rep=REP_ALL, int grp=GRP_ALL,
			// this is not nice, but is the only way I know how to initialize this array.
				const vectorstr& infoFields=vectorstr()):
			Operator("", "", DuringMating, begin, end, step, at, rep, grp, infoFields)
			{
			};

			/// destructor
			virtual ~tagger(){};

			virtual Operator* clone() const
			{
				return new tagger(*this);
			}
	};

	/// inherite tag from parents.
	/// If both parents have tags, use fathers.

	class inheritTagger: public tagger
	{
		public:
#define TAG_Paternal   0
#define TAG_Maternal   1
#define TAG_Both       2

		public:
			/// constructor. default to be always active.
			inheritTagger(int mode=TAG_Paternal, int begin=0, int end=-1, int step=1,
				vectorl at=vectorl(), int rep=REP_ALL, int grp=GRP_ALL,
				const vectorstr& infoFields=vectorstr(TAG_InheritFields, TAG_InheritFields+2)):
			tagger( begin, end, step, at, rep, grp, infoFields), m_mode(mode)
			{
				DBG_ASSERT(infoSize() == 2, ValueError,
					"Inherit tagger needs to know the information fields of both parents");
			};

			virtual ~inheritTagger()
			{
			}

			virtual string __repr__()
			{
				return "<simuPOP::inherittagger>" ;
			}

			virtual bool applyDuringMating(population& pop, population::IndIterator offspring,
				individual* dad=NULL, individual* mom=NULL);

			virtual Operator* clone() const
			{
				return new inheritTagger(*this);
			}

		private:
			/// mode can be
			/// TAG_Paternal: get dad's info
			/// TAG_Maternal: get mon's info
			/// TAG_BOTH:     get parents' first field
			int m_mode;
	};

	/// inherite tag from parents.
	/// If both parents have tags, use fathers.
	///

	class parentsTagger: public tagger
	{
		public:
			/// constructor. default to be always active.
			/// string can be any string (m_Delimiter will be ignored for this class.)
			///  %r will be replicate number %g will be generation number.
			parentsTagger( int begin=0, int end=-1, int step=1, vectorl at=vectorl(), int rep=REP_ALL, int grp=GRP_ALL,
				const vectorstr& infoFields=vectorstr(TAG_ParentsFields, TAG_ParentsFields+2)):
			tagger( begin, end, step, at, rep, grp, infoFields)
			{
			};

			virtual ~parentsTagger()
			{
			}

			virtual Operator* clone() const
			{
				return new parentsTagger(*this);
			}

			virtual string __repr__()
			{
				return "<simuPOP::parentstagger>" ;
			}

			virtual bool applyDuringMating(population& pop, population::IndIterator offspring,
				individual* dad=NULL, individual* mom=NULL);
	};

	/** This tagger take some information fields from both parents, pass to a python function
		and set individual field with the return value.

		This operator can be used to trace the inheritance of trait values.
	*/
	class pyTagger: public tagger
	{
		public:
			/** \param infoFields information fields. The user should gurantee the existence of
				these fields.
				\param func a pyton function that return a list to assign the information fields.
					e.g. if fields=['A', 'B'], the function will pass values of fields 'A' and
					'B' of father, followed by mother if there is one, to this function. The returned value
					is assigned to fields 'A' and 'B' of the offspring. The returned value
					has to be a list even if only one field is given.
			*/				
			pyTagger(PyObject * func=NULL, int begin=0, int end=-1, 
				int step=1, vectorl at=vectorl(), int rep=REP_ALL, int grp=GRP_ALL,
				const vectorstr& infoFields=vectorstr()):
			tagger( begin, end, step, at, rep, grp, infoFields)
			{	
				DBG_FAILIF(infoSize() == 0, ValueError,
					"infoFields can not be empty.");
			
				DBG_ASSERT(PyCallable_Check(func), ValueError,
					"Passed variable is not a callable python function.");

				Py_XINCREF(func);
				m_func = func;
			};

			virtual ~pyTagger()
			{	
				if( m_func != NULL )
					Py_DECREF(m_func);
			}

			/// CPPONLY
			pyTagger(const pyTagger & rhs):
				tagger(rhs), m_func(rhs.m_func)
			{
				if (m_func != NULL)
					Py_INCREF(m_func);
			}
			
			virtual Operator* clone() const
			{
				return new pyTagger(*this);
			}

			virtual string __repr__()
			{
				return "<simuPOP::pyTagger>" ;
			}

			virtual bool applyDuringMating(population& pop, population::IndIterator offspring,
				individual* dad=NULL, individual* mom=NULL);
				
		private:

			PyObject * m_func;
	};
}
#endif
