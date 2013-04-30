#include <planning/FF_PDDL_To_STRIPS.hxx>
#include <planning/STRIPS_Problem.hxx>
#include <planning/Fluent.hxx>
#include <planning/Action.hxx>
#include <planning/PDDL_Type.hxx>
#include <planning/PDDL_Object.hxx>
#include <planning/PDDL_Operator.hxx>
#include <planning/heuristics/Max_Heuristic.hxx>
#include <planning/heuristics/Relaxed_Plan_Extractor.hxx>
#include <planning/Types.hxx>
#include <planning/inference/Propagator.hxx>

#include <search/Node.hxx>
#include <search/Best_First_Search.hxx>

#include <util/time.hxx>
#include <util/ext_math.hxx>

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <typeinfo>
#include <cmath>
#include "Expression.hxx"
#include "ConceptNode.hxx"
#include "RoleNode.hxx"
#include "BinaryOperator.hxx"
#include "Join.hxx"
#include "Not.hxx"
#include "InverseRole.hxx"
#include "TransitiveClosure.hxx"
#include "Equality.hxx"
#include "ValueRestriction.hxx"
#include "Rule.hxx"

using namespace aig_tk;
using namespace std;

map<string, RoleNode*> primitiveRoles;
map<string, RoleNode*> goalRoles;
map<string, ConceptNode*> goalConcepts;
map<string, ConceptNode*> primitiveConcepts;
map<string, ConceptNode*> typeConcepts;
vector<Expression*> rootConcepts;
vector<Expression*> rootRoles;
vector<vector<int> > objectSubsets;
vector<Rule> ruleSet;
PDDL_Object_Ptr_Vec instanceObjects;

void initialize_root_concepts() {
	if (rootConcepts.size() > 0 && rootRoles.size() > 0)
		return;
	map<string, ConceptNode*>::iterator conceptIt;
	map<string, RoleNode*>::iterator roleIt;
	for (conceptIt = primitiveConcepts.begin(); conceptIt != primitiveConcepts.end(); ++conceptIt) {
		rootConcepts.push_back(conceptIt->second);
	}
	for (conceptIt = typeConcepts.begin(); conceptIt != typeConcepts.end(); ++conceptIt) {
		rootConcepts.push_back(conceptIt->second);
	}
	for (conceptIt = goalConcepts.begin(); conceptIt != goalConcepts.end(); ++conceptIt) {
		rootConcepts.push_back(conceptIt->second);
	}
	for (roleIt = primitiveRoles.begin(); roleIt != primitiveRoles.end(); ++roleIt) {
		rootRoles.push_back(roleIt->second);
	}
	for (roleIt = goalRoles.begin(); roleIt != goalRoles.end(); ++roleIt) {
		rootRoles.push_back(roleIt->second);
	}
}

bool concept_allowed(Expression* operationExpr, vector<Expression*>* candidateConcepts,
		Expression* childConcept = NULL) {
	if (operationExpr->GetInterpretation()->size() == 0)
		return false;
//	if (childConcept != NULL) {
//		if (typeid(*childConcept) == typeid(Not))
//			return false;
//		else if (typeid(*childConcept) == typeid(BinaryOperator)) {
//			if (((BinaryOperator*) operationExpr)->GetLeft() == ((BinaryOperator*) operationExpr)->GetRight())
//				return false;
//		}
//	}

	for (unsigned i = 0; i < rootConcepts.size(); i++) {
		vector<int>* rootInterp = rootConcepts[i]->GetInterpretation();
		if (rootInterp->size() == 0)
			continue;
		vector<int>* candidateInterp = operationExpr->GetInterpretation();
		vector<int> intersect;
		set_intersection(rootInterp->begin(), rootInterp->end(), candidateInterp->begin(), candidateInterp->end(),
				back_inserter(intersect));
		if (intersect.size() == rootInterp->size())
			return false;
	}

	for (unsigned i = 0; i < candidateConcepts->size(); i++) {
		vector<int>* cansInterp = (*candidateConcepts)[i]->GetInterpretation();
		if (cansInterp->size() == 0)
			continue;
		vector<int>* candidateInterp = operationExpr->GetInterpretation();
		vector<int> intersect;
		set_intersection(cansInterp->begin(), cansInterp->end(), candidateInterp->begin(), candidateInterp->end(),
				back_inserter(intersect));
		if (intersect.size() == cansInterp->size())
			return false;
	}
	return true;
}

void insert_compound_concept(Expression* operationExpr, vector<Expression*>* candidateConcepts,
		Expression* childConcept = NULL) {
	if (concept_allowed(operationExpr, candidateConcepts, childConcept)) {
		candidateConcepts->push_back(operationExpr);
	} else {
		delete operationExpr;
	}
}

