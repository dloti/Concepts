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
#include <cstdlib>
#include <fstream>
#include "CompoundConcept.hxx"
#include "ConceptNode.hxx"
#include "BinaryOperator.hxx"
#include "Join.hxx"

using namespace aig_tk;
using namespace std;

map<string, vector<int> > relations;
map<string, ConceptNode> primitiveConcepts;
map<string, ConceptNode> goalConcepts;
vector<CompoundConcept*> rootConcepts;
void encode_as_text(aig_tk::STRIPS_Problem& prob, std::string destination) {
	std::ofstream out_stream(destination.c_str());

	/**
	 * Printing The Fluents of the problem.
	 * Recall that all the information is accesible from STRIPS_Problem
	 */
	map<string, int> concepts;
	out_stream << "Primitive concepts:" << std::endl;
	for (unsigned k = 0; k < prob.num_fluents(); k++) {
		aig_tk::Index_Vec types_idxs = prob.fluents()[k]->pddl_types_idx();
		if (types_idxs.size() > 1)
			continue;

		if (concepts.find(prob.fluents()[k]->predicate()) != concepts.end())
			continue;
		concepts[prob.fluents()[k]->predicate()] = 1;
		out_stream << "\t\tPredicate:" << std::endl;
		out_stream << "\t\t\t" << prob.fluents()[k]->predicate() << std::endl;

		out_stream << "\t\tType:" << std::endl;
		aig_tk::Index_Vec obj_idxs = prob.fluents()[k]->pddl_objs_idx();
		for (unsigned i = 0; i < obj_idxs.size(); i++)
			out_stream << "\t\t\t" << prob.types()[types_idxs[i]]->signature()
					<< std::endl;
	}
	concepts.clear();

	out_stream << "Types:" << std::endl;
	for (unsigned i = 0; i < prob.num_types(); i++) {
		out_stream << "\t" << prob.types()[i]->signature() << std::endl;

		aig_tk::Fluent_Ptr_Vec fl_vec = prob.fluents_by_type(i);
		if (fl_vec.size() != 0) {
			out_stream << "\t\tFluents with this type" << std::endl;
			for (unsigned k = 0; k < fl_vec.size(); k++)
				out_stream << "\t\t\t" << fl_vec[k]->signature() << std::endl;
		}
		out_stream << "\t\tObjects with this type" << std::endl;
		aig_tk::PDDL_Object_Ptr_Vec obj_vec = prob.objects_by_type(i);
		for (unsigned k = 0; k < obj_vec.size(); k++)
			out_stream << "\t\t\t" << obj_vec[k]->signature() << std::endl;

	}

	out_stream << "Binary predicates:" << std::endl;
	for (unsigned k = 0; k < prob.num_fluents(); k++) {
		aig_tk::Index_Vec types_idxs = prob.fluents()[k]->pddl_types_idx();
		if (types_idxs.size() != 2)
			continue;

		if (concepts.find(prob.fluents()[k]->predicate()) != concepts.end())
			continue;
		concepts[prob.fluents()[k]->predicate()] = 1;
		out_stream << "\t\tPredicate:" << std::endl;
		out_stream << "\t\t\t" << prob.fluents()[k]->predicate() << std::endl;

		out_stream << "\t\tTypes:" << std::endl;
		aig_tk::Index_Vec obj_idxs = prob.fluents()[k]->pddl_objs_idx();
		for (unsigned i = 0; i < obj_idxs.size(); i++)
			out_stream << "\t\t\t" << prob.types()[types_idxs[i]]->signature()
					<< std::endl;
	}
	concepts.clear();

	out_stream << "Objects:" << std::endl;
	for (unsigned i = 0; i < prob.num_objects(); i++) {
		out_stream << "\t" << prob.objects()[i]->signature() << std::endl;
		out_stream << "\t\ttypes:" << std::endl;

		aig_tk::Index_Vec& types_idx = prob.objects()[i]->types_idx();
		for (unsigned k = 0; k < types_idx.size(); k++)
			out_stream << "\t\t\t" << prob.types()[types_idx[k]]->signature()
					<< std::endl;

		out_stream << "\t\tFluents with this object" << std::endl;
		aig_tk::Fluent_Ptr_Vec fl_vec = prob.fluents_by_object(i);
		for (unsigned k = 0; k < fl_vec.size(); k++)
			out_stream << "\t\t\t" << fl_vec[k]->signature() << std::endl;
	}

	out_stream << "Fluents:" << std::endl << std::endl;
	for (unsigned k = 0; k < prob.num_fluents(); k++) {
		out_stream << "\t" << prob.fluents()[k]->signature() << std::endl;

		out_stream << "\t\tPredicate:" << std::endl;
		out_stream << "\t\t\t" << prob.fluents()[k]->predicate() << std::endl;

		out_stream << "\t\tObjects - Types:" << std::endl;
		aig_tk::Index_Vec obj_idxs = prob.fluents()[k]->pddl_objs_idx();
		aig_tk::Index_Vec types_idxs = prob.fluents()[k]->pddl_types_idx();
		for (unsigned i = 0; i < obj_idxs.size(); i++)
			out_stream << "\t\t\t" << prob.objects()[obj_idxs[i]]->signature()
					<< " - " << prob.types()[types_idxs[i]]->signature()
					<< std::endl;

	}

	/**
	 * Printing PDDL Abstract Operators of the problem.
	 * Recall that all the information is accesible from STRIPS_Problem
	 */
	out_stream << "PDDL Operators:" << std::endl << std::endl;
	for (unsigned k = 0; k < prob.num_pddl_ops(); k++) {
		aig_tk::PDDL_Operator* pddl_op = prob.pddl_ops()[k];
		out_stream << pddl_op->signature();
		out_stream << ":" << std::endl;

		out_stream << "\tBase Name:" << std::endl;
		out_stream << "\t\t" << pddl_op->name() << std::endl;

		out_stream << "\tObjects - Types:" << std::endl;
		aig_tk::Index_Vec types_idxs = pddl_op->pddl_types_idx();
		for (unsigned i = 0; i < types_idxs.size(); i++)
			out_stream << "\t\t" << pddl_op->args_name()[i] << " - "
					<< prob.types()[types_idxs[i]]->signature() << std::endl;

		out_stream << "\t grounded actions:" << std::endl;
		aig_tk::Action_Ptr_Vec a_vec = prob.actions_by_pddl_op(k);
		for (unsigned z = 0; z < a_vec.size(); z++)
			out_stream << "\t\t" << a_vec[z]->signature() << std::endl;
	}

	/**
	 * Printing the Actions of the problem.
	 */
	out_stream << "Actions:" << std::endl << std::endl;
	for (unsigned k = 0; k < prob.num_actions(); k++) {

		aig_tk::Action* action = prob.actions()[k];
		out_stream << action->signature();

		/**
		 * Printing the precs adds and dels of the aciton
		 */
		out_stream << ":" << std::endl;

		out_stream << "\tBase Name:" << std::endl;
		out_stream << "\t\t" << prob.actions()[k]->name() << std::endl;

		out_stream << "\tObjects - Types:" << std::endl;
		aig_tk::Index_Vec obj_idxs = prob.actions()[k]->pddl_objs_idx();
		aig_tk::Index_Vec types_idxs = prob.actions()[k]->pddl_types_idx();
		for (unsigned i = 0; i < obj_idxs.size(); i++)
			out_stream << "\t\t" << prob.objects()[obj_idxs[i]]->signature()
					<< " - " << prob.types()[types_idxs[i]]->signature()
					<< std::endl;

		out_stream << "\t" << "Preconditions:" << std::endl;

		aig_tk::Fluent_Vec& precs = action->prec_vec();
		for (unsigned j = 0; j < precs.size(); j++) {
			unsigned p = precs[j];
			out_stream << "\t\t";
			out_stream << prob.fluents()[p]->signature();
			out_stream << std::endl;
		}

		out_stream << "\t" << "Adds:" << std::endl;

		aig_tk::Fluent_Vec& adds = action->add_vec();
		for (unsigned j = 0; j < adds.size(); j++) {
			unsigned p = adds[j];
			out_stream << "\t\t";
			out_stream << prob.fluents()[p]->signature();
			out_stream << std::endl;
		}

		out_stream << "\t" << "Deletes:" << std::endl;

		aig_tk::Fluent_Vec& dels = action->del_vec();
		for (unsigned j = 0; j < dels.size(); j++) {
			unsigned p = dels[j];
			out_stream << "\t\t";
			out_stream << prob.fluents()[p]->signature();
			out_stream << std::endl;
		}

		out_stream << std::endl;

		if (!action->ceff_vec().empty()) {
			out_stream << "\t" << action->ceff_vec().size()
					<< " Conditional Effects:" << std::endl;
			aig_tk::Conditional_Effect_Vec& ceffs = action->ceff_vec();
			for (unsigned k = 0; k < ceffs.size(); k++) {
				out_stream << "\t\t" << "Conditions:" << std::endl;

				aig_tk::Fluent_Vec& precs = ceffs[k]->prec_vec();
				for (unsigned j = 0; j < precs.size(); j++) {
					unsigned p = precs[j];
					out_stream << "\t\t\t";
					out_stream << prob.fluents()[p]->signature();
					out_stream << std::endl;
				}

				out_stream << "\t\t" << "Adds:" << std::endl;

				aig_tk::Fluent_Vec& adds = ceffs[k]->add_vec();
				for (unsigned j = 0; j < adds.size(); j++) {
					unsigned p = adds[j];
					out_stream << "\t\t\t";
					out_stream << prob.fluents()[p]->signature();
					out_stream << std::endl;
				}

				out_stream << "\t\t" << "Deletes:" << std::endl;

				aig_tk::Fluent_Vec& dels = ceffs[k]->del_vec();
				for (unsigned j = 0; j < dels.size(); j++) {
					unsigned p = dels[j];
					out_stream << "\t\t\t";
					out_stream << prob.fluents()[p]->signature();
					out_stream << std::endl;
				}
				out_stream << std::endl;
			}
		}
		out_stream << std::endl;

	}
	/**
	 * Printing the Initial State of the problem.
	 */
	out_stream << "Initial State:" << std::endl << "\t";
	for (unsigned k = 0; k < prob.init().size(); k++) {
		unsigned p = prob.init()[k];
		out_stream << prob.fluents()[p]->signature();
		if (k < prob.init().size() - 1)
			out_stream << " ";
	}
	out_stream << std::endl << std::endl;

	/**
	 * Printing the Goal of the problem.
	 */
	out_stream << "Goal:" << std::endl << "\t";
	for (unsigned k = 0; k < prob.goal().size(); k++) {
		unsigned p = prob.goal()[k];
		out_stream << prob.fluents()[p]->signature();
		if (k < prob.goal().size() - 1)
			out_stream << " ";
	}
	out_stream << std::endl;

	out_stream.close();
}

