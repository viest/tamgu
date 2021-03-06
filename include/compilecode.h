/*
 *  Tamgu (탐구)
 *
 * Copyright 2019-present NAVER Corp.
 * under BSD 3-clause
 */
/* --- CONTENTS ---
 Project    : Tamgu (탐구)
 Version    : See tamgu.cxx for the version number
 filename   : compilecode.h
 Date       : 2017/09/01
 Purpose    :  
 Programmer : Claude ROUX (claude.roux@naverlabs.com)
 Reviewer   :
*/

#ifndef compile_h
#define compile_h

class TamguActionVariable;
class TamguCallFunction;
class TamguFunctionLambda;

//TamguCode is a class in which programs are loaded...
class TamguCode {
private:


	//this is where we record functions, variables and instructions
	TamguCode* loader;



public:
    TamguMainFrame mainframe;
    size_t currentline;
    size_t current_start, current_end;

	//This is a pointer to the global object with which this code space is linked...
	TamguGlobal* global;
    basebin_hash<TamguActionVariable*> variables;
    basebin_hash<Tamgu*> fibrevariables;
	uchar featureassignment;

	string filename;
    string currentpredicatename;

    size_t firstinstruction;
    long insidecall;
	short currentfileid;
	short idcode;

	bool isprivate;
	bool iscommon;
	bool isconstant;

	bool compilemode;

	Tamgu* EvaluateVariable(Tamgu* var);

	TamguCode(short i, string& f, TamguGlobal* g) : idcode(i), filename(f), currentline(0), mainframe(a_mainframe, false, NULL) {
		compilemode = true;
        insidecall = 0;
        currentpredicatename="";
        currentfileid = (short)g->filenames.size();
		g->filenames.push_back(f);
		g->frames[a_mainframe] = &mainframe;
		loader = NULL;
		isprivate = false;
		iscommon = false;
		isconstant = false;
		firstinstruction = 0;
		global = g;
		current_start = 0;
		current_end = 0;
		featureassignment = 0;
	}

	~TamguCode() {
		mainframe.cleaning();
	}

	short SetFilename(string f) {
		f = NormalizeFileName(f);

		currentfileid = (short)global->filenames.size();
		global->filenames.push_back(f);
		filename = f;
		return currentfileid;
	}

	inline Tamgu* Mainframe() {
		return &mainframe;
	}

	long InstructionSize() {
		return mainframe.instructions.size();
	}

	long Computecurrentline(int i, x_node* xn);

	long Getcurrentline() {
		return (long)currentline;
	}

	short Getfileid(string f) {
		for (short i = 0; i < global->filenames.size(); i++) {
			if (global->filenames[i] == f)
				return i;
		}
		return -1;
	}


	bool Load(x_reading& xr);
	bool Compile(string& code);
	Tamgu* Compilefunction(string& code);

	bool Loadlibrary(string n, string& library_name);
	//------------------------------
    Tamgu* Run(bool glock = false);
	Tamgu* Execute(long i, short idthread);
	Tamgu* Loading();
	//---------------------------------------------
	Tamgu* Declaror(short id);
	Tamgu* Declaror(short id, Tamgu* parent);

	Tamgu* Declaration(short id);
	Tamgu* Declaration(short id, Tamgu* parent);
    Tamgu* Frame();
	bool isDeclared(short id);
	//---------------------------------------------
	//This method traverses a x_node tree to create the adequate elements...
	Tamgu* Traverse(x_node* xn, Tamgu* parent);
	void BrowseVariable(x_node* xn, Tamgu* kf);
	void BrowseVariableVector(x_node* xn, Tamgu* kf);
	void BrowseVariableMap(x_node* xn, Tamgu* kf);
	void DeclareVariable(x_node* xn, Tamgu* kf);

    TamguCallFunction* CreateCallFunction(Tamgu* function, Tamgu* parent);
	//---------------------------------------
	TamguInstruction* TamguCreateInstruction(Tamgu* kf, short op);
	Tamgu* CloningInstruction(TamguInstruction* ki);