void update_subsets() {
	objectSubsets.clear();
	bool found;
	for (unsigned i = 0; i < rootConcepts.size(); i++) {
		found = false;
		vector<int>* interp = rootConcepts[i]->GetInterpretation();
		for (unsigned j = 0; j < objectSubsets.size(); j++) {
			if (interp->size() != objectSubsets[j].size())
				continue;
			vector<int> intersect;
			set_intersection(interp->begin(), interp->end(), objectSubsets[j].begin(), objectSubsets[j].end(),
					back_inserter(intersect));
			if (intersect.size() == interp->size()) {
				found = true;
				break;
			}
		}
		if (!found)
			objectSubsets.push_back(*(rootConcepts[i]->GetInterpretation()));
	}
}

void update_subsets(vector<Expression*> candidateConcepts) {
	bool found;
	for (unsigned i = 0; i < candidateConcepts.size(); i++) {
		found = false;
		vector<int>* interp = candidateConcepts[i]->GetInterpretation();
		for (unsigned j = 0; j < objectSubsets.size(); j++) {
			if (interp->size() != objectSubsets[j].size())
				continue;
			vector<int> intersect;
			set_intersection(interp->begin(), interp->end(), objectSubsets[j].begin(), objectSubsets[j].end(),
					back_inserter(intersect));
			if (intersect.size() == interp->size()) {
				found = true;
				break;
			}
		}
		if (!found)
			objectSubsets.push_back(*(candidateConcepts[i]->GetInterpretation()));
	}
}
void print_expressions(string title, vector<Expression*>* expressions){
	vector<Expression*>::iterator it;
		cout << endl << "-"<<title<<"-" << endl;
		if(expressions->begin() ==  expressions->end()){
			cout<<"\tNo expressions"<<endl;
			return;
		}
		for (it = expressions->begin(); it != expressions->end(); ++it) {
			cout << "\t";
			(*it)->infix(cout);
			cout << " Interp: ";
			for (unsigned i = 0; i < (*it)->GetInterpretation()->size(); i++) {
				cout << instanceObjects[(*(*it)->GetInterpretation())[i]]->signature() << " ";
			}
			cout << endl;
		}
}

void combine_concepts() {
	vector<Expression*>::iterator conceptIt, conceptIt1, roleIt, roleIt1;
	vector<Expression*> candidates;
	UnaryOperator* uo;
	BinaryOperator* bo;
	update_subsets();
	double max_subset_num = std::pow(2., instanceObjects.size());
	cout << "MAX_SUBSET_NUM:" << max_subset_num << endl;
	while (objectSubsets.size()-1 < max_subset_num) {
		for (conceptIt = rootConcepts.begin(); conceptIt < rootConcepts.end(); ++conceptIt) {
			uo = new Not(*conceptIt, &instanceObjects);
			cout<<" ";(uo)->infix(cout);
			insert_compound_concept(uo, &candidates, *conceptIt);
			for (roleIt = rootRoles.begin(); roleIt < rootRoles.end(); ++roleIt) {
				bo = new ValueRestriction(*roleIt, *conceptIt);
				insert_compound_concept(bo, &candidates);
				for (roleIt1 = rootRoles.begin(); roleIt1 < rootRoles.end(); ++roleIt1) {
					bo = new Equality(*roleIt, *roleIt1);
					insert_compound_concept(bo, &candidates);
				}
			}
			for (conceptIt1 = rootConcepts.begin(); conceptIt1 < rootConcepts.end(); ++conceptIt1) {
				bo = new Join(*conceptIt, *conceptIt1);
				insert_compound_concept(bo, &candidates, *conceptIt1);
			}
		}
		unsigned previousSubsetsSize = objectSubsets.size();
		update_subsets(candidates);
		print_expressions("Candidates",&candidates);
		cout << "SUBSETS:" << objectSubsets.size() -1 << endl;
		for(unsigned i = 0;i<objectSubsets.size();i++){
			cout<<"\t";
			for(unsigned j=0;j<objectSubsets[i].size();j++)
				cout<<" "<< instanceObjects[objectSubsets[i][j]]->signature();
		}
		cout<<endl;
		if (previousSubsetsSize == objectSubsets.size())
			break;
		for (conceptIt = candidates.begin(); conceptIt < candidates.end(); ++conceptIt) {
			rootConcepts.push_back(*conceptIt);
		}
	}
}

void combine_roles() {
	vector<Expression*>::iterator roleIt, roleIt1;
	vector<Expression*> candidates;
	UnaryOperator* uo;
	for (roleIt = rootRoles.begin(); roleIt < rootRoles.end(); ++roleIt) {
		uo = new InverseRole(*roleIt);
		candidates.push_back(uo);
		uo = new TransitiveClosure(*roleIt);
		candidates.push_back(uo);
	}

	for (roleIt = candidates.begin(); roleIt < candidates.end(); ++roleIt) {
		rootRoles.push_back(*roleIt);
	}
}