void combine_concepts() {
//TODO mix them up, mind the grammar silly
	map<string, ConceptNode>::iterator pos, pos1;

	int itr = primitiveConcepts.size();
	int rnd = rand() % 3;

	for (pos = pos1 = primitiveConcepts.begin(); pos != primitiveConcepts.end();
			++pos) {
		BinaryOperator* c = new Join();
		(*c).SetLeft(&(pos->second));
		for (int i = rnd; i > 0; i--) {
			if (pos1 == primitiveConcepts.end()) {
				--pos1;
				break;
			}
		}
		(*c).SetRight(&((pos1)->second));
		rootConcepts.push_back(c);
	}
}

void get_primitive_concepts_relations(STRIPS_Problem& prob) {
	//vector<concept*> concepts;
	vector<int> vec;
	for (unsigned k = 0; k < prob.num_fluents(); k++) {
		aig_tk::Index_Vec types_idxs = prob.fluents()[k]->pddl_types_idx();
		if (types_idxs.size() > 1) {
			if (types_idxs.size() == 2)
				relations[prob.fluents()[k]->predicate()] = vec;
			continue;
		}

		map<string, ConceptNode>::iterator it;
		bool inside = false;
		for (it = primitiveConcepts.begin(); it != primitiveConcepts.end();
				++it) {
			if (it->second.GetPredicate() == prob.fluents()[k]->predicate()) {
				inside = true;
				break;
			}
		}
		if (inside || (prob.types()[types_idxs[0]]->name().compare("NO-TYPE"))==0)
			continue;
		cout<<endl<<prob.types()[types_idxs[0]]->name();
		ConceptNode* c = new ConceptNode(prob.fluents()[k]->predicate());
		if (types_idxs.size() == 1
				&& prob.types()[types_idxs[0]]->name() == "NO-TYPE") {
			//delete c;
			continue;
		}

		primitiveConcepts[prob.fluents()[k]->predicate()] = *c;
		c->IsGoal(true);
		goalConcepts[prob.fluents()[k]->predicate()] = *c;
		//if (concepts.find(prob.fluents()[k]->predicate()) != concepts.end())
		//	continue;

		//concepts[prob.fluents()[k]->predicate()] = vec;
		//concepts.push_back(new concept(prob.fluents()[k]->predicate()));
	}

	/*Just print out the primitive concepts*/
	map<string, ConceptNode>::iterator pos;
	for (pos = primitiveConcepts.begin(); pos != primitiveConcepts.end();
			++pos) {

		cout << "Concept: " << pos->second.GetPredicate() << endl;
	}
	//Combine concepts test
	combine_concepts();
//	for (pos = relations.begin(); pos != relations.end(); ++pos) {
//		cout << "Relation: " << pos->first << endl;
//	}
}