	Tamgu* Callingprocedure(x_node*, short id);
	//----------------------------------------
	//The methods which need to be implemented to compile a code
    Tamgu* C_expression(x_node*, Tamgu*);
	Tamgu* C_parameterdeclaration(x_node*, Tamgu*);
	Tamgu* C_multideclaration(x_node*, Tamgu*);
	Tamgu* C_subfunc(x_node*, Tamgu*);
	Tamgu* C_variable(x_node*, Tamgu*);
	bool isaFunction(string& name, short id);
	Tamgu* C_regularcall(x_node*, Tamgu*);
	Tamgu* C_taskellcall(x_node*, Tamgu*);
	Tamgu* C_indexes(x_node*, Tamgu*);
	Tamgu* C_interval(x_node*, Tamgu*);
	Tamgu* C_astring(x_node*, Tamgu*);
    Tamgu* C_ustring(x_node*, Tamgu*);
    Tamgu* C_rstring(x_node*, Tamgu*);
    Tamgu* C_pstring(x_node*, Tamgu*);
	Tamgu* C_anumber(x_node*, Tamgu*);
	Tamgu* C_axnumber(x_node*, Tamgu*);
	Tamgu* C_frame(x_node* xn, Tamgu* kf);
	Tamgu* C_extension(x_node* xn, Tamgu* kf);
	Tamgu* C_affectation(x_node* xn, Tamgu* kf);
	Tamgu* C_operator(x_node* xn, Tamgu* kf);
	Tamgu* C_increment(x_node* xn, Tamgu* kf);
	Tamgu* C_valvector(x_node* xn, Tamgu* kf);
	Tamgu* C_jsonvector(x_node* xn, Tamgu* kf);

	Tamgu* C_dependencyrule(x_node* xn, Tamgu* kf);
	Tamgu* C_dependency(x_node* xn, Tamgu* kf);
	Tamgu* C_dependencyresult(x_node* xn, Tamgu* kf);

	Tamgu* C_intentionvector(x_node* xn, Tamgu* kf);
	Tamgu* C_intentionwithdouble(x_node* xn, Tamgu* kf);

	Tamgu* C_valmap(x_node* xn, Tamgu* kf);

	Tamgu* C_conceptfunction(x_node* xn, Tamgu* kf);

	Tamgu* C_jsonmap(x_node* xn, Tamgu* kf);
	Tamgu* C_dico(x_node* xn, Tamgu* kf);
	Tamgu* C_jsondico(x_node* xn, Tamgu* kf);
	Tamgu* C_features(x_node* xn, Tamgu* kf);

	Tamgu* C_parenthetic(x_node* xn, Tamgu* kf);
	Tamgu* C_operation(x_node* xn, Tamgu* kf);
	Tamgu* C_multiply(x_node* xn, Tamgu* kf);
	Tamgu* C_plusplus(x_node* xn, Tamgu* kf);
	Tamgu* C_operationin(x_node* xn, Tamgu* kf);
    Tamgu* C_comparison(x_node* xn, Tamgu* kf);
    Tamgu* C_taskcomparison(x_node* xn, Tamgu* kf);
	Tamgu* C_constant(x_node* xn, Tamgu* kf);
	Tamgu* C_negated(x_node* xn, Tamgu* kf);
	Tamgu* C_uniquecall(x_node* xn, Tamgu* kf);
	Tamgu* C_createfunction(x_node* xn, Tamgu* kf);
	Tamgu* C_blocs(x_node* xn, Tamgu* kf);
	bool CheckUse(x_node* xn, Tamgu* kf);

    Tamgu* C_tamgulisp(x_node* xn, Tamgu* kf);
    Tamgu* C_tamgulispquote(x_node* xn, Tamgu* kf);
    Tamgu* C_tamgulispvariable(x_node* xn, Tamgu* kf);
    Tamgu* C_tamgulispatom(x_node* xn, Tamgu* kf);

	Tamgu* C_ifcondition(x_node* xn, Tamgu* kf);

	Tamgu* C_negation(x_node* xn, Tamgu* kf);

	Tamgu* C_booleanexpression(x_node* xn, Tamgu* kf);
	
	Tamgu* C_switch(x_node* xn, Tamgu* kf);
	Tamgu* C_testswitch(x_node* xn, Tamgu* kf);

	Tamgu* C_trycatch(x_node* xn, Tamgu* kf);

	Tamgu* C_loop(x_node* xn, Tamgu* kf);
	Tamgu* C_doloop(x_node* xn, Tamgu* kf);

	Tamgu* C_for(x_node* xn, Tamgu* kf);
	Tamgu* C_blocfor(x_node* xn, Tamgu* kf);
	Tamgu* C_forin(x_node* xn, Tamgu* kf);
	Tamgu* C_parameters(x_node* xn, Tamgu* kf);

