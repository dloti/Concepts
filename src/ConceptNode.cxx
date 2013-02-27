/*
 * ConceptNode.cxx
 *
 *  Created on: Feb 14, 2013
 *      Author: dloti
 */

#include "ConceptNode.hxx"
#include <algorithm>
ConceptNode::ConceptNode() {
	left = NULL;
	right = NULL;
	this->goal = false;
	this->predicate = "";
}


ConceptNode::ConceptNode(std::string predicate) {
	left = NULL;
	right = NULL;
	this->goal = false;
	this->predicate = predicate;
}


bool ConceptNode::IsGoal() {
	return goal;
}

void ConceptNode::IsGoal(bool goal) {
	this->goal = goal;
}

//bool ConceptNode::IsSimple() {
//	return simple;
//}
//
//void ConceptNode::IsSimple(bool simple) {
//	this->simple = simple;
//}

//bool ConceptNode::SimpleValue(){
//	return this->simpleValue;
//}
//
//void ConceptNode::SimpleValue(bool simpleValue) {
//	this->simpleValue = simpleValue;
//}

ConceptNode::~ConceptNode() {
}

//void ConceptNode::SetLeft(ConceptNode* left) {
//	this->left = left;
//}
//
//void ConceptNode::SetRight(ConceptNode* right) {
//	this->right = right;
//}
//
//ConceptNode* ConceptNode::GetLeft() {
//	return this->left;
//}
//ConceptNode* ConceptNode::GetRight() {
//	return this->right;
//}

void ConceptNode::UpdateInterpretation() {
	if (this->left != NULL && this->right != NULL) {
		this->left->UpdateInterpretation();
		this->right->UpdateInterpretation();
		int sizel = this->left->GetInterpretation()->size();
		int sizer = this->right->GetInterpretation()->size();
		std::vector<int>::iterator it, it1, end, end1;

		std::vector<int>::iterator first1 =
				this->left->GetInterpretation()->begin();
		std::vector<int>::iterator last1 =
				this->left->GetInterpretation()->end();
		std::vector<int>::iterator first2 =
				this->right->GetInterpretation()->begin();
		std::vector<int>::iterator last2 =
				this->right->GetInterpretation()->end();
		this->ClearInterpretation();
		std::set_intersection(first1, last1, first2, last2,
				std::back_inserter(this->interpretation));
}
}

void ConceptNode::ClearInterpretation() {
	this->interpretation.clear();
}

std::vector<int>* ConceptNode::GetInterpretation() {
	return &(this->interpretation);
}

void ConceptNode::PrintConcept() {

	//if(this->simple){ (!this->simpleValue)?std::cout<<"!":std::cout<<"";std::cout<<this->predicate; return;}
	if ((this->left) == NULL && (this->right) == NULL) {
		std::cout << this->GetPredicate();
	} else {
		std::cout << "(";
		this->left->PrintConcept();
		std::cout << "^";
		this->right->PrintConcept();
		std::cout << ")";
	}
}
//std::ostream& operator<< (std::ostream &out, ConceptNode &cNode)
//{
//	out<<cNode.left;
//	if(cNode.left==NULL && cNode.right==NULL)
//	out<< "(" << cNode.left->predicate <<"^"<<cNode.right->predicate;
//	out<<cNode.right;
//    return out;
//}

