/*
 * BinaryOperator.cxx
 *
 *  Created on: Feb 26, 2013
 *      Author: dloti
 */

#include "BinaryOperator.hxx"

BinaryOperator::BinaryOperator(char op) { this->op = op; left = NULL; right = NULL;}



void BinaryOperator::SetLeft(CompoundConcept* left) {
	this->left = left;
}

void BinaryOperator::SetRight(CompoundConcept* right) {
	this->right = right;
}

CompoundConcept* BinaryOperator::GetLeft() {
	return this->left;
}
CompoundConcept* BinaryOperator::GetRight() {
	return this->right;
}


BinaryOperator::~BinaryOperator() {
	// TODO Auto-generated destructor stub
}