void get_type_concepts_interpretation(STRIPS_Problem& prob) {
	for (unsigned i = 0; i < prob.num_types(); i++) {
		string type_signature = prob.types()[i]->signature();
		if (type_signature.compare("NO-TYPE") == 0 || type_signature.compare("ARTFICIAL-ALL-OBJECTS") == 0)
			continue;
		ConceptNode* c = typeConcepts[type_signature];
		vector<int>* interpretation = c->GetInterpretation();
		aig_tk::PDDL_Object_Ptr_Vec obj_vec = prob.objects_by_type(i);

		for (unsigned k = 0; k < obj_vec.size(); k++)
			interpretation->push_back(obj_vec[k]->index());
	}
}

void get_primitive_concepts_relations(STRIPS_Problem& prob) {
	//Get types as primitive concepts
	for (unsigned i = 0; i < prob.num_types(); i++) {
		string type_signature = prob.types()[i]->signature();
		if (type_signature.compare("NO-TYPE") == 0 || type_signature.compare("ARTFICIAL-ALL-OBJECTS") == 0)
			continue;
		ConceptNode* c = new ConceptNode(type_signature);
		typeConcepts[type_signature] = c;
	}

	//Get fluents as primitive concepts
	aig_tk::Index_Vec types_idxs;
	unsigned fnum = prob.num_fluents();
	for (unsigned k = 0; k < fnum; k++) {
		types_idxs = prob.fluents()[k]->pddl_types_idx();

		if (types_idxs.size() == 1 && prob.types()[types_idxs[0]]->name() == "NO-TYPE") {
			continue;
		}
		if (types_idxs.size() > 1) {
			if (types_idxs.size() == 2) {
				map<string, RoleNode*>::iterator itPR;
				bool inside = false;
				for (itPR = primitiveRoles.begin(); itPR != primitiveRoles.end(); ++itPR) {
					if ((*itPR->second).GetPredicate() == prob.fluents()[k]->predicate()) {
						inside = true;
						break;
					}
				}
				if (inside)
					continue;
				RoleNode* rnode = new RoleNode(prob.fluents()[k]->predicate());
				primitiveRoles[prob.fluents()[k]->predicate()] = rnode;

				rnode = new RoleNode(prob.fluents()[k]->predicate());
				rnode->IsGoal(true);
				goalRoles[prob.fluents()[k]->predicate()] = rnode;
			}
			continue;
		}

		//Get relations as primitive roles
		map<string, ConceptNode*>::iterator it;
		bool inside = false;

		for (it = primitiveConcepts.begin(); it != primitiveConcepts.end(); ++it) {
			if (it->second->GetPredicate() == prob.fluents()[k]->predicate()) {
				inside = true;
				break;
			}
		}

		if (inside || (prob.types()[types_idxs[0]]->name().compare("NO-TYPE")) == 0) {
			continue;
		}

		ConceptNode* c = new ConceptNode(prob.fluents()[k]->predicate());
		primitiveConcepts[prob.fluents()[k]->predicate()] = c;
		c = new ConceptNode(prob.fluents()[k]->predicate());
		c->IsGoal(true);
		goalConcepts[prob.fluents()[k]->predicate()] = c;

	}

	/*Just print out the primitive concepts and roles*/
	map<string, ConceptNode*>::iterator pos;
	cout << "Primitive concepts: " << endl;
	for (pos = primitiveConcepts.begin(); pos != primitiveConcepts.end(); ++pos) {
		cout << "Concept: ";
		pos->second->print(cout);
		cout << endl;
	}
	cout << "Type concepts: " << endl;
	for (pos = typeConcepts.begin(); pos != typeConcepts.end(); ++pos) {
		cout << "Concept: ";
		pos->second->print(cout);
		cout << endl;
	}

	map<string, RoleNode*>::iterator riter;
	cout << "Primitive roles: " << endl;
	for (riter = primitiveRoles.begin(); riter != primitiveRoles.end(); ++riter) {
		cout << "Concept: ";
		riter->second->print(cout);
		cout << endl;
	}
}

bool ruleSortPredicate(Rule r1, Rule r2) {
	if (r1.GetCorrect() > r2.GetCorrect())
		return true;
	if (r1.GetCoverage() < r2.GetCoverage())
		return false;
	return (r1.GetCurrentCoverage() > r2.GetCurrentCoverage());
}

void print_ruleset() {
	sort(ruleSet.begin(), ruleSet.end(), ruleSortPredicate);
	vector<Rule>::iterator ruleIterator;
	cout << "**************Rules******************" << endl;
	for (ruleIterator = ruleSet.begin(); ruleIterator != ruleSet.end(); ++ruleIterator) {
		cout << "\t" << *ruleIterator << "; Examples: " << ruleIterator->GetExamples() << "; Coverage:"
				<< ruleIterator->GetCoverage() << "; Hits:" << ruleIterator->GetCorrect() << std::endl;
	}
	cout << "*************************************";
}