void print_interpretations(STRIPS_Problem& prob) {
	map<string, ConceptNode>::iterator pos;
	for (pos = primitiveConcepts.begin(); pos != primitiveConcepts.end();
			++pos) {
		cout << endl << "Concept: " << pos->first << " Objs: " << endl;
			for (int i = 0; i < pos->second.GetInterpretation()->size(); i++) {
				cout
						<< prob.objects()[(*pos->second.GetInterpretation())[i]]->signature()
						<< endl;
			}

	}

	//Complex concept interpretations
	vector<CompoundConcept* >::iterator it;
	cout << "++++++++++++++++++++++++++++++++++" << endl;
	for (it = rootConcepts.begin(); it != rootConcepts.end(); ++it) {
		cout << endl << "CConcept: ";
		(*it)->infix(cout);
		cout << " CObjs: " << endl;
		for (int i = 0; i < (*it)->GetInterpretation()->size(); i++) {
			cout << prob.objects()[(*(*it)->GetInterpretation())[i]]->signature()
					<< endl;
		}
	}
	cout << "++++++++++++++++++++++++++++++++++" << endl;
}

void clear_interpretations() {
	map<string, ConceptNode>::iterator pos;
	for (pos = primitiveConcepts.begin(); pos != primitiveConcepts.end();
			++pos) {
		pos->second.ClearInterpretation();
	}
}

