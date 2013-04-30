/*
 * Rule.cxx
 *
 *  Created on: Feb 1, 2013
 *      Author: dloti
 */

#include "Rule.hxx"

Rule::Rule(aig_tk::Action* action) {
	this->action = action;
	coverage = 0;
	correct = 0;
	examples=1;
}

aig_tk::Action* Rule::GetAction() {
	return this->action;
}

bool Rule::AddConcept(Expression* concept) {
	if (concepts.size() >= action->pddl_objs_idx().size())
		return false;
	concepts.push_back(concept);
	return true;
}

std::vector<Expression*> Rule::GetConcepts() {
	return this->concepts;
}

bool operator ==(Rule& r1, Rule& r2) {
	bool allConcepts = true;
	if (r1.concepts.size() == r2.concepts.size()) {
		for (unsigned i = 0; i < r1.concepts.size(); i++) {
			if (r1.concepts[i] != r2.concepts[i]) {
				allConcepts = false;
				break;
			}
		}
	} else
		allConcepts = false;
	return (allConcepts && (r1.action->name() == r2.action->name()));
}

bool operator !=(Rule& r1, Rule& r2) {
	return !(r1 == r2);
}

std::ostream& operator <<(std::ostream& out, Rule& r) {
	for (unsigned i = 0; i < r.concepts.size(); i++) {
		r.concepts[i]->infix(out);
		out << ";";
	}
	out << " : " << r.action->name();
	return out;
}

int Rule::GetCurrentCoverage() {
	int currentCoverage = 0;
	for (unsigned i = 0; i < this->concepts.size(); i++) {
		currentCoverage += this->concepts[i]->GetInterpretation()->size();
	}
	return currentCoverage;
}

Rule::~Rule() {
	// TODO Auto-generated destructor stub
}