void print_interpretations(STRIPS_Problem& prob) {
//	map<string, ConceptNode*>::iterator pos;
//	for (pos = primitiveConcepts.begin(); pos != primitiveConcepts.end(); ++pos) {
//		cout << endl << "Concept: " << pos->first << " Individuals: " << endl;
//		for (unsigned i = 0; i < pos->second->GetInterpretation()->size(); i++) {
//			cout << instanceObjects[(*pos->second->GetInterpretation())[i]]->signature() << " ";
//		}
//		cout << endl;
//	}
//
//	for (pos = typeConcepts.begin(); pos != typeConcepts.end(); ++pos) {
//		cout << endl << "Type concept: " << pos->first << " Individuals: " << endl;
//		for (unsigned i = 0; i < pos->second->GetInterpretation()->size(); i++) {
//			cout << instanceObjects[(*pos->second->GetInterpretation())[i]]->signature() << " ";
//		}
//		cout << endl;
//	}

////Primitive role interpretations
//	map<string, RoleNode*>::iterator itrl;
//	vector<pair<int, int> >::iterator itintrp;
//	for (itrl = primitiveRoles.begin(); itrl != primitiveRoles.end(); ++itrl) {
//		cout << endl << "Role: " << itrl->first << " Pairs: " << endl;
//		vector<pair<int, int> >* rinterpretation = (*itrl->second).GetRoleInterpretation();
//
//		for (itintrp = rinterpretation->begin(); itintrp != rinterpretation->end(); ++itintrp) {
//			cout << "(" << instanceObjects[(int) (itintrp->first)]->signature() << ","
//					<< instanceObjects[(int) (itintrp->second)]->signature() << ")" << " ";
//		}
//		cout << endl;
//	}

	print_expressions("Concepts",&rootConcepts);
	vector<Expression*>::iterator it;
	cout <<"-Roles-" << endl;
	for (it = rootRoles.begin(); it != rootRoles.end(); ++it) {
		cout << "\t";
		(*it)->infix(cout);
		cout << " Interp: ";
		for (unsigned i = 0; i < (*it)->GetRoleInterpretation()->size(); i++) {
			cout << "(" << instanceObjects[(*(*it)->GetRoleInterpretation())[i].first]->signature() << ","
					<< instanceObjects[(*(*it)->GetRoleInterpretation())[i].second]->signature() << ")" << " ";
		}
		cout << endl;
	}
	print_ruleset();
	cout << endl;
}

void print_goal_interpretations(STRIPS_Problem& prob) {
	map<string, ConceptNode*>::iterator pos;
	cout << "-Goal concepts-" << endl;
	for (pos = goalConcepts.begin(); pos != goalConcepts.end(); ++pos) {
		cout << "\t" << pos->first << " Interp: ";
		for (unsigned i = 0; i < pos->second->GetInterpretation()->size(); i++) {
			cout << instanceObjects[(*pos->second->GetInterpretation())[i]]->signature() << " ";
		}
		cout << endl;
	}

//Goal Role interpretations
	cout << "-Goal roles-" << endl;
	map<string, RoleNode*>::iterator itrl;
	vector<pair<int, int> >::iterator itintrp;
	for (itrl = goalRoles.begin(); itrl != goalRoles.end(); ++itrl) {
		cout << "\t" << itrl->first << " Interp: ";
		vector<pair<int, int> >* rinterpretation = (*itrl->second).GetRoleInterpretation();

		for (itintrp = rinterpretation->begin(); itintrp != rinterpretation->end(); ++itintrp) {
			cout << "(" << instanceObjects[(int) (itintrp->first)]->signature() << ",";
			cout << instanceObjects[(int) (itintrp->second)]->signature() << ")" << " ";
		}
		cout << endl;
	}
}

void clear_interpretations() {
	map<string, ConceptNode*>::iterator itPC;
	for (itPC = primitiveConcepts.begin(); itPC != primitiveConcepts.end(); ++itPC) {
		itPC->second->ClearInterpretation();
	}

	map<string, RoleNode*>::iterator itPR;
	for (itPR = primitiveRoles.begin(); itPR != primitiveRoles.end(); ++itPR) {
		itPR->second->ClearInterpretation();
	}
}

void clear_static_interpretations() {
	map<string, ConceptNode*>::iterator itPC;
	for (itPC = goalConcepts.begin(); itPC != goalConcepts.end(); ++itPC) {
		itPC->second->ClearInterpretation();
	}

	for (itPC = typeConcepts.begin(); itPC != typeConcepts.end(); ++itPC) {
		itPC->second->ClearInterpretation();
	}

	map<string, RoleNode*>::iterator itPR;
	for (itPR = goalRoles.begin(); itPR != goalRoles.end(); ++itPR) {
		itPR->second->ClearInterpretation();
	}
}

