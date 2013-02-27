/*
 * ConceptNode.hxx
 *
 *  Created on: Feb 14, 2013
 *      Author: dloti
 */

#ifndef CONCEPTNODE_HXX_
#define CONCEPTNODE_HXX_

#include "Operand.hxx"

class ConceptNode : public Operand {
private:
	std::vector<int> interpretation;
	ConceptNode* left;
	ConceptNode* right;

public:
	ConceptNode();
	ConceptNode(std::string predicate);
	bool IsGoal();
	void IsGoal(bool goal);
	//bool IsSimple();
	//void IsSimple(bool simple);
	//bool SimpleValue();
	//void SimpleValue(bool simpleValue);
//	void SetLeft(ConceptNode* left);
//	void SetRight(ConceptNode* right);
//	ConceptNode* GetLeft();
//	ConceptNode* GetRight();

	virtual ~ConceptNode();
	inline std::string GetPredicate() {return predicate;};
	//inline void SetPredicate(std::string predicate) {this->predicate = predicate};
	void UpdateInterpretation();
	void ClearInterpretation();
	std::vector<int>* GetInterpretation();
	//friend std::ostream& operator<< (std::ostream &out, ConceptNode &cNode);
	void PrintConcept();
};

#endif /* CONCEPTNODE_HXX_ */