	//Taskell

    Tamgu* C_taskellbasic(x_node* xn, Tamgu* kf);

    Tamgu* C_curryingleft(x_node* xn, Tamgu* kf);
    Tamgu* C_curryingright(x_node* xn, Tamgu* kf);

	Tamgu* C_hdata(x_node* xn, Tamgu* kf);
    Tamgu* C_hdeclaration(x_node* xn, Tamgu* kf);
    Tamgu* C_hdatadeclaration(x_node* xn, Tamgu* kf);
	Tamgu* C_telque(x_node* xn, Tamgu* kf);
	Tamgu* C_ontology(x_node* xn, Tamgu* kf);

	Tamgu* C_hbloc(x_node* xn, Tamgu* kf);
	Tamgu* C_ParseReturnValue(x_node* xn, TamguFunctionLambda* kf, char adding = false);
	bool CheckVariable(x_node* xn, Tamgu* kf);
	bool CheckingVariableName(x_node* xn, Tamgu* kf);

    //Taskell compiling functions
	Tamgu* C_declarationtaskell(x_node* xn, Tamgu* kf);
	Tamgu* C_hlambda(x_node* xn, Tamgu* kf);
	Tamgu* C_hcompose(x_node* xn, Tamgu* kf);
	Tamgu* C_hfunctioncall(x_node* xn, Tamgu* kf);
	Tamgu* C_taskellalltrue(x_node* xn, Tamgu* kf);
    Tamgu* C_taskellboolchecking(x_node* xn, Tamgu* kf);
	Tamgu* C_folding(x_node* xn, Tamgu* kf);
	Tamgu* C_zipping(x_node* xn, Tamgu* kf);
	Tamgu* C_filtering(x_node* xn, Tamgu* kf);
	Tamgu* C_mapping(x_node* xn, Tamgu* kf);
	Tamgu* C_flipping(x_node* xn, Tamgu* kf);
	Tamgu* C_cycling(x_node* xn, Tamgu* kf);
	Tamgu* C_taskellexpression(x_node* xn, Tamgu* kf);
	Tamgu* C_taskellmap(x_node* xn, Tamgu* kf);
	Tamgu* C_taskellvector(x_node* xn, Tamgu* kf);
	Tamgu* C_whereexpression(x_node* xn, Tamgu* kf);
	Tamgu* C_hinexpression(x_node* xn, Tamgu* kf);
	Tamgu* C_letmin(x_node* xn, Tamgu* kf);
	Tamgu* C_taskellcase(x_node* xn, Tamgu* kf);
	Tamgu* C_guard(x_node* xn, Tamgu* kf);
	Tamgu* C_dataassignment(x_node* xn, Tamgu* kf);

    //Rules functions
    Tamgu* C_annotationrule(x_node* xn, Tamgu* kf);
    Tamgu* C_annotation(x_node* xn, Tamgu* kf);
    Tamgu* C_listoftokens(x_node* xn, Tamgu* kf);
    Tamgu* C_sequenceoftokens(x_node* xn, Tamgu* kf);
    Tamgu* C_optionaltokens(x_node* xn, Tamgu* kf);
    Tamgu* C_removetokens(x_node* xn, Tamgu* kf);
    Tamgu* C_token(x_node* xn, Tamgu* kf);

    
    //Predicate compiling functions
	Tamgu* C_alist(x_node* xn, Tamgu* kf);
	Tamgu* C_cut(x_node* xn, Tamgu* kf);
	Tamgu* C_dcg(x_node* xn, Tamgu* kf);
	Tamgu* C_predicatefact(x_node* xn, Tamgu* kf);
	Tamgu* C_predicate(x_node* xn, Tamgu* kf);
	Tamgu* C_predicateexpression(x_node* xn, Tamgu* kf);
	Tamgu* C_predicatevariable(x_node* xn, Tamgu* kf);
	Tamgu* C_assertpredicate(x_node* xn, Tamgu* kf);
	Tamgu* C_term(x_node* xn, Tamgu* kf);
	Tamgu* C_comparepredicate(x_node* xn, Tamgu* kf);

	Tamgu* TamguParsePredicateDCGVariable(string& name, Tamgu* kcf, bool param);
	void ComputePredicateParameters(x_node* xn, Tamgu* kcf);

};

#endif






