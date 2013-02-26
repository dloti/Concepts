/*
 * UnaryOperator.hxx
 *
 *  Created on: Feb 26, 2013
 *      Author: dloti
 */

#ifndef UNARYOPERATOR_HXX_
#define UNARYOPERATOR_HXX_
#include "Operator.hxx"
class UnaryOperator : public Operator {
protected:
	CompoundConcept* child;
public:
	UnaryOperator();
	virtual ~UnaryOperator();
};

#endif /* UNARYOPERATOR_HXX_ */
