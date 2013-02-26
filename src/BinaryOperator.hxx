/*
 * BinaryOperator.hxx
 *
 *  Created on: Feb 26, 2013
 *      Author: dloti
 */

#ifndef BINARYOPERATOR_HXX_
#define BINARYOPERATOR_HXX_
#include "Operator.hxx"
class BinaryOperator : public Operator {
protected:
	CompoundConcept *left,*right;
public:
	BinaryOperator();
	virtual ~BinaryOperator();
};

#endif /* BINARYOPERATOR_HXX_ */