void update_compound_interpretations() {
	vector<Expression*>::iterator pos;
	for (pos = rootConcepts.begin(); pos != rootConcepts.end(); ++pos) {
		(*pos)->UpdateInterpretation();
	}
	for (pos = rootRoles.begin(); pos != rootRoles.end(); ++pos) {
		(*pos)->UpdateInterpretation();
	}
}

void get_goal_interpretations(STRIPS_Problem& prob) {
	cout << "-Goal state-" << endl;
	for (unsigned k = 0; k < prob.goal().size(); k++) {
		unsigned p = prob.goal()[k];
		aig_tk::Index_Vec types_idxs = prob.fluents()[p]->pddl_types_idx();
		int arity = types_idxs.size();
		if ((prob.types()[types_idxs[0]]->name().compare("NO-TYPE")) == 0)
			continue;
		cout << prob.fluents()[p]->signature() << endl;

		if (arity > 2)
			continue;
		string current_predicate = prob.fluents()[p]->predicate();
		Index_Vec& objs_idx = prob.fluents()[p]->pddl_objs_idx();

		pair<int, int>* po;
		if (arity == 2 && objs_idx.size() == 2) {
			vector<pair<int, int> >* interpPRVec = goalRoles[current_predicate]->GetRoleInterpretation();
			po = new pair<int, int>(objs_idx[0], objs_idx[1]);

			if (std::find((*interpPRVec).begin(), (*interpPRVec).end(), *po) == (*interpPRVec).end()) {
				interpPRVec->push_back(*po);
			} else {
				delete po;
			}
			continue;
		}

		//TODO find function
		for (unsigned j = 0; j < objs_idx.size(); j++) {
			bool found = false;
			int primitiveConceptsSize = goalConcepts[current_predicate]->GetInterpretation()->size();
			for (int k = 0; k < primitiveConceptsSize; k++) {
				if ((*goalConcepts[current_predicate]->GetInterpretation())[k] == objs_idx[j]) {
					found = true;
					break;
				}
			}
			if (!found) {
				goalConcepts[current_predicate]->GetInterpretation()->push_back(objs_idx[j]);
				found = false;
			}
		}
	}
}

void update_primitive_interpretations(STRIPS_Problem& prob, Node* n) {
	aig_tk::State* s = n->s();

	int tmp = 0;
	string current_predicate;
	clear_interpretations();
	for (unsigned i = 0; i < s->fluent_vec().size(); i++) {
		tmp = s->fluent_vec()[i];

		aig_tk::Index_Vec types_idxs = prob.fluents()[tmp]->pddl_types_idx();
		int arity = types_idxs.size();

		if ((prob.types()[types_idxs[0]]->name().compare("NO-TYPE")) == 0) {
			continue;
		}

		current_predicate = prob.fluents()[tmp]->predicate();
		cout << current_predicate;
		cout << " : ";

		if (arity > 2)
			continue;

		Index_Vec& objs_idx = prob.fluents()[tmp]->pddl_objs_idx();

		pair<int, int>* p;
		if (arity == 2 && objs_idx.size() == 2) {
			cout << "(" << instanceObjects[objs_idx[0]]->signature() << "," << instanceObjects[objs_idx[1]]->signature()
					<< ") | ";

			vector<pair<int, int> >* interpPRVec = primitiveRoles[current_predicate]->GetRoleInterpretation();
			p = new pair<int, int>(objs_idx[0], objs_idx[1]);

			if (std::find((*interpPRVec).begin(), (*interpPRVec).end(), *p) == (*interpPRVec).end()) {
				interpPRVec->push_back(*p);
			} else {
				delete p;
			}
			continue;
		}

		for (unsigned j = 0; j < objs_idx.size(); j++) {
			cout << instanceObjects[objs_idx[j]]->signature() << " ";
			vector<int>* interpPCVec = primitiveConcepts[current_predicate]->GetInterpretation();

			if (std::find((*interpPCVec).begin(), (*interpPCVec).end(), objs_idx[j]) == (*interpPCVec).end()) {
				interpPCVec->push_back(objs_idx[j]);
			}
		}
		sort(primitiveConcepts[current_predicate]->GetInterpretation()->begin(),
				primitiveConcepts[current_predicate]->GetInterpretation()->end());
		cout << " | ";
	}
	update_compound_interpretations();

}