void update_interpretations() {
	vector<CompoundConcept* >::iterator pos;
	for (pos = rootConcepts.begin(); pos != rootConcepts.end(); ++pos) {
		(*pos)->UpdateInterpretation();
	}
}

void get_interpretations(STRIPS_Problem& prob, vector<aig_tk::Node*>& plan) {
	for (unsigned k = 0; k < plan.size(); k++) {
		aig_tk::Action* a = plan[k]->op();
		aig_tk::State* s = plan[k]->s();
		std::cout << std::endl;
		std::cout << "---------------------------------" << std::endl;
		int tmp = 0;
		string current_predicate;
		clear_interpretations();
		for (unsigned i = 0; i < s->fluent_vec().size(); i++) {
			tmp = s->fluent_vec()[i];

			aig_tk::Index_Vec types_idxs = prob.fluents()[tmp]->pddl_types_idx();
			int arity = types_idxs.size();
			//get only unary predicates
			if (arity > 1)
				continue;

			if ((prob.types()[types_idxs[0]]->name().compare("NO-TYPE"))==0) {
							continue;
						}

			current_predicate = prob.fluents()[tmp]->predicate();
			cout << current_predicate;
			cout << " : ";


			Index_Vec& objs_idx = prob.fluents()[tmp]->pddl_objs_idx();
			for (int j = 0; j < objs_idx.size(); j++) {
				cout << prob.objects()[objs_idx[j]]->signature() << " ";
				bool found = false;
				int primitiveConceptsSize =
						primitiveConcepts[current_predicate].GetInterpretation()->size();
				for (int k = 0; k < primitiveConceptsSize; k++) {
					if ((*primitiveConcepts[current_predicate].GetInterpretation())[k]
							== objs_idx[j]) {
						found = true;
						break;
					}
				}
				if (!found) {

					primitiveConcepts[current_predicate].GetInterpretation()->push_back(
							objs_idx[j]);
					found = false;
				}
			}
			cout << " | ";
		}

		for (unsigned i = 0; i < s->fluent_vec().size(); i++){

		}
		update_interpretations();
		print_interpretations(prob);
	}
}

