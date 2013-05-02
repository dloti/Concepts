/*
 * Equality.cxx
 *
 *  Created on: Mar 11, 2013
 *      Author: dloti
 */

#include "Equality.hxx"

Equality::Equality(Expression* left, Expression* right) :
		BinaryOperator('=') {
	this->SetLeft(left);
	this->SetRight(right);

}

std::vector<int>* Equality::GetInterpretation() {
	this->UpdateInterpretation();
	return &(this->interpretation);
}

void Equality::UpdateInterpretation() {
	this->left->UpdateInterpretation();
	this->right->UpdateInterpretation();
	ClearInterpretation();
	std::vector<std::pair<int, int> >* leftInterpretation =
			(this->left)->GetRoleInterpretation();
	std::vector<std::pair<int, int> >* rightInterpretation =
			(this->right)->GetRoleInterpretation();

	if (leftInterpretation == NULL || leftInterpretation->size() == 0
			|| rightInterpretation == NULL || rightInterpretation->size() == 0)
		return;

	std::vector<std::pair<int, int> >::iterator first =
			rightInterpretation->begin(), last = rightInterpretation->end(),
			first1 = leftInterpretation->begin(), last1 =
					leftInterpretation->end();
	if (leftInterpretation->size() < rightInterpretation->size()) {
		first1 = rightInterpretation->begin();
		last1 = rightInterpretation->end();
		first = leftInterpretation->begin();
		last = leftInterpretation->end();
	}
	const std::vector<std::pair<int, int> >::iterator tmp = first1;
	for (; first != last; ++first) {
		for (first1 = tmp; first1 != last1; ++first1) {
			if (first->first == first1->first
					&& first->second == first1->second) {
				this->interpretation.push_back(first->first);
				break;
			}
		}
	}
}

void Equality::SetLeft(Expression* left) {
	this->left = left;
}

void Equality::SetRight(Expression* right) {
	this->right = right;
}

Equality::~Equality() {
	// TODO Auto-generated destructor stub
}