void interpret_plan(STRIPS_Problem& prob, vector<aig_tk::Node*>& plan) {

	for (unsigned k = 0; k < plan.size() - 1; k++) {
		std::cout << endl << "State(" << k + 1 << "): ";
		update_primitive_interpretations(prob, plan[k]);
		if (k == 0) {
			initialize_root_concepts();
		}
		print_interpretations(prob);
		cout << "-Combining concepts-" << endl;
		combine_concepts();

		//Adding rules
		aig_tk::Action* a = plan[k + 1]->op();
		aig_tk::Index_Vec objs_idx = a->pddl_objs_idx();
		sort(objs_idx.begin(), objs_idx.end());

		//TODO loop here
		Rule* r = new Rule(a);
		unsigned j = 0;
		for (unsigned g = 0; g < rootConcepts.size() && j < 1; g++) {
			vector<int>* interp = rootConcepts[g]->GetInterpretation();
			if (interp->size() == 0)
				continue;
			sort(interp->begin(), interp->end());
			if ((int) (*interp)[0] == (int) objs_idx[j]) {
				j++;
				if (!r->AddConcept(rootConcepts[g]))
					cout << "Error";
			}
		}

		if (r->GetConcepts().size() == objs_idx.size()) {
			bool found = false;
			for (unsigned i = 0; i < ruleSet.size(); i++)
				if (ruleSet[i] == *r) {
					found = true;
					break;
				}
			if (!found)
				ruleSet.push_back(*r);
		} else
			delete r;

		//Rule coverage
		for (unsigned i = 0; i < ruleSet.size(); i++) {
			vector<Expression*> ruleConcepts = ruleSet[i].GetConcepts();
			bool conceptCover = false;
			bool ruleCorrect = false;
			for (unsigned j = 0; j < ruleConcepts.size(); j++) {
				vector<int>* interp = ruleConcepts[j]->GetInterpretation();
				conceptCover = false;
				ruleCorrect = false;
				if (interp->size() > 0) {
					conceptCover = true;
					if ((int) (*interp)[0] == (int) objs_idx[j])
						ruleCorrect = true;
				}
			}
			ruleSet[i].IncExamples();
			if (conceptCover)
				ruleSet[i].IncCoverage();
			if (ruleCorrect)
				ruleSet[i].IncCorrect();

		}
	}
}

void get_plan(STRIPS_Problem& strips_prob, vector<aig_tk::Node*>& plan) {
	aig_tk::Max_Heuristic estimator;
	estimator.initialize(strips_prob);
	estimator.compute(strips_prob.init());
	aig_tk::Cost_Type goal_cost = estimator.eval(strips_prob.goal());

	aig_tk::Node* n0 = aig_tk::Node::root(strips_prob);
	/**
	 * SEARCH
	 **/

	Best_First_Search engine;

	engine.set_heuristic(estimator);
	engine.set_problem(strips_prob);

//float t0, tf;

//t0 = time_used();

//	if (engine.solve(n0, plan)) {
//		tf = time_used();
//		//output_plan(plan, t0, tf);
//	}

	plan.clear();
	n0 = Node::root(strips_prob);

	if (engine.solve(n0, plan)) {
		//tf = time_used();
		//output_plan(plan, t0, tf);
	} else
		std::cerr << "FALLO" << std::endl;

	clear_static_interpretations();
	get_type_concepts_interpretation(strips_prob);
	get_goal_interpretations(strips_prob);
	cout << "Goal interpretations" << endl;
	print_goal_interpretations(strips_prob);
	cout << "Update primitive interpretations" << endl;
	interpret_plan(strips_prob, plan);
}

