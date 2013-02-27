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
	char op;
	CompoundConcept *left,*right;
public:
	BinaryOperator(char op);
	void SetLeft(CompoundConcept* left);
	void SetRight(CompoundConcept* right);
	CompoundConcept* GetLeft();
	CompoundConcept* GetRight();
	void print(std::ostream& s) const {s<<op;}
	void infix(std::ostream& s) const { s << '('; left->infix(s); s << ')'; print(s); s << '('; right->infix(s); s << ')'; }
	void UpdateInterpretation() {//TODO
	}
	std::vector<int>* GetInterpretation(){return new std::vector<int>();};
	virtual ~BinaryOperator();
};

#endif /* BINARYOPERATOR_HXX_ */
