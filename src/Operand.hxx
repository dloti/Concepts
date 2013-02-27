/*
 * Operand.hxx
 *
 *  Created on: Feb 26, 2013
 *      Author: dloti
 */

#ifndef OPERAND_HXX_
#define OPERAND_HXX_
#include "CompoundConcept.hxx"

class Operand : public CompoundConcept {
protected:
	bool goal;
	std::string predicate;
public:
	Operand();
	void print(std::ostream& s) const {s << predicate;}
	void infix(std::ostream& s) const { print(s);}
	virtual ~Operand();
};

#endif /* OPERAND_HXX_ */