void reset_globals() {
	/* used to time the different stages of the planner
	 */
	gtempl_time = 0, greach_time = 0, grelev_time = 0, gconn_time = 0;
	gLNF_time = 0, gsearch_time = 0;

	/* number of states that got heuristically evaluated
	 */
	gevaluated_states = 0;

	/* maximal depth of breadth first search
	 */
	gmax_search_depth = 0;
	gbracket_count = 0;
	gproblem_name = NULL;

	/* The current input line number
	 */
	lineno = 1;

	/* The current input filename
	 */
	gact_filename = NULL;

	/* The pddl domain name
	 */
	gdomain_name = NULL;

	/* loaded, uninstantiated operators
	 */
	gloaded_ops = NULL;

	/* stores initials as fact_list
	 */
	gorig_initial_facts = NULL;

	/* not yet preprocessed goal facts
	 */
	gorig_goal_facts = NULL;

	/* axioms as in UCPOP before being changed to ops
	 */
	//gloaded_axioms = NULL;
	/* the types, as defined in the domain file
	 */
	gparse_types = NULL;

	/* the constants, as defined in domain file
	 */
	gparse_constants = NULL;

	/* the predicates and their arg types, as defined in the domain file
	 */
	gparse_predicates = NULL;

	/* the functions and their arg types, as defined in the domain file
	 */
	gparse_functions = NULL;

	/* the objects, declared in the problem file
	 */
	gparse_objects = NULL;

	/* the metric
	 */
	gparse_optimization = NULL;
	gparse_metric = NULL;

	/* connection to instantiation ( except ops, goal, initial )
	 */

	/* all typed objects
	 */
	gorig_constant_list = NULL;

	/* the predicates and their types
	 */
	gpredicates_and_types = NULL;

	/* the functions and their types
	 */
	gfunctions_and_types = NULL;
	gnum_constants = 0;
	//gtype_names[MAX_TYPES];
	//gtype_consts[MAX_TYPES][MAX_TYPE];
	//gis_member[MAX_CONSTANTS][MAX_TYPES];
	//gtype_size[MAX_TYPES];
	gnum_types = 0;
	//gpredicates[MAX_PREDICATES];
	//garity[MAX_PREDICATES];
	//gpredicates_args_type[MAX_PREDICATES][MAX_ARITY];
	gnum_predicates = 0;
	//gfunctions[MAX_FUNCTIONS];
	//gf_arity[MAX_FUNCTIONS];
	//gfunctions_args_type[MAX_FUNCTIONS][MAX_ARITY];
	gnum_functions = 0;

	/* the domain in integer (Fact) representation
	 */
	//goperators[MAX_OPERATORS];
	gnum_operators = 0;
	gfull_initial = NULL;
	gnum_full_initial = 0;
	gfull_fluents_initial = NULL;
	gnum_full_fluents_initial = 0;
	ggoal = NULL;
	gmetric = NULL;

	ginitial = NULL;
	gnum_initial = 0;
	ginitial_predicate = NULL;
	gnum_initial_predicate = NULL;

	/* same thing for functions
	 */
	gf_initial = NULL;
	gnum_f_initial = 0;
	ginitial_function = NULL;
	gnum_initial_function = NULL;

	/* splitted domain: hard n easy ops
	 */
	ghard_operators = NULL;
	gnum_hard_operators = 0;
	geasy_operators = NULL;
	gnum_easy_operators = 0;

	/* so called Templates for easy ops: possible inertia constrained
	 * instantiation constants
	 */
	geasy_templates = NULL;
	gnum_easy_templates = 0;

	/* first step for hard ops: create mixed operators, with conjunctive
	 * precondition and arbitrary effects
	 */
	ghard_mixed_operators = NULL;
	gnum_hard_mixed_operators = 0;

	/* hard ''templates'' : pseudo actions
	 */
	ghard_templates = NULL;
	gnum_hard_templates = 0;

	/* store the final "relevant facts"
	 */
	gnum_relevant_facts = 0;
	gnum_pp_facts = 0;
	/* store the "relevant fluents"
	 */
	gnum_relevant_fluents = 0;
	//delete[] grelevant_fluents_name;
	/* this is NULL for normal, and the LNF for
	 * artificial fluents.
	 */
	//delete[] grelevant_fluents_lnf;
	/* the final actions and problem representation
	 */
	gactions = NULL;
	gnum_actions = 0;
	//ginitial_state;
	glogic_goal = NULL;
	gnum_logic_goal = 0;
	gnumeric_goal_comp = NULL;
	gnumeric_goal_lh = NULL;
	gnumeric_goal_rh = NULL;
	gnum_numeric_goal = 0;

	/* direct numeric goal access
	 */
	//gnumeric_goal_direct_comp = NULL;
	//gnumeric_goal_direct_c = NULL;
	/* to avoid memory leaks; too complicated to identify
	 * the exact state of the action to throw away (during construction),
	 * memory gain not worth the implementation effort.
	 */
	gtrash_actions = NULL;

	/* additional lnf step between finalized inst and
	 * conn graph
	 */
	glnf_goal_comp = NULL;
	glnf_goal_lh = NULL;
	glnf_goal_rh = NULL;
	gnum_lnf_goal = 0;

	//LnfExpNode glnf_metric;
	goptimization_established = FALSE;

	/**********************
	 * CONNECTIVITY GRAPH *
	 **********************/
	/* one ops (actions) array ...
	 */
	gop_conn = NULL;
	gnum_op_conn = 0;

	/* one effects array ...
	 */
	gef_conn = NULL;
	gnum_ef_conn = 0;

	/* one facts array.
	 */
	gft_conn = NULL;
	gnum_ft_conn = 0;

	/* and: one fluents array.
	 */
	gfl_conn = NULL;
	gnum_fl_conn = 0;
	gnum_real_fl_conn = 0;/* number of non-artificial ones */

	/* final goal is also transformed one more step.
	 */
	gflogic_goal = NULL;
	gnum_flogic_goal = 0;
	gfnumeric_goal_comp = NULL;
	gfnumeric_goal_fl = NULL;
	gfnumeric_goal_c = NULL;
	gnum_fnumeric_goal = 0;

	/* direct access (by relevant fluents)
	 */
	gfnumeric_goal_direct_comp = NULL;
	gfnumeric_goal_direct_c = NULL;

	/*******************
	 * SEARCHING NEEDS *
	 *******************/
	/* applicable actions
	 */
	gA = NULL;
	gnum_A = 0;

	/* communication from extract 1.P. to search engine:
	 * 1P action choice
	 */
	gH = NULL;
	gnum_H = 0;
	/* cost of relaxed plan
	 */
	gcost = 0.;
	gnum_plan_ops = 0;

}

