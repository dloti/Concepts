/*
 * Rule.hxx
 *
 *  Created on: Feb 1, 2013
 *      Author: dloti
 */

#ifndef RULE_HXX_
#define RULE_HXX_
#include "Expression.hxx"
//#include<iostream>
#include <planning/Action.hxx>
class Rule {
protected:
	int coverage, correct;
	std::vector<Expression*> concepts;
	aig_tk::Action* action;
public:
	Rule(aig_tk::Action* action);
	bool AddConcept(Expression* concept);
	std::vector<Expression*> GetConcepts();

	aig_tk::Action* GetAction();
	inline int GetMaxConcepts(){ if(action== NULL) return 0; return action->pddl_objs_idx().size();}
	inline int GetCoverage(){ return coverage;}
	inline int GetCorrect(){ return correct;}
	inline void IncCoverage(){ coverage++;}
	inline void IncCorrect(){ correct++;}
	int GetCurrentCoverage();
	friend std::ostream& operator<< (std::ostream &out, Rule &r);
	friend bool operator== (Rule &r1, Rule &r2);
	friend bool operator!= (Rule &r1, Rule &r2);
	virtual ~Rule();
};

#endif /* RULE_HXX_ */