void get_plan(STRIPS_Problem& strips_prob, vector<aig_tk::Node*>& plan) {
	/**
	 * HEURISTIC EXAMPLE
	 */

	aig_tk::Max_Heuristic estimator;

	/**
	 * Initializing heuristic with the problem
	 */
	estimator.initialize(strips_prob);

	/**
	 * computing heuristic from the initial state
	 */
	estimator.compute(strips_prob.init());

	/**
	 * Evaluates h_max to a given set of fluents
	 */
	aig_tk::Cost_Type goal_cost = estimator.eval(strips_prob.goal());

	std::cout << "Goal Distance is " << goal_cost << std::endl;

	/**
	 * RELAXED PLAN EXAMPLE
	 */

	/**
	 * call generator of initial search node
	 */
	aig_tk::Node* n0 = aig_tk::Node::root(strips_prob);

	/**
	 * Create a propagator that uses h_max
	 */
	aig_tk::Propagator<aig_tk::Max_Heuristic> rel_plan(strips_prob);

	/**
	 * builds relax plan
	 */
	rel_plan.build_propagation_graph(n0);

	/**
	 * Prints Relax Plan
	 */
	rel_plan.print();

	/**
	 * EFFICIENT RELAXED PLAN EXTRACTOR
	 */

	aig_tk::Relaxed_Plan_Extractor<aig_tk::Max_Heuristic> h_ff;

	h_ff.initialize(strips_prob);

	unsigned cost_rel_plan = h_ff.eval(strips_prob.init(), strips_prob.goal());

	std::cout << std::endl << "Cost of Relaxed Plan: " << cost_rel_plan
			<< std::endl;

	/**
	 * SEARCH
	 **/

	Best_First_Search engine;

	engine.set_heuristic(estimator);
	engine.set_problem(strips_prob);

//std::vector<aig_tk::Node*> plan;
	float t0, tf;

	t0 = time_used();

	if (engine.solve(n0, plan)) {
		tf = time_used();
		//output_plan(plan, t0, tf);
	}

	plan.clear();
	n0 = Node::root(strips_prob);

	if (engine.solve(n0, plan)) {
		tf = time_used();
		//output_plan(plan, t0, tf);
	} else
		std::cerr << "FALLO" << std::endl;

	get_interpretations(strips_prob, plan);
}

void output_plan(std::vector<aig_tk::Node*> plan, float t0, float tf) {
	aig_tk::Cost_Type plan_cost = 0;

	std::cout << std::endl;
	std::cout << "================================================"
			<< std::endl;
	std::cout << ";; SOLUTION " << std::endl;
	std::cout << ";;\t Plan cost: " << std::fixed << plan_cost << ", steps: "
			<< plan.size() << std::endl;
	std::cout << ";;\t Time: ";
	report_interval(t0, tf, std::cout);
	std::cout << std::endl << "================================================"
			<< std::endl;

	for (unsigned k = 0; k < plan.size(); k++) {
		aig_tk::Action* a = plan[k]->op();
		std::cout << k + 1 << " - " << a->signature() << std::endl;
		plan_cost = aig_tk::add(plan_cost, a->cost());
	}
}

int main(int argc, char** argv) {
	if (argc < 3) {
		std::cerr << "No prob description provided, bailing out!" << std::endl;
		std::exit(1);
	}

	//TODO random seed
	srand(time(NULL));
	std::string domain(argv[1]);
	std::string instance(argv[2]);

	aig_tk::FF_PDDL_To_STRIPS adl_compiler;
	aig_tk::STRIPS_Problem strips_prob;

	/**
	 * PARSING
	 * Parser that creates the problem representation given a domain and an instance pddl file
	 */
	adl_compiler.get_problem_description(domain, instance, strips_prob, true);
	get_primitive_concepts_relations(strips_prob);

	std::vector<aig_tk::Node*> plan;
	get_plan(strips_prob, plan);
	output_plan(plan, 0., 0.);

	std::cout << std::endl;
	std::cout << "Actions: " << strips_prob.num_actions();
	std::cout << " Fluents: " << strips_prob.num_fluents() << std::endl;

	std::string txt_output_filename("prob.txt.strips");
	encode_as_text(strips_prob, txt_output_filename);

	return 0;
}