void set_instance_objects(STRIPS_Problem& prob) {
	instanceObjects.clear();
	instanceObjects = prob.objects();
	PDDL_Object_Ptr_Vec::iterator itr;
	for (itr = instanceObjects.begin(); itr != instanceObjects.end(); ++itr) {
		if ((*itr)->signature().compare("NO-OBJECT") == 0) {
			instanceObjects.erase(itr);
			break;
		}
	}
}

void solve(STRIPS_Problem& prob) {
	aig_tk::Node* n = aig_tk::Node::root(prob);
	int max = 50;
	set_instance_objects(prob);
	clear_static_interpretations();
	get_type_concepts_interpretation(prob);
	get_goal_interpretations(prob);
	while (!(n->s()->entails(prob.goal())) && max > 0) {
		update_primitive_interpretations(prob, n);
		std::sort(ruleSet.begin(), ruleSet.end(), ruleSortPredicate);
		vector<Rule>::iterator it = ruleSet.begin();
		bool applied = false;
		while (!applied && it != ruleSet.end()) {
			Rule r = *(it);

			for (unsigned i = 0; i < prob.num_actions(); i++) {
				aig_tk::Action* a = prob.actions()[i];
				if (a->name() == r.GetAction()->name())
					if (a->can_be_applied_on(*(n->s())) && n->successor(a) != n->parent()) {
						n = n->successor(a);
						cout << endl << "Rule:" << r << " Action:" << a->signature() << endl;
						applied = true;
						break;
					}
			}
			++it;
		}
		--max;
	}
	if (max != 0)
		cout << endl << "Reached goal" << endl;
}

int main(int argc, char** argv) {
	if (argc < 4) {
		std::cerr << "No prob description provided, bailing out!" << std::endl;
		std::exit(1);
	}

	std::string folder(argv[1]);
	std::string domain(argv[2]);
	int instance_num(atoi(argv[3]));
	std::string txt_output_filename("prob.txt.strips");
	string instance("instance");

	string instance_path;
	folder = "./problems/" + folder + "/";
	domain = folder + domain;
	vector<vector<aig_tk::Action*>*> plans;
	std::vector<aig_tk::Node*>* plan;
	aig_tk::STRIPS_Problem* strips_prob;
	aig_tk::FF_PDDL_To_STRIPS adl_compiler;
	for (int i = 0; i < instance_num; i++) {
		instance_path = folder + instance + static_cast<ostringstream*>(&(ostringstream() << (i + 1)))->str() + ".pddl";

		reset_globals();
		strips_prob = new STRIPS_Problem();
		adl_compiler = FF_PDDL_To_STRIPS();
		adl_compiler.get_problem_description(domain, instance_path, *strips_prob, true);
		cout << "Compiled" << endl;

		if (i == 0) {
			get_primitive_concepts_relations(*strips_prob);
			combine_roles();
		}

		set_instance_objects(*strips_prob);

		cout << "Extracted primitive concepts" << endl;
		plan = new vector<aig_tk::Node*>();
		get_plan(*strips_prob, *plan);
		vector<aig_tk::Action*>* p = new vector<aig_tk::Action*>();
		for (unsigned j = 0; j < plan->size(); j++) {
			p->push_back((*plan)[j]->op());
		}
		plans.push_back(p);

		std::cout << std::endl;
		std::cout << "Actions: " << strips_prob->num_actions();
		std::cout << " Fluents: " << strips_prob->num_fluents() << std::endl;
		delete strips_prob;
	}

	cout << "Plans" << endl;
	vector<vector<aig_tk::Action*>*>::iterator planItr;
	vector<aig_tk::Action*> actionItr;
	for (planItr = plans.begin(); planItr != plans.end(); ++planItr) {
		cout << "Plan[-]:" << endl;

		for (unsigned i = 0; i < (**planItr).size(); i++)
			cout << " - " << (**planItr)[i]->signature() << endl;
	}

	cout << endl << "Training finished using ruleset to solve a problem" << endl;
	reset_globals();
	strips_prob = new STRIPS_Problem();
	adl_compiler = FF_PDDL_To_STRIPS();
	adl_compiler.get_problem_description(domain, instance_path, *strips_prob, true);
	cout << "Compiled" << endl;
	solve(*strips_prob);

	return 0;
}
