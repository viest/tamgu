/*
 *  Tamgu (탐구)
 *
 * Copyright 2019-present NAVER Corp.
 * under BSD 3-clause
 */
/* --- CONTENTS ---
 Project    : Tamgu (탐구)
 Version    : See tamgu.cxx for the version number
 filename   : codeexecute.cxx
 Date       : 2017/09/01
 Purpose    : 
 Programmer : Claude ROUX (claude.roux@naverlabs.com)
 Reviewer   :
*/

#include "tamgu.h"
#include "instructions.h"
#include "compilecode.h"
#include "tamgustring.h"
#include "tamgusvector.h"
#include "constobjects.h"
#include "tamgumap.h"
#include <memory>
#include "tamgutamgu.h"
#include "tamgudecimal.h"
#include "predicate.h"
#include "tamguhvector.h"
#include "tamguframeinstance.h"
#include "tamgutaskell.h"
#include "tamgufraction.h"
#include "tamgubyte.h"

#ifdef INTELINTRINSICS
#ifdef WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif

//--------------------------------------------------------------------
static hmap<string, Tamgu*> throughvariables;
static hmap<string, string> throughvariabletype;

void RetrieveThroughVariables(string& declaration) {
    declaration += "\n";
    for (auto& u : throughvariabletype) {
        declaration += u.second;
        declaration += " ";
        declaration += u.first;
        declaration += ";\n";
    }
}

void RetrieveThroughVariables(wstring& decl) {
    string declaration = "\n";
    for (auto& u : throughvariabletype) {
        declaration += u.second;
        declaration += " ";
        declaration += u.first;
        declaration += ";\n";
    }
    s_utf8_to_unicode(decl,USTR(declaration),declaration.size());
}

bool CheckThroughVariables() {
    if (throughvariables.size())
        return true;
    return false;
}
//--------------------------------------------------------------------
bool TestEndthreads();
//--------------------------------------------------------------------
#ifdef WIN32
static const char TamguOS[] = "WINDOWS";
#else
#ifdef APPLE
static const char TamguOS[] = "MACOS";
#else
static const char TamguOS[] = "UNIX";
#endif
#endif

//--------------------------------------------------------------------
Exporting TamguSystemVariable::TamguSystemVariable(TamguGlobal* g, Tamgu* v, short n, short t) : value(v), TamguVariableDeclaration(g, n, t) {
	g->systems[n] = this;
	if (v->idtracker == -1)
		g->RecordInTracker(v);
}

//--------------------------------------------------------------------

void TamguGlobal::RecordSystemVariables() {
	Tamgu* a;

	a = new TamguSystemVariable(this, aNULL, a_null, a_const);
	a = new TamguSystemVariable(this, aNOELEMENT, a_empty, a_const);
    a = new TamguSystemVariable(this, aUNIVERSAL, a_universal, a_const);
    a = new TamguSystemVariable(this, aNOTHING, a_Nothing, a_const);

	a = new TamguSystemVariable(this, new TamguConstString(""), Createid("_current"), a_string);
	a = new TamguSystemVariable(this, new TamguConstSVector, Createid("_paths"), a_svector);
	a = new TamguSystemVariable(this, new TamguConstSVector, Createid("_args"), a_svector);

	a = new TamguSystemVariable(this, new TamguConstString(TamguOS), Createid("_OS"), a_string);

#ifdef WIN32
	a = new TamguSystemVariable(this, new TamguConstString("\\"), Createid("_sep"), a_string);
	a = new TamguSystemVariable(this, new TamguConstString("\r\n"), Createid("_endl"), a_string);
#else
	a = new TamguSystemVariable(this, new TamguConstString("/"), Createid("_sep"), a_string);
	a = new TamguSystemVariable(this, new TamguConstString("\n"), Createid("_endl"), a_string);
#endif

}

void TamguGlobal::SystemInitialization(string spath) {

	char localpath[4096];
	localpath[0] = 0;

#ifdef WIN32
	_fullpath(localpath, ".", 4096);
#else
	realpath(".", localpath);
#endif

#ifdef WIN32
	long pos = spath.rfind("\\");
	if (pos == string::npos) {
		spath = localpath;
		if (localpath[spath.size() - 1] != '\\')
			spath += "\\";
	}
	else
		spath = spath.substr(0, pos + 1);
#else
	long pos = spath.rfind("/");
	if (pos == string::npos) {
		spath = localpath;
		if (localpath[spath.size() - 1] != '/')
			spath += "/";
	}
	else
		spath = spath.substr(0, pos + 1);
#endif
	TamguConstString* s = (TamguConstString*)systems[Getid("_current")]->value;
	s->value = spath;

	short i;
	TamguConstSVector* v = (TamguConstSVector*)systems[Getid("_paths")]->value;
	v->values.clear();


	v->values.push_back(localpath);

	for (i = 0; i < spaces.size(); i++)
		v->values.push_back(spaces[i]->filename);

	v = (TamguConstSVector*)systems[Getid("_args")]->value;
	v->values.clear();

	for (i = 0; i < arguments.size(); i++)
		v->values.push_back(arguments[i]);
}

void TamguGlobal::Initarguments(vector<string>& args) {
    TamguConstSVector* v = (TamguConstSVector*)systems[Getid("_args")]->value;
    v->values.clear();
    
    for (long i = 0; i < args.size(); i++)
        v->values.push_back(args[i]);
}

//--------------------------------------------------------------------

void TamguDeclaration::Cleaning(short idthread) {
	short j;
	BULONG filter;
	if (declarations.base == -1)
		return;

    unsigned long qj;
    
    for (short ii = 0; ii < declarations.tsize; ii++) {
        filter = declarations.indexes[ii];
        if (filter) {
            j = 0;
            while (filter) {
#ifdef INTELINTRINSICS
                if (!(filter & 1)) {
                    if (!(filter & 0x00000000FFFFFFFF)) {
                        filter >>= 32;
                        j += 32;
                    }
					bitscanforward(qj, (uint32_t)(filter & 0x00000000FFFFFFFF));
                    filter >>= qj;
                    j += qj;
                }
#else
                if (!(filter & 1)) {
                    while (!(filter & 65535)) {
                        filter >>= 16;
                        j = j + 16;
                    }
                    while (!(filter & 255)) {
                        filter >>= 8;
                        j = j + 8;
                    }
                    while (!(filter & 15)) {
                        filter >>= 4;
                        j = j + 4;
                    }
                    while (!(filter & 1)) {
                        filter >>= 1;
                        j++;
                    }
                }
#endif
				declarations.table[ii][j]->Resetreference();
				declarations.table[ii][j] = NULL;
				globalTamgu->Removevariable(idthread, ((ii + declarations.base) << binbits) + j);
				filter >>= 1;
				j++;
			}
			declarations.indexes[ii] = 0;
		}
	}
}

//--------------------------------------------------------------------
Tamgu* TamguGlobal::EvaluateMainVariable() {
	TamguCode* acode = Getcurrentcode();
	TamguFrame* frame = (TamguFrame*)acode->Mainframe();
	Tamgu* res;
	for (short i = 0; i < frame->instructions.size(); i++) {
        res = frame->instructions.vecteur[i];
		if (res->isVariable()) {
			res = res->Eval(frame, aNULL, 0);
			if (res->needInvestigate())
				return res;
		}
	}
	return aTRUE;
}

//--------------------------------------------------------------------
Exporting Tamgu* TamguExecute(TamguCode* code, string name, vector<Tamgu*>& params) {

	globalTamgu->threadMODE = true;

	short idthread = globalTamgu->GetThreadid();
    globalTamgu->Cleanerror(0);

	short id = globalTamgu->Getid(name);
	Tamgu* func = code->Mainframe()->Declaration(id);

	if (func == NULL || !func->isFunction()) {
		name = "Unknown function: " + name;
		return globalTamgu->Returnerror(name, idthread);
	}

	short i;
	TamguCallFunction call(func);
	for (i = 0; i < params.size(); i++) {
		call.arguments.push_back(params[i]);
		params[i]->Setreference();
	}

	globalTamgu->Pushstack(code->Mainframe(), idthread);
	func = call.Eval(aNULL, aNULL, idthread);
	globalTamgu->Popstack(idthread);

	for (i = 0; i < params.size(); i++)
		params[i]->Resetreference();
	return func;
}

Exporting Tamgu* TamguExecute(TamguCode* code, string name, vector<Tamgu*>& params, short idthread) {
	Tamgu* main = code->Mainframe();

	globalTamgu->threadMODE = true;
    globalTamgu->Cleanerror(idthread);


	short id = globalTamgu->Getid(name);
	Tamgu* func = main->Declaration(id);

	if (func == NULL || !func->isFunction()) {
		name = "Unknown function: " + name;
		return globalTamgu->Returnerror(name, idthread);
	}

	short i;
	TamguCallFunction call(func);
	for (i = 0; i < params.size(); i++) {
		call.arguments.push_back(params[i]);
		params[i]->Setreference();
	}
	
	globalTamgu->Pushstack(main, idthread);
	func = call.Eval(aNULL, aNULL, idthread);
	globalTamgu->Popstack(idthread);

	for (i = 0; i < params.size(); i++)
		params[i]->Resetreference();
	return func;
}

Tamgu* TamguCode::Execute(long begininstruction, short idthread) {
	Tamgu* a = aNULL;
    global->Cleanerror(idthread);

	short sz = mainframe.instructions.size();
	_setdebugmin(idthread);
	short i;

	for (i = begininstruction; i < sz; i++) {
        a = mainframe.instructions.vecteur[i];

		_debugpush(a);
		a = a->Eval(&mainframe, aNULL, idthread);
		_debugpop();

		if (global->Error(idthread)) {
			_cleandebugmin;
			a->Releasenonconst();
			a = global->Errorobject(idthread);
			break;
		}

		a->Releasenonconst();
		a = aNULL;
	}

	mainframe.instructions.last = begininstruction;

	_cleandebugmin;
	return a;
}

Tamgu* TamguCode::Loading() {
    
    //These are atomic values that need to be set before all
    globalTamgu->threadMODE = false;


	short idthread = 0;
	global->InitThreadid(idthread);

	global->Pushstack(&mainframe);
	_setdebugmin(0);

	Tamgu* a = aNULL;
	short sz = mainframe.instructions.size();

	for (short i = firstinstruction; i < sz; i++) {
        a = mainframe.instructions.vecteur[i];

		_debugpush(a);
		a = a->Eval(&mainframe, aNULL, 0);
		_debugpop();

		if (global->Error(0)) {
			a->Releasenonconst();
			a = global->Errorobject(0);
			break;
		}

		a->Releasenonconst();
		a = aNULL;
	}


	_cleandebugmin;
	global->Popstack(0);

	return a;
}

Tamgu* TamguCode::Run(bool glock) {

    executionbreak = false;

    //These are atomic values that need to be set before all
    global->running = true;
    global->waitingonfalse = false;
    global->threadMODE = glock;
    global->isthreading = glock;
    global->threadcounter = 0;

    global->Cleanerror(0);
    
	short idthread = 0;
	global->InitThreadid(idthread);

	global->Pushstack(&mainframe);
	_setdebugmin(0);

	Tamgu* a = aNULL;
	short sz = mainframe.instructions.size();
    
    bool testcond = false;
    
	for (short i = firstinstruction; i < sz && !testcond; i++) {
        a->Releasenonconst();
        a = mainframe.instructions.vecteur[i];
        
		_debugpush(a);
		a = a->Eval(&mainframe, aNULL, 0);
		_debugpop();

        testcond = global->Error(0) || executionbreak;
    }
     
    a->Releasenonconst();
    a = aNULL;
    if (testcond) {
		if (global->Error(0)) {
			a = global->Errorobject(0);
		}
	}

	_cleandebugmin;
	global->Popstack(0);


    global->Clearfibres(0);
    if (a != aNULL) {
        if (global->threads[0].currentinstruction != NULL) {
            currentline = global->threads[0].currentinstruction->Currentline();
            currentfileid = global->threads[0].currentinstruction->Currentfile();
            if (currentfileid != -1)
                filename = global->filenames[currentfileid];
        }
    }
    
    if (global->terminationfunction != NULL)
        (*global->terminationfunction)(global);
    
    executionbreak = true;
    
    //we wait for all threads to end, before leaving...
	while (global->threadcounter) {}

    global->Releasevariables();
    global->isthreading = false;
    global->threads[0].used = false;
    global->running = false;

	return a;
}

//--------------------------------------------------------------------
Tamgu* Tamgustring::EvalIndex(Tamgu* kidx, TamguIndex* idx, short idthread) {
    static Fast_String basestr(1024);
    
    long ileft, iright;
    
    locking();
    char res = StringIndexes(value, idx, ileft, iright, idthread);
    if (!res) {
        if (globalTamgu->erroronkey)
            return globalTamgu->Returnerror("Wrong key in a container or a string access", idthread);
        return aNOELEMENT;
    }

    kidx = idx->function;
    if (kidx == NULL) {
        Tamgustring* inter = globalTamgu->Providestring();
        if (res == 1)
            inter->value = c_char_get(USTR(value), ileft);
        else
            inter->value = value.substr(ileft, iright - ileft);
        unlocking();
        return inter;
    }
    Fast_String* str = &basestr;
    
    if (idthread)
        str = new Fast_String(value.size());
    
    if (res == 1)
        str->addonechar(USTR(value), ileft);
    else
        str->substr(USTR(value), ileft, iright);
    unlocking();

    while (kidx != NULL && kidx->isIndex()) {
        res = StringIndexes(str->neo, str->size(), kidx, ileft, iright, idthread);
        if (!res) {
            if (globalTamgu->threadMODE)
                delete str;
            if (globalTamgu->erroronkey)
                return globalTamgu->Returnerror("Wrong key in a container or a string access", idthread);
            return aNOELEMENT;
        }
        if (res == 1)
            str->addonechar(ileft);
        else
            str->substr(ileft, iright);
        
        kidx = kidx->Function();
    }
    
    Tamgustring* inter = globalTamgu->Providestring();
    inter->value = str->str();
    if (idthread)
        delete str;

    if (kidx != NULL) {
        kidx = kidx->Eval(aNULL, inter, idthread);
        if (kidx != inter)
            inter->Release();
        return kidx;
    }
    
    return inter;
}

Tamgu* Tamguustring::EvalIndex(Tamgu* kidx, TamguIndex* idx, short idthread) {
    long ileft, iright;
    
#ifdef WSTRING_IS_UTF16
    uint32_t c;
#endif

    locking();
    char res = StringIndexes(value, idx, ileft, iright, idthread);
    if (!res) {
        if (globalTamgu->erroronkey)
            return globalTamgu->Returnerror("Wrong key in a container or a string access", idthread);
        return aNOELEMENT;
    }

    Tamguustring* inter = globalTamgu->Provideustring();

    if (res == 1) {
#ifdef WSTRING_IS_UTF16
        c = value[ileft];
        if (checklargeutf16(c))
            inter->value = c + value[ileft + 1];
        else
            inter->value = c;
#else
        inter->value = value[ileft];
#endif
    }
    else
        inter->value = value.substr(ileft, iright - ileft);
    unlocking();
    
    kidx = idx->function;
    if (kidx == NULL)
        return inter;

    while (kidx != NULL && kidx->isIndex()) {
        res = StringIndexes(inter->value, kidx, ileft, iright, idthread);
        if (!res) {
            inter->Release();
            if (globalTamgu->erroronkey)
                return globalTamgu->Returnerror("Wrong key in a container or a string access", idthread);
            return aNOELEMENT;
        }
        if (res == 1) {
#ifdef WSTRING_IS_UTF16
            c = inter->value[ileft];
            if (checklargeutf16(c))
                inter->value = c + inter->value[ileft + 1];
            else
                inter->value = c;
#else
            inter->value = inter->value[ileft];
#endif
        }
        else
            inter->value = inter->value.substr(ileft, iright - ileft);
        kidx = kidx->Function();
    }
    
    if (kidx != NULL) {
        kidx = kidx->Eval(aNULL, inter, idthread);
        if (kidx != inter)
            inter->Release();
        return kidx;
    }
    
    return inter;
}
 
Tamgu* TamguIndex::Eval(Tamgu* klocal, Tamgu* obj, short idthread) {
    if (obj->isPureString())
        return obj->EvalIndex(klocal, this, idthread);
        
    if (function == NULL) {
        obj = obj->Eval(klocal, this, idthread);
        obj->Enablelock(obj->isToBelocked());
        return obj;
    }

    klocal = obj->Eval(klocal, this, idthread);
    unsigned short istobelocked = obj->isToBelocked();

	if (function->isIncrement()) {
		if (klocal == aNOELEMENT && obj->isValueContainer()) {
            if (obj->isString()) {
				klocal = globalTamgu->Providestring();
                klocal->Enablelock(istobelocked);
            }
			else
			if (obj->isFloat())
				klocal = globalTamgu->Providefloat(0);
			else
				klocal = globalTamgu->Provideint(0);
		}
        else
            klocal->Enablelock(istobelocked);

		//we increment
		function->Eval(aNULL, klocal, idthread);

		if (obj->isValueContainer()) {
			//we store the value back into place
			//we reevaluate our indexes...
			obj->Put(this, klocal, idthread);
		}

		return klocal;
	}

    klocal->Enablelock(istobelocked);
    
	Tamgu* kidx = function;
	Tamgu* object = klocal;


	while (kidx != NULL && kidx->isIndex()) {
		if (object == aNOELEMENT) {
			if (globalTamgu->erroronkey)
				return globalTamgu->Returnerror("Wrong key in a container or a string access", idthread);
			return aNOELEMENT;
		}

		obj = object;
		klocal = object->Eval(aNULL, kidx, idthread);
        klocal->Enablelock(istobelocked);
        
		if (klocal != object) {
			object->Releasenonconst();
			object = klocal;
		}

		kidx = kidx->Function();
	}

	if (kidx != NULL) {
		if (klocal == aNOELEMENT) {
			if (!obj->isValueContainer() && kidx->Name() == a_push) {
				//if it is not a value container, then it can contain a vector...
				klocal = globalTamgu->Providevector();
				obj->Put(this, klocal, idthread);
			}
			else {
				if (globalTamgu->erroronkey)
					return globalTamgu->Returnerror("Wrong key in a container or a string access", idthread);
				return aNOELEMENT;
			}
		}

		if (kidx->isIncrement()) {
			kidx->Eval(aNULL, klocal, idthread);
			if (obj->isValueContainer()) {
				obj->Put(this, klocal, idthread);
			}
            klocal->Enablelock(istobelocked);
			return klocal;
		}

		object = kidx->Eval(aNULL, klocal, idthread);
		if (object != klocal)
			klocal->Releasenonconst();
        object->Enablelock(istobelocked);
		return object;
	}

	return klocal;
}

Tamgu* TamguIndex::Put(Tamgu* recipient, Tamgu* value, short idthread) {
	TamguIndex* idx;
    
	if (function == NULL) {
        recipient = recipient->Put(this, value, idthread);
		return aTRUE;
	}

    unsigned short istobelocked = recipient->isToBelocked();

	if (function->isIndex()) {
		idx = this;
		Tamgu* intermediate = recipient;
		vector<Tamgu*> stack;

		while (idx->function != NULL && idx->function->isIndex()) {
			stack.push_back(idx);
			stack.push_back(intermediate);
			intermediate = intermediate->Eval(aNULL, idx, idthread);
            intermediate->Enablelock(istobelocked);
			stack.push_back(intermediate);
			idx = (TamguIndex*)idx->function;
		}

		intermediate->Put(idx, value, idthread);

		for (long i = stack.size() - 1; i >= 0; i -= 3) {
			intermediate = stack[i];
			if (!intermediate->isProtected() || intermediate->Reference())
				break;
			stack[i-1]->Put(stack[i-2], intermediate, idthread);
		}
	}

	return aTRUE;
}

//----------------------------------------------------------------------------------
//This is actually a line of code, which when it is executed records a new variable into the declaration space...
TamguVariableDeclaration::TamguVariableDeclaration(TamguGlobal* g, short n, short t, bool p, bool c, Tamgu* parent) : TamguTracked(a_declaration, g, parent) {
	if (parent != NULL)
		parent->SetVariablesWillBeCreated();
	
	name = n;
	typevariable = t;
	isprivate = p;
	initialization = NULL;
	function = NULL;
	choice = 0;
	isconstant = c;

	isframe = false;
	if (g != NULL && g->newInstance.check(typevariable))
		isframe = g->newInstance.get(typevariable)->isFrame();
    investigate = is_variable;
}

TamguVariableDeclaration::TamguVariableDeclaration(TamguGlobal* g, short n, short t, Tamgu* parent) : TamguTracked(a_declaration, g, parent) {
	if (parent != NULL)
		parent->SetVariablesWillBeCreated();

	name = n;
	typevariable = t;
	isprivate = false;
	initialization = NULL;
	function = NULL;
	choice = 0;
	isframe = false;
	isconstant = false;

	if (g != NULL && g->newInstance.check(typevariable))
		isframe = g->newInstance.get(typevariable)->isFrame();

    investigate = is_variable;
}

//When we call this function, we actually will create an element of type value
Tamgu* TamguVariableDeclaration::Eval(Tamgu* domain, Tamgu* value, short idthread) {
	//we create our instance...

	Tamgu* a = globalTamgu->newInstance.get(typevariable)->Newinstance(idthread, function);
	a->Setname(name);
	domain->Declare(name, a);
	globalTamgu->Storevariable(idthread, name, a);
	a->Setreference(1);

	if (initialization != NULL) {
		if (initialization->Name() == a_initial) {
			value = initialization->Eval(domain, a, idthread);
			if (value->isError())
				return value;
		}
		else {
			bool aff = a->checkAffectation();
			value = initialization->Eval(a, aAFFECTATION, idthread);
			if (value->isError())
				return value;
			if (value != a)
				a->Put(aNULL, value, idthread);
            if (!aff)
                a->Setaffectation(false);
		}

		value->Releasenonconst();
	}

	//If it is a frame, then we instanciate local frame declaration here...
	//Hence, intialization of local frames can depend on local frame variables...
	a->Postinstantiation(idthread, true);

	return a;
}

Tamgu* TamguAtomicVariableDeclaration::Eval(Tamgu* domain, Tamgu* value, short idthread) {
    //we create our instance...
    
    if (directcall) {
        if (directcall == 2) {
            value = reference->Newvalue(initialization, idthread);
            domain->Declare(name, value);
            globalTamgu->Storevariable(idthread, name, value);
            value->Setreference(1);
            return value;
        }
        
        value = initialization->Eval(constant, aNULL, idthread);

        if (!value->Checktype(typevariable)) {
            Tamgu* a = reference->Newvalue(value, idthread);
            domain->Declare(name, a);
            globalTamgu->Storevariable(idthread, name, a);
            a->Setreference(1);
            value->Releasenonconst();
            return a;
        }
    }
    else
        value = reference->Newinstance(idthread, function);
    
    domain->Declare(name, value);
    globalTamgu->Storevariable(idthread, name, value);
    value->Setreference(1);
    return value;
}

Tamgu* TamguAtomicVariableDeclaration::Put(Tamgu* domain, Tamgu* value, short idthread) {
    Tamgu* a = reference->Newinstance(idthread);
    domain->Declare(name, a);
    globalTamgu->Storevariable(idthread, name, a);
    a->Setreference();
    a->Putatomicvalue(value);
    return a;
}


Tamgu* TamguFrameVariableDeclaration::Eval(Tamgu* domain, Tamgu* value, short idthread) {
	//we create our instance...
	Tamgu* a;
	if (common) {
		a = domain->Frame()->Declaration(name);
		if (a == this)
			a = TamguVariableDeclaration::Eval(domain->Frame(), aNULL, idthread);
        
		domain->Declare(name, a);
		return a;
	}

	a = globalTamgu->newInstance.get(typevariable)->Newinstance(idthread, function);
	a->Setname(name);
	domain->Declare(name, a);

	//The only difference with the previous one is that we do not set the reference here...

	if (initialization != NULL) {
		if (initialization->Name() == a_initial) {
			//We protect our new variable against any potential destruction
			a->Setreference();
			value = initialization->Eval(domain, a, idthread);
			//But we need to reset the internal references to 0, without deleting any of these elements.
			a->Protect();

			if (value->isError())
				return value;
		}
		else {
			bool aff = a->checkAffectation();
			value = initialization->Eval(a, aAFFECTATION, idthread);
			if (value->isError())
				return value;
			if (value != a)
				a->Put(aNULL, value, idthread);
            if (!aff)
                a->Setaffectation(false);
		}

		value->Releasenonconst();
	}

	//If it is a frame, then we instanciate local frame declaration here...
	//Hence, intialization of local frames can depend on local frame variables...

	a->Postinstantiation(idthread, false);

	return a;
}

Tamgu* TamguFrameAtomicVariableDeclaration::Eval(Tamgu* domain, Tamgu* value, short idthread) {
    //we create our instance...
    
    if (common) {
        value = domain->Frame()->Declaration(name);
        if (value == this)
            value = TamguAtomicVariableDeclaration::Eval(domain->Frame(), aNULL, idthread);
        domain->Declare(name, value);
        return value;
    }
    
    if (directcall) {
        if (directcall == 2) {
            value = reference->Newvalue(initialization, idthread);
            domain->Declare(name, value);
            value->Setreference(1);
            return value;
        }
        
        value = initialization->Eval(domain, aNULL, idthread);
        
        if (!value->Checktype(typevariable)) {
            Tamgu* a = reference->Newvalue(value, idthread);
            domain->Declare(name, a);
            a->Setreference(1);
            value->Releasenonconst();
            return a;
        }
    }
    else
        value = reference->Newinstance(idthread, function);
    
    domain->Declare(name, value);
    value->Setreference(1);
    return value;
}

Tamgu* TamguVariableDeclaration::Put(Tamgu* domain, Tamgu* value, short idthread) {
	Tamgu* a = globalTamgu->newInstance.get(typevariable)->Newinstance(idthread);
	domain->Declare(name, a);
	globalTamgu->Storevariable(idthread, name, a);
	a->Setreference();
	return a->Put(aNULL, value, idthread);
}


bool TamguVariableDeclaration::Setvalue(Tamgu* domain, Tamgu* value, short idthread, bool strict) {
	//we create a new variable, whose type is the one from the function declaration...
	//We need to check some cases

    if (value->isError())
        return false;

    
    if (value->checkAtomType(typevariable)) {
        value = value->Atomref();
        domain->Declare(name, value);
        globalTamgu->Storevariable(idthread, name, value);
        return true;
    }

	//This is a case when creating a duplicate variable is needless or too dangerous...
	//Exemple: vector, map, primemap etc...
    if (typevariable == a_self || typevariable == a_let) {
        if (value->isConst())
            value = value->Atomref();
        else
            value->Setreference();
        domain->Declare(name, value);
        globalTamgu->Storevariable(idthread, name, value);
        return true;
    }

    if (!value->duplicateForCall()) {
        if (value->Type() == typevariable || value->isLetSelf()) {
            value->Setreference();
            domain->Declare(name, value);
            globalTamgu->Storevariable(idthread, name, value);
            return true;
        }
    }


	//we accept "null" as a default value...
	if (value->Type() == typevariable || value->isNULL() || globalTamgu->Testcompatibility(typevariable, value->Type(), strict)) {
        
        if (value->isProtected()) {
            value->Setreference();
            domain->Declare(name, value);
            globalTamgu->Storevariable(idthread, name, value);
            return true;
        }
        
		Tamgu* a = globalTamgu->newInstance.get(typevariable)->Newinstance(idthread);
		a->Setname(name);
		a->Setreference();
		domain->Declare(name, a);
		globalTamgu->Storevariable(idthread, name, a);
		if (value != aNULL)
			a->Put(aNULL, value, idthread);
		return true;
	}

	return false;
}

bool TamguAtomicVariableDeclaration::Setvalue(Tamgu* domain, Tamgu* value, short idthread, bool strict) {
    //we create a new variable, whose type is the one from the function declaration...
    //We need to check some cases
    short ty = value->Type();
    if (ty != typevariable && value != aNULL && !globalTamgu->Testcompatibility(typevariable, ty, strict))
        return false;
    if (value->isNULL()) {
        switch (typevariable) {
            case a_boolean:
                value = new Tamguboolean(true);
                value->Setreference();
                break;
            case a_byte:
                value = new Tamgubyte(0);
                value->Setreference();
                break;
            case a_short:
                value = new Tamgushort(0);
                value->Setreference();
                break;
            case a_int:
                value = globalTamgu->Provideint();
                value->Setreference();
                break;
            case a_long:
                value = new Tamgulong(0);
                value->Setreference();
                break;
            case a_decimal:
                value = new Tamgudecimal(0);
                value->Setreference();
                break;
            case a_fraction:
                value = new Tamgufraction(0,1);
                value->Setreference();
                break;
            case a_float:
                value = globalTamgu->Providefloat(0);
                value->Setreference();
                break;
            case a_string:
                value = globalTamgu->Providestring();
                value->Setreference();
                break;
            case a_ustring:
                value = globalTamgu->Provideustring();
                value->Setreference();
                break;
            default:
                value = aNULL;
        }
    }
    else
        value = value->Atomref();
    
    domain->Declare(name, value);
    globalTamgu->Storevariable(idthread, name, value);
    return true;
}

Tamgu* TamguGlobalVariableDeclaration::Eval(Tamgu* domain, Tamgu* value, short idthread) {
	//we create our instance...
	if (alreadydeclared)
		return globalTamgu->Getmaindeclaration(name, idthread);

	alreadydeclared = true;
	Tamgu* a = globalTamgu->newInstance.get(typevariable)->Newinstance(idthread, function);
    //Global Variables should always be protected with a lock
	a->Setname(name);
	domain->Declare(name, a);
	globalTamgu->Storevariable(idthread, name, a);

	a->Setreference(1);
	if (initialization != NULL) {
		if (initialization->Name() == a_initial)
			value = initialization->Eval(aNULL, a, idthread);
		else {
            bool aff = a->checkAffectation();
			value = initialization->Eval(a, aAFFECTATION, idthread);
			if (value->isError())
				return value;

			if (value != a)
				a->Putvalue(value, idthread);
            
            if (!aff)
                a->Setaffectation(false);
		}
		value->Releasenonconst();
	}

	//If it is a frame, then we instanciate local frame declaration here...
	//Hence, intialization of local frames can depend on local frame variables...
	a->Postinstantiation(idthread, true);

	return a;
}

Tamgu* TamguThroughVariableDeclaration::Eval(Tamgu* domain, Tamgu* value, short idthread) {
	//we create our instance...
	if (throughvariables.find(sname) != throughvariables.end())
		return throughvariables[sname];
	
	Tamgu* a = globalTamgu->newInstance.get(globalTamgu->throughs[typevariable])->Newpureinstance(idthread);
	a->Setname(name);

	a->Setreference(1);
	if (initialization != NULL) {
        bool aff = a->checkAffectation();
		value = initialization->Eval(a, aAFFECTATION, idthread);
		if (value->isError())
			return value;
		if (value != a)
			a->Putvalue(value, idthread);
        if (!aff)
            a->Setaffectation(false);
		value->Releasenonconst();
	}

    throughvariables[sname] = a;
    throughvariabletype[sname] = tname;

	return a;
}


//----------------------------------------------------------------------------------
bool TamguVariableDeclaration::Checkarity() {
    if (function == NULL)
        return true;
    
    unsigned long nb = globalTamgu->newInstance[typevariable]->CallBackArity();
    if (nb == P_NONE)
        return true;
    
    return Arity(nb, function->Size());
}

bool TamguCallMethod::Checkarity() {
	//We execute the method associated to an object...
	return Arity(globalTamgu->allmethods[name], arguments.size());
}

bool TamguCallCommonMethod::Checkarity() {
	return Arity(globalTamgu->arities[name], arguments.size());
}

bool TamguCallProcedure::Checkarity() {
	return Arity(globalTamgu->arities[name], arguments.size());
}

Exporting bool TamguCallFunction::Checkarity() {
	if (body == NULL || body->isCallVariable())
		return true;

	TamguFunction* bd = NULL;
	TamguFunction* b = (TamguFunction*)body;

    short btype, atype;
	while (b != NULL) {
		bool found = true;		
		if (b->parameters.size() == arguments.size()) {
			if (bd == NULL) {
				for (short i = 0; i < b->parameters.size(); i++) {
					btype = b->parameters[i]->Typevariable();
					atype = arguments[i]->Typeinfered();
					if (btype != atype && !globalTamgu->Compatible(btype, atype)) {
						found = false;
						break;
					}
				}

				if (found)
					bd = b;
			}			
			b = b->next;
			continue;
		}

		//We need to check if we have at least one section with pre-initialized values...
		//The size of arguments should then be inferior to the one of parameters
		if (arguments.size() < b->parameters.size()) {
			//We have one, we need to test if there is a default initialisation
			short nb = 0;			
			for (short i = 0; i < b->parameters.size(); i++) {
				if (i < arguments.size()) {
					btype = b->parameters[i]->Typevariable();
					atype = arguments[i]->Typeinfered();
					if (btype != atype && !globalTamgu->Compatible(btype, atype)) {
						found = false;
						break;
					}
				}

				if (b->parameters[i]->Initialisation() != NULL)
					nb++;
			}

			if (found) {
				while (nb) {
					//we have nb default initialization...
					if ((b->parameters.size() - nb) == arguments.size()) {
						//we know we have one in case of failure...
						if (bd == NULL)
							bd = b;
						nonlimited = true;
						break;
					}
					nb--;
				}
			}
		}
		b = b->next;
	}

	if (bd != NULL) {
		if (((TamguFunction*)body)->next != NULL)
			bd->strict = true;

		body = bd;
		return true;
	}
	return false;
}

bool TamguCallThread::Checkarity() {
	TamguFunction* b = (TamguFunction*)body;
	while (b != NULL) {
		if (b->parameters.size() == arguments.size())
			return true;

		short nb = 0;
		for (short i = 0; i < b->parameters.size(); i++) {
			if (b->parameters[i]->Initialisation() != NULL)
				nb++;
		}
		while (nb) {
			if ((b->parameters.size() - nb) == arguments.size()) {
				nonlimited = true;
				return true;
			}
			nb--;
		}

		b = b->next;
	}
	return false;
}

bool TamguCallFrameFunction::Checkarity() {
	if (frame == NULL)
		return Arity(globalTamgu->framemethods[name], arguments.size());

	Tamgu* b = frame->Declaration(name);
	if (b == NULL || !b->isFunction())
		return false;

	while (b != NULL) {
		if (b->Size() == arguments.size())
			return true;
		b = b->Nextfunction();
	}
	return false;
}

TamguCallTamguVariable::TamguCallTamguVariable(short n, Tamgutamgu* f, TamguGlobal* g, Tamgu* parent) : 
	aa(f),TamguCallVariable(n, f->Typevariable(), g, parent) {}

Tamgu* TamguCallTamguVariable::Eval(Tamgu* context, Tamgu* object, short idthread) {
    if (call->Name() ==  a_methods)
        return aa->MethodMethods(context, idthread, (TamguCall*)call);

	return aa->MethodMethod(context, idthread, (TamguCall*)call);
}

Exporting Tamgu* TamguCallMethod::Put(Tamgu* context, Tamgu* object, short idthread) {
	//We execute the method associated to an object...
	TamguCallMethod* call = this;
	if (idthread && call->arguments.size()) {
		//In the case of thread, we need to duplicate this structure
		//in order to get a proper cleaning variable...
		call = new TamguCallMethod(name);
		call->arguments = arguments;
	}

	if (!object->Type())
		object->Setidtype(globalTamgu);

	object = object->CallMethod(name, context, idthread, call);

	if (function != NULL) {
		if (object->isError()) {
			call->Cleaning(idthread);
			if (call != this)
				delete call;
			return object;
		}

		//This is a function which is called from an assignment, in this case, we need to catch
		//the index before computing it...
		if (function->isStop())
			return object;

		context = function->Eval(context, object, idthread);
		if (context != object) {
			if (object->isProtected()) {
				context->Setreference();
				object->Releasenonconst();
				context->Protect();
			}
			object = context;			
		}
	}


	call->Releasing(object);
	if (call != this)
		delete call;

	return object;
}

Exporting Tamgu* TamguCallMethod::Eval(Tamgu* context, Tamgu* object, short idthread) {
	//We execute the method associated to an object...
    
	TamguCallMethod* call = this;
	if (idthread && call->arguments.size()) {
		//In the case of thread, we need to duplicate this structure
		//in order to get a proper cleaning variable...
		call = new TamguCallMethod(name);
		call->arguments = arguments;
	}

	if (!object->Type())
		object->Setidtype(globalTamgu);

	object = object->CallMethod(name, context, idthread, call);

	if (function != NULL) {
		if (object->isError()) {
			call->Cleaning(idthread);
			if (call != this)
				delete call;
			return object;
		}

		context = function->Eval(context, object, idthread);
		if (context != object) {
			if (object->isProtected()) {
				context->Setreference();
				object->Releasenonconst();
				context->Protect();
			}
			object = context;
		}
	}

	call->Releasing(object);
	if (call != this)
		delete call;

	return object;
}

//This is either a call from another method or from a container object...
Exporting Tamgu* TamguCallFromCall::Put(Tamgu* context, Tamgu* object, short idthread) {
        //We execute the method associated to an object...
    short t = object->Type();
	if (!t) {
		object->Setidtype(globalTamgu);
		t = object->Type();
	}

	if (!globalTamgu->checkarity(t, name, arguments.size())) {
        if (!globalTamgu->frames.check(t)) {
            string mess("'");
            mess += globalTamgu->Getsymbol(name);
            mess += "' : Unknown method or Wrong number of arguments for type: '";
            mess += object->Typestring();
            mess += "'";
            return globalTamgu->Returnerror(mess, idthread);
        }
    }
    
    TamguCallMethod* call = this;
    if (idthread && call->arguments.size()) {
            //In the case of thread, we need to duplicate this structure
            //in order to get a proper cleaning variable...
        call = new TamguCallMethod(name);
        call->arguments = arguments;
    }
    
    object = object->CallMethod(name, context, idthread, call);
    
    if (function != NULL) {
        if (object->isError()) {
            call->Cleaning(idthread);
            if (call != this)
                delete call;
            return object;
        }
        
            //This is a function which is called from an assignment, in this case, we need to catch
            //the index before computing it...
        if (function->isStop())
            return object;
        
        context = function->Eval(context, object, idthread);
        if (context != object) {
            if (object->isProtected()) {
                context->Setreference();
                object->Releasenonconst();
                context->Protect();
            }
            object = context;
        }
    }
    
    
    call->Releasing(object);
    if (call != this)
        delete call;
    
    return object;
}

Exporting Tamgu* TamguCallFromCall::Eval(Tamgu* context, Tamgu* object, short idthread) {
        //We execute the method associated to an object...
    short t = object->Type();    
	if (!t) {
		object->Setidtype(globalTamgu);
		t = object->Type();
	}
    if (!globalTamgu->checkarity(t, name, arguments.size())) {
        if (!globalTamgu->frames.check(t)) {
            string mess("'");
            mess += globalTamgu->Getsymbol(name);
            mess += "' : Unknown method or Wrong number of arguments for type: '";
            mess += object->Typestring();
            mess += "'";
            return globalTamgu->Returnerror(mess, idthread);
        }
    }
    
    TamguCallMethod* call = this;
    if (idthread && call->arguments.size()) {
            //In the case of thread, we need to duplicate this structure
            //in order to get a proper cleaning variable...
        call = new TamguCallMethod(name);
        call->arguments = arguments;
    }

    object = object->CallMethod(name, context, idthread, call);
    
    if (function != NULL) {
        if (object->isError()) {
            call->Cleaning(idthread);
            if (call != this)
                delete call;
            return object;
        }
        
        context = function->Eval(context, object, idthread);
        if (context != object) {
            if (object->isProtected()) {
                context->Setreference();
                object->Releasenonconst();
                context->Protect();
            }
            object = context;
        }
    }
    
    call->Releasing(object);
    if (call != this)
        delete call;
    
    return object;
}

Tamgu* TamguCallCommonMethod::Eval(Tamgu* context, Tamgu* object, short idthread) {
	TamguCallCommonMethod* call = this;

	if (idthread && call->arguments.size()) {
		call = new TamguCallCommonMethod(name);
		call->arguments = arguments;
	}

	object = globalTamgu->commons[name](object, idthread, call);

	if (function != NULL) {
		if (object->isError()) {
			call->Cleaning(idthread);
			if (call != this)
				delete call;
			return object;
		}

		context = function->Eval(context, object, idthread);
		if (context != object) {
			if (object->isProtected()) {
				context->Setreference();
				object->Releasenonconst();
				context->Protect();
			}
			object = context;
		}
	}

	call->Releasing(object);
	if (call != this)
		delete call;

	return object;
}

Tamgu* TamguCallFrameFunction::Put(Tamgu* context, Tamgu* object, short idthread) {
	object = object->CallMethod(name, context, idthread, this);

	if (function != NULL) {
		if (object->isError()) {
			return object;
		}

		//We catch the index before assignment...
		if (function->isStop())
			return object;

		Tamgu* a = function->Eval(context, object, idthread);
		if (a != object) {
			if (object->isProtected()) {
				a->Setreference();
				object->Releasenonconst();
				a->Protect();
			}
		}
		return a;
	}
	return object;
}


Tamgu* TamguCallFrameFunction::Eval(Tamgu* context, Tamgu* object, short idthread) {
	object = object->CallMethod(name, context, idthread, this);

	if (function != NULL) {
		if (object->isError()) {
			return object;
		}

		Tamgu* a = function->Eval(context, object, idthread);
		if (a != object) {
			if (object->isProtected()) {
				a->Setreference();
				object->Releasenonconst();
				a->Protect();
			}
		}
		return a;
	}
	return object;
}

Tamgu* TamguCallTopFrameFunction::Put(Tamgu* context, Tamgu* object, short idthread) {
	object = object->CallTopMethod(name, frame, context, idthread, this);

	if (function != NULL) {
		if (object->isError()) {
			return object;
		}

		if (function->isStop())
			return object;

		Tamgu* a = function->Eval(context, object, idthread);
		if (a != object) {
			if (object->isProtected()) {
				a->Setreference();
				object->Releasenonconst();
				a->Protect();
			}
		}
		return a;
	}
	return object;
}


Tamgu* TamguCallTopFrameFunction::Eval(Tamgu* context, Tamgu* object, short idthread) {
	object = object->CallTopMethod(name, frame, context, idthread, this);

	if (function != NULL) {
		if (object->isError()) {
			return object;
		}

		Tamgu* a = function->Eval(context, object, idthread);
		if (a != object) {
			if (object->isProtected()) {
				a->Setreference();
				object->Releasenonconst();
				a->Protect();
			}
		}
		return a;
	}
	return object;
}


Tamgu* TamguCallProcedure::Eval(Tamgu* context, Tamgu* object, short idthread) {
	TamguCallProcedure* call = this;

	if (idthread && call->arguments.size()) {
		call = new TamguCallProcedure(name);
		call->arguments = arguments;
	}

	object = (*globalTamgu->procedures.get(name))(context, idthread, call);

	if (function != NULL) {
		if (object->isError()) {
			call->Cleaning(idthread);
			if (call != this)
				delete call;
			return object;
		}

		Tamgu* a = function->Eval(context, object, idthread);
		if (a != object) {
			if (object->isProtected()) {
				a->Setreference();
				object->Releasenonconst();
				a->Protect();
			}
			object = a;
		}
	}

	call->Releasing(object);
	if (call != this)
		delete call;

	return object;
}

//------------------------------------------------------------------------------------

Exporting Tamgu* TamguCallFunction::Eval(Tamgu* domain, Tamgu* a, short idthread) {
    static vector<Tamgu*> bvalues;
    
    TamguFunction* bd = (TamguFunction*)body->Body(idthread);

    if (curryfied) {
        TamguGetCurryfiedFunction* func = new TamguGetCurryfiedFunction(bd);
        for (long i = 0; i < arguments.size(); i++) {
            a = arguments[i]->Eval(aNULL, aHASKELL, idthread);
            if (a->isError()) {
                func->Release();
                return a;
            }
            func->arguments.push_back(a);
        }
        return func;
    }
    
	TamguDeclarationLocal* environment = globalTamgu->Providedeclaration(this, idthread, false);
    
	Tamgu* p;

	short i, sz = 0;
	bool strict = false;
    if (bd != NULL)
        strict = bd->strict;

    vector<Tamgu*>* values = &bvalues;
    if (idthread)
        values = new vector<Tamgu*>;
    
    for (i=0; i < arguments.size(); i++) {
        a = arguments[i]->Eval(domain, aNULL, idthread);
        values->push_back(a);
    }
    
	bool error = true;
    bool release;
	while (bd != NULL) {
		if (arguments.size() != bd->Size()) {
			if (!nonlimited) {
				bd = bd->next;
				continue;
			}
		}
		sz = bd->parameters.size();
		error = false;
		if (arguments.size() || nonlimited) {
			for (i = 0; i < sz; i++) {
				p = bd->parameters[i];
                release=false;
				if (!nonlimited || i < arguments.size())
                    a = values->at(i);
				else {
					a = p->Initialisation();
					if (a == NULL) {
						error = true;
						break;
					}
                    release=true;
				}

				if (!p->Setvalue(environment, a, idthread, strict)) {
                    if (release)
                        a->Releasenonconst();
					error = true;
					break;
				}
                if (release)
                    a->Releasenonconst();
			}

			if (!error)
				break;

			environment->Cleaning();

			bd = bd->next;
		}
		else
			break;
	}

    for (i = 0; i < values->size(); i++)
        values->at(i)->Releasenonconst();
    
    if (idthread)
        delete values;
    else
        values->clear();
    
	if (error) {
        environment->Release();
		string err = "Check the arguments of: ";
		err += globalTamgu->Getsymbol(Name());
		return globalTamgu->Returnerror(err, idthread);
	}

	globalTamgu->Pushstack(environment, idthread);
	//We then apply our function within this environment
	a = bd->Eval(environment, aNULL, idthread);
	globalTamgu->Popstack(idthread);

    //if a has no reference, then it means that it was recorded into the environment
	if (!a->Reference())
        environment->Release();
	else {
		a->Setreference();
		//we clean our structure...
        environment->Release();
		a->Protect();
	}

	return a;
}

//No parameter
Tamgu* TamguCallFunction0::Eval(Tamgu* domain, Tamgu* a, short idthread) {
    TamguDeclarationLocal* environment = globalTamgu->Providedeclaration(this, idthread, false);
    
    globalTamgu->Pushstack(environment, idthread);
    //We then apply our function within this environment
    a = ((TamguFunction*)body)->Eval(environment, aNULL, idthread);
    globalTamgu->Popstack(idthread);
    
    //if a has no reference, then it means that it was recorded into the environment
    
    if (!a->Reference())
        environment->Release();
    else {
        a->Setreference();
        //we clean our structure...
        environment->Release();
        a->Protect();
    }
    

    return a;
}

Tamgu* TamguCallFunction1::Eval(Tamgu* domain, Tamgu* a, short idthread) {
    if (nonlimited)
        return TamguCallFunction::Eval(domain, a, idthread);

    TamguDeclarationLocal* environment = globalTamgu->Providedeclaration(this, idthread, false);
    
    TamguFunction* bd = (TamguFunction*)body;
    
    bool strict = false;
    if (bd != NULL)
        strict = bd->strict;
    
    a = arguments[0]->Eval(domain, aNULL, idthread);
    
    if (!bd->parameters[0]->Setvalue(environment, a, idthread, strict)) {
        a->Releasenonconst();
        string err = "Check the arguments of: ";
        err += globalTamgu->Getsymbol(Name());
        return globalTamgu->Returnerror(err, idthread);
    }
    
    globalTamgu->Pushstack(environment, idthread);
    //We then apply our function within this environment
    a = bd->Eval(environment, aNULL, idthread);
    globalTamgu->Popstack(idthread);
    
    //if a has no reference, then it means that it was recorded into the environment
    if (!a->Reference())
        environment->Release();
    else {
        a->Setreference();
        //we clean our structure...
        environment->Release();
        a->Protect();
    }
    
    
    return a;
}

Tamgu* TamguCallFunction2::Eval(Tamgu* domain, Tamgu* a, short idthread) {
    if (nonlimited)
        return TamguCallFunction::Eval(domain, a, idthread);
    
    TamguDeclarationLocal* environment = globalTamgu->Providedeclaration(this, idthread, false);
    
    TamguFunction* bd = (TamguFunction*)body;
    
    bool strict = false;
    if (bd != NULL)
        strict = bd->strict;
    
    a = arguments[0]->Eval(domain, aNULL, idthread);
    if (!bd->parameters[0]->Setvalue(environment, a, idthread, strict)) {
        environment->Release();
        a->Releasenonconst();
        string err = "Check the arguments of: ";
        err += globalTamgu->Getsymbol(Name());
        return globalTamgu->Returnerror(err, idthread);
    }
    a = arguments[1]->Eval(domain, aNULL, idthread);
    if (!bd->parameters[1]->Setvalue(environment, a, idthread, strict)) {
        environment->Release();
        a->Releasenonconst();
        string err = "Check the arguments of: ";
        err += globalTamgu->Getsymbol(Name());
        return globalTamgu->Returnerror(err, idthread);
    }
    
    globalTamgu->Pushstack(environment, idthread);
    //We then apply our function within this environment
    a = bd->Eval(environment, aNULL, idthread);
    globalTamgu->Popstack(idthread);
    
    //if a has no reference, then it means that it was recorded into the environment
    if (!a->Reference())
        environment->Release();
    else {
        a->Setreference();
        //we clean our structure...
        environment->Release();
        a->Protect();
    }
    
    
    return a;
}

Tamgu* TamguCallFunction3::Eval(Tamgu* domain, Tamgu* a, short idthread) {
    if (nonlimited)
        return TamguCallFunction::Eval(domain, a, idthread);

    TamguDeclarationLocal* environment = globalTamgu->Providedeclaration(this, idthread, false);

    TamguFunction* bd = (TamguFunction*)body;
    
    bool strict = false;
    if (bd != NULL)
        strict = bd->strict;

    
    for (short i = 0; i < 3; i++) {
        a = arguments[i]->Eval(domain, aNULL, idthread);
        if (!bd->parameters[i]->Setvalue(environment, a, idthread, strict)) {
            environment->Release();
            a->Releasenonconst();
            string err = "Check the arguments of: ";
            err += globalTamgu->Getsymbol(Name());
            return globalTamgu->Returnerror(err, idthread);
        }
    }
    
    globalTamgu->Pushstack(environment, idthread);
    //We then apply our function within this environment
    a = bd->Eval(environment, aNULL, idthread);
    globalTamgu->Popstack(idthread);
    
    //if a has no reference, then it means that it was recorded into the environment
    if (!a->Reference())
        environment->Release();
    else {
        a->Setreference();
        //we clean our structure...
        environment->Release();
        a->Protect();
    }
    
    return a;
}

Tamgu* TamguCallFunction4::Eval(Tamgu* domain, Tamgu* a, short idthread) {
    if (nonlimited)
        return TamguCallFunction::Eval(domain, a, idthread);

    TamguDeclarationLocal* environment = globalTamgu->Providedeclaration(this, idthread, false);
    
    TamguFunction* bd = (TamguFunction*)body;
    
    bool strict = false;
    if (bd != NULL)
        strict = bd->strict;
    
    for (short i = 0; i < 4; i++) {
        a = arguments[i]->Eval(domain, aNULL, idthread);
        if (!bd->parameters[i]->Setvalue(environment, a, idthread, strict)) {
            environment->Release();
            a->Releasenonconst();
            string err = "Check the arguments of: ";
            err += globalTamgu->Getsymbol(Name());
            return globalTamgu->Returnerror(err, idthread);
        }
    }

    
    globalTamgu->Pushstack(environment, idthread);
    //We then apply our function within this environment
    a = bd->Eval(environment, aNULL, idthread);
    globalTamgu->Popstack(idthread);
    
    //if a has no reference, then it means that it was recorded into the environment
    if (!a->Reference())
        environment->Release();
    else {
        a->Setreference();
        //we clean our structure...
        environment->Release();
        a->Protect();
    }
    
    return a;
}

Tamgu* TamguCallFunction5::Eval(Tamgu* domain, Tamgu* a, short idthread) {
    if (nonlimited)
        return TamguCallFunction::Eval(domain, a, idthread);

    TamguDeclarationLocal* environment = globalTamgu->Providedeclaration(this, idthread, false);
    
    TamguFunction* bd = (TamguFunction*)body;
    
    bool strict = false;
    if (bd != NULL)
        strict = bd->strict;
    
    for (short i = 0; i < 5; i++) {
        a = arguments[i]->Eval(domain, aNULL, idthread);
        if (!bd->parameters[i]->Setvalue(environment, a, idthread, strict)) {
            environment->Release();
            a->Releasenonconst();
            string err = "Check the arguments of: ";
            err += globalTamgu->Getsymbol(Name());
            return globalTamgu->Returnerror(err, idthread);
        }
    }

    
    globalTamgu->Pushstack(environment, idthread);
    //We then apply our function within this environment
    a = bd->Eval(environment, aNULL, idthread);
    globalTamgu->Popstack(idthread);
    
    //if a has no reference, then it means that it was recorded into the environment
    if (!a->Reference())
        environment->Release();
    else {
        a->Setreference();
        //we clean our structure...
        environment->Release();
        a->Protect();
    }
    
    return a;
}
//------------------------------------------------------------------------------------
//This is a trick which allows for a variable creation...
Tamgu* TamguCallVariable::Put(Tamgu* domain, Tamgu* value, short idthread) {
    if (domain->isDeclared(name))
        return aTRUE;
    
    value = globalTamgu->newInstance.get(typevariable)->Newinstance(idthread);
    domain->Declare(name, value);
    globalTamgu->Storevariable(idthread, name, value);
    value->Setreference(1);
    return aTRUE;
}

Tamgu* TamguCallFrameVariable::Put(Tamgu* context, Tamgu* value, short idthread) {
    switch (nocall) {
        case 1:
            return globalTamgu->Getframedefinition(frame_name, name, idthread);
        case 10:
            if (call == NULL) {
                nocall = 1;
                return globalTamgu->Getframedefinition(frame_name, name, idthread);
            }
            nocall=0;
    }
    
    value = globalTamgu->Getframedefinition(frame_name, name, idthread);
    
    if (call->isStop())
        return value;
    return call->Put(context, value, idthread);
}

Tamgu* TamguCallFrameVariable::Eval(Tamgu* context, Tamgu* value, short idthread) {
    switch (nocall) {
        case 1:
            return globalTamgu->Getframedefinition(frame_name, name, idthread);
        case 2:
            return globalTamgu->Getframedefinition(frame_name, name, idthread)->Declaration(call_name);
        case 3:
            value = globalTamgu->Getdeclaration(frame_name, idthread);
            if (affectation && call->isStop())
                return value;
            return call->Eval(context, value, idthread);
        case 4:
            return globalTamgu->Getdeclaration(frame_name, idthread);
        case 10:
            if (call == NULL) {
                if (frame_name ==  name) {
                    nocall = 4;
                    return globalTamgu->Getdeclaration(frame_name, idthread);
                }
                nocall = 1;
                return globalTamgu->Getframedefinition(frame_name, name, idthread);
            }
            else
                if (call->Function() == NULL && call->isCallVariable()) {
                    nocall = 2;call_name = call->Name();
                    return globalTamgu->Getframedefinition(frame_name, name, idthread)->Declaration(call_name);
                }
            else
                if (frame_name ==  name) {
                    nocall = 3;
                    value = globalTamgu->Getdeclaration(frame_name, idthread);
                    if (affectation && call->isStop())
                        return value;
                    return call->Eval(context, value, idthread);
                }
            nocall=0;
    }

    value = globalTamgu->Getframedefinition(frame_name, name, idthread);
    
    if (affectation && call->isStop())
        return value;
    return call->Eval(context, value, idthread);
}

Tamgu* TamguCallFromFrameVariable::Eval(Tamgu* context, Tamgu* value, short idthread) {
    //In this case, the value is the calling variable...
    if (call == NULL)
        return ((TamguframeBaseInstance*)value)->Getvariable(name);
    
    value = ((TamguframeBaseInstance*)value)->Getvariable(name);
    
    if (affectation && call->isStop())
        return value;
    return call->Eval(context, value, idthread);
}

Tamgu* TamguCallFromFrameVariable::Put(Tamgu* context, Tamgu* value, short idthread) {
    //In this case, the value is the calling variable...
    if (call == NULL)
        return ((TamguframeBaseInstance*)value)->Getvariable(name);
    
    value = ((TamguframeBaseInstance*)value)->Getvariable(name);

    if (call->isStop())
        return value;
    return call->Put(context, value, idthread);
}


Tamgu* TamguCallThroughVariable::Eval(Tamgu* context, Tamgu* value, short idthread) {
    if (call == NULL)
        return throughvariables[sname];

    value = throughvariables[sname];
    if (affectation) {
        if (call->isStop())
            return value;
        return call->Put(context, value, idthread);
    }
    return call->Eval(context, value, idthread);
}

Tamgu* TamguCallVariable::Eval(Tamgu* context, Tamgu* value, short idthread) {
    if (directcall)
        return globalTamgu->Getdeclaration(name, idthread);

    if (call == NULL) {
        if (!forced) {
            directcall = true;
            return globalTamgu->Getdeclaration(name, idthread);
        }

        value = globalTamgu->Getdeclaration(name, idthread);
        value->Forcedclean();
        return value;
    }
    
    value = globalTamgu->Getdeclaration(name, idthread);
    
    if (affectation) {
        if (call->isStop())
            return value;
        return call->Put(context, value, idthread);
    }
    return call->Eval(context, value, idthread);
}

Tamgu* TamguCallGlobalVariable::Eval(Tamgu* context, Tamgu* v, short idthread) {
    if (!first)
        return value;
    
    if (first == true) {
        value = globalTamgu->Getmaindeclaration(name, idthread);
        
        if (value == NULL) {
            value = aNOELEMENT;
            return value;
        }

        first = false;
    }

    if (call != NULL) {
        first = 2;
        if (affectation) {
            if (call->isStop())
                return value;
            return call->Put(context, value, idthread);
        }
        return call->Eval(context, value, idthread);
    }
    
    if (forced) {
        first = 3;
        value->Forcedclean();
    }
    
    return value;
}

Tamgu* TamguCallConstantVariable::Eval(Tamgu* context, Tamgu* value, short idthread) {
	value = globalTamgu->Getdeclaration(name, idthread);

	if (call != NULL)
		return call->Eval(context, value, idthread);

	value->SetConst();
	return value;
}

Tamgu* TamguCallFunctionVariable::Eval(Tamgu* context, Tamgu* value, short idthread) {
    if (call == NULL) {
        directcall = true;
        return globalTamgu->Getdeclaration(name, idthread);
    }
    
    value = globalTamgu->Getdeclaration(name, idthread);
    
    if (affectation) {
        if (call->isStop())
            return value;
        return call->Put(context, value, idthread);
    }
    return call->Eval(context, value, idthread);
}

//------------------------------------------------SELF SECTION-------------------------------------

Tamgu* TamguSelfVariableDeclaration::Eval(Tamgu* domain, Tamgu* value, short idthread) {
	TamguLet* a;
	if (typevariable == a_self)
		a = globalTamgu->Provideself();
	else
		a = new TamguLet;

	domain->Declare(name, a);
	globalTamgu->Storevariable(idthread, name, a);
	a->Setreference();
	if (initialization != NULL) {
		value = initialization->Eval(aNULL, aNULL, idthread);
		if (value->isError())
			return value;
		a->value = value;
		value->Setreference();
        a->Postinstantiation(idthread, true);
	}

	return a;
}

Tamgu* TamguSelfVariableDeclaration::Put(Tamgu* domain, Tamgu* value, short idthread) {
    if (typevariable == a_self)
        value = globalTamgu->Provideself();
    else
        value = new TamguLet;
    
    domain->Declare(name, value);
    globalTamgu->Storevariable(idthread, name, value);
    value->Setreference();
    return value;
}

Tamgu* TamguCallSelfVariable::Eval(Tamgu* context, Tamgu* value, short idthread) {
    if (directcall)
        return globalTamgu->Getdeclaration(name, idthread);
    
	value = globalTamgu->Getdeclaration(name, idthread);

	if (call != NULL) {
		value = value->Value();

		if (value == aNOELEMENT)
			return globalTamgu->Returnerror("Uninitialized 'self' variable", idthread);

		if (affectation && call->isStop())
			return value;
		return call->Eval(context, value, idthread);
	}

	if (affectation) {
		if (forced)
			value->Forcedclean();
        else
            directcall = true;
		return value;
	}

	return value->Value();
}

Tamgu* TamguCallSelfVariable::Put(Tamgu* domain, Tamgu* value, short idthread) {
	if (domain->isDeclared(name))
		return aTRUE;

	if (typevariable == a_self)
		value = globalTamgu->Provideself();
	else
		value = new TamguLet;

	domain->Declare(name, value);
	globalTamgu->Storevariable(idthread, name, value);
	value->Setreference();
	return aTRUE;
}

//------------------------------------------------------------------------------------
Tamgu* TamguInstruction::Eval(Tamgu* context, Tamgu* a, short idthread) {
	if (instructions.last <= 1) {
        if (!instructions.last)
            return aNULL;

        a = instructions.vecteur[0];
        _setdebugfull(idthread,a);
		a = a->Eval(context, aNULL, idthread);
		_cleandebugfull;
		return a;
	}

    _setdebugmin(idthread);

	if (variablesWillBeCreated)
		context = globalTamgu->Providedeclaration(this, idthread, true);

    bool testcond = false;
    a = aNULL;
    
	for (long i = 0; i < instructions.last && !testcond; i++) {
        a->Releasenonconst();

		a = instructions.vecteur[i];
		
		_debugpush(a);
		a = a->Eval(context, aNULL, idthread);
		_debugpop();

        testcond = globalTamgu->Error(idthread) || a->needFullInvestigate();
    }
    
    if (testcond) {
        if (globalTamgu->Error(idthread)) {
            _cleandebugmin;
            if (!a->isError())
                a->Releasenonconst();
            return globalTamgu->Errorobject(idthread);
        }

        if (!variablesWillBeCreated)
            return a;
        
        if (a->isReturned()) {
            if (context->isEmpty()) {
                context->Release();
                return a;
            }
            
            Tamgu* r = a->Returned(idthread);
            if (!r->Reference())
                context->Release();
            else {
                r->Setreference();
                context->Release();
                r->Protect();
            }
            return a;
        }
        
        context->Release();
        return a;
    }

    a->Releasenonconst();

	_cleandebugmin;
	if (variablesWillBeCreated)
		context->Release();

	return aNULL;
}

Tamgu* TamguFunction::Eval(Tamgu* environment, Tamgu* a, short idthread) {
	long size = instructions.size();

	_setdebugfull(idthread, this);

	a = aNULL;
    bool testcond = false;

	for (long i = 0; i < size && !testcond; i++) {

        a->Releasenonconst();

        a = instructions.vecteur[i];
        
		_debugpush(a);
		a = a->Eval(environment, aNULL, idthread);
		_debugpop();

        testcond = globalTamgu->Error(idthread) || a->needFullInvestigate();
    }
    
    if (testcond) {
        _cleandebugfull;
		if (globalTamgu->Error(idthread)) {
			if (!a->isError())
				a->Releasenonconst();
            return globalTamgu->Errorobject(idthread);
		}

        a = a->Returned(idthread);
        if (returntype) {
            if (a->Type() != returntype && !globalTamgu->Compatiblestrict(returntype, a->Type())) {
                a->Releasenonconst();
                stringstream msg;
                msg << "Mismatch between return value and function declaration. Expecting: '"
                << globalTamgu->Getsymbol(returntype)
                << "' Got: '" << globalTamgu->Getsymbol(a->Type()) << "'";
                return globalTamgu->Returnerror(msg.str(), idthread);
            }
        }
        return a;
	}

    a->Releasenonconst();

	_cleandebugfull;
	if (returntype && name != a_initial)
		return globalTamgu->Returnerror("This function is expected to return a value", idthread);

	return aNULL;
}
//____________________________________________________________________________________
#ifdef WIN32
void __stdcall AThread(TamguThreadCall* call) {
#else
void AThread(TamguThreadCall* call) {
#endif

    globalTamgu = call->global;
    
    short idparent = call->parentid;
	short idthread = call->idthread;
    bool waitonjoin = call->joined;

	if (globalTamgu->Error(idparent)) {
		for (short i = 0; i < call->arguments.size(); i++)
			call->arguments[i]->Resetreference();
		if (call->cleandom) {
			if (globalTamgu->Checktracker(call->domain, call->idomain))
				call->domain->Resetreference();
		}
		globalTamgu->EraseThreadid(idthread);
		delete call;
		return;
	}
	
	globalTamgu->InitThreadid(idthread);
    call->tid = globalTamgu->threads[idthread].handle;
    
    
	Locking* _exclusive = NULL;
	if (call->exclusive && call->cleandom)
		_exclusive = new Locking((TamguObject*)call->domain);
    
    //If the thread is executed from within a frame...
    if (call->cleandom)
        ((TamguframeBaseInstance*)call->domain)->Pushframe(idthread);
    
    //--------------------------------------------------------
    //We launch the code of our thread
    Tamgu* a = call->Eval(call->domain, aNULL, idthread);
   //Which might be returning a value..
   //--------------------------------------------------------

    if (call->cleandom)
        ((TamguframeBaseInstance*)call->domain)->Popframe(idthread);

	if (call->recipient != NULL) {//If we have a streamed variable
		call->recipient->Put(call->domain, a, idthread);
		call->recipient->Resetreference();
	}
	
    a->Releasenonconst();
	if (_exclusive != NULL)
		delete _exclusive;
    
    globalTamgu->Clearfibres(idthread);

	for (auto& it : globalTamgu->threads[idthread].locks)
		it.second->Unlocking();
	globalTamgu->threads[idthread].locks.clear();
	globalTamgu->threads[idthread].Clearknowledgebase();
	
    if (globalTamgu->Error(idthread)) {
        if (!globalTamgu->errors[idparent] && globalTamgu->errorraised[idthread] != NULL) {
            globalTamgu->errors[idparent] = true;
            globalTamgu->errorraised[idparent] = globalTamgu->errorraised[idthread];
            globalTamgu->threads[idparent].currentinstruction = globalTamgu->threads[idthread].currentinstruction;
        }
	}
	for (short i = 0; i < call->arguments.size(); i++)
		call->arguments[i]->Resetreference();
	if (call->cleandom) {
		if (globalTamgu->Checktracker(call->domain, call->idomain))
			call->domain->Resetreference();
	}	
	globalTamgu->EraseThreadid(idthread);
	delete call;

    if (waitonjoin)
        globalTamgu->threads[idparent].nbjoined--;

}

Tamgu* TamguThreadCall::Eval(Tamgu* domain, Tamgu* value, short idthread) {
    TamguDeclarationLocal* environment = globalTamgu->Providedeclaration(this, idthread, false);

	Tamgu* p;
	TamguFunction* bd = (TamguFunction*)body->Body(idthread);
	bool strict = bd->strict;
	if (!strict && bd->next)
		strict = true;

    Tamgu* a;

	bool error = true;
	while (bd != NULL) {
		if (arguments.size() != bd->Size()) {
			if (!nonlimited) {
				bd = bd->next;
				continue;
			}
		}

		error = false;
		for (short i = 0; i < bd->parameters.size(); i++) {
			p = bd->Parameter(i);

			if (!nonlimited || i < arguments.size())
				a = arguments[i];
			else {
				a = p->Initialisation();
				if (a == NULL) {
					error = true;
					break;
				}
			}

			if (!p->Setvalue(environment, a, idthread, strict)) {
				error = true;
				break;
			}
		}

		if (!error)
			break;

		environment->Cleaning();
		bd = bd->next;
	}

	if (error) {
        environment->Release();
		string err = "Check the arguments of: ";
		err += globalTamgu->Getsymbol(Name());
		return globalTamgu->Returnerror(err, idthread);
	}

	globalTamgu->Pushstack(environment, idthread);
	//We then apply our function within this environment
	a = bd->Eval(environment, aNULL, idthread);
	globalTamgu->Popstack(idthread);

	if (!a->Reference())
		environment->Release();
	else {
		a->Setreference();
		//we clean our structure...
        environment->Release();
		a->Protect();
	}
    
	globalTamgu->threads[idthread].variables.clear();

	return a;
}

Tamgu* TamguThread::Eval(Tamgu* environment, Tamgu* a, short idthread) {
	Locking* _lock = NULL;
	if (_locker != NULL)
		_lock = new Locking(*_locker);

	_setdebugfull(idthread, this);
	long size = instructions.size();
    a = aNULL;
    bool testcond = false;
    
	for (long i = 0; i < size && !testcond; i++) {
        a->Releasenonconst();

		a = instructions.vecteur[i];

		_debugpush(a);
		a = a->Eval(environment, aNULL, idthread);
		_debugpop();

        testcond = a->isReturned() || globalTamgu->Error(idthread);
    }
     
    if (testcond) {
		if (a->isReturned()) {
			if (_lock != NULL)
				delete _lock;
			_cleandebugfull;
			if (returntype) {
				if (a->Type() != returntype && !globalTamgu->Compatible(returntype, a->Type())) {
					a->Releasenonconst();
					stringstream msg;
					msg << "Mismatch between return value and thread declaration. Expecting: '"
						<< globalTamgu->Getsymbol(returntype)
						<< "' Got: '" << globalTamgu->Getsymbol(a->Type()) << "'";
					return globalTamgu->Returnerror(msg.str(), idthread);
				}
			}

			return a->Returned(idthread);
		}

		if (globalTamgu->Error(idthread)) {
			if (_lock != NULL)
				delete _lock;
			_cleandebugfull;
			return globalTamgu->Errorobject(idthread);
		}
	}

    a->Releasenonconst();

	if (_lock != NULL)
		delete _lock;
	_cleandebugfull;
	if (returntype)
		return globalTamgu->Returnerror("This function is expected to return a value", idthread);

	return aNULL;
}

Tamgu* TamguCallThread::Eval(Tamgu* environment, Tamgu* value, short idthread) {
	//We enter a thread... //Now Locks will be created...
	globalTamgu->Sethreading();

	short id = globalTamgu->SelectThreadid();
	if (id == -1)
		return globalTamgu->Returnerror("Too many threads", idthread);

	Tamgu* main = globalTamgu->Getmainframe(Currentspace());

	globalTamgu->Pushstack(main, id);
	short i;
	vector<short> vars;
	main->Variables(vars);
    for (i = 0; i < vars.size(); i++) {
        //this is a hack, which help maintain compatibility with the main thread, where global variables are stored at position 1
        globalTamgu->threads[id].variables[vars[i]].push_back(aNULL);
        value = main->Declaration(vars[i]);
        value->Enablelock(is_tobelocked);
		globalTamgu->threads[id].variables[vars[i]].push_back(value);
    }
    
	//Then we copy the current knowledge base into the new thread own knowledge base as they must share it...
	globalTamgu->threads[id].Setknowledgebase();

	bool cleandom = false;
	Tamgu* dom = main;
	if (environment != main) {
		if (environment->isFrameinstance()) {
			//We are in a frame...
			//We protect it and its content, which much be pretected within their
            //access within a thread...
            environment->Enablelock(is_tobelocked);
			environment->Setreference();
			cleandom = true;
			dom = environment;
		}
	}

	TamguThreadCall* threadcall = new TamguThreadCall(body, recipient, dom, cleandom, body->isExclusive(), id, idthread);
	threadcall->Getinfo(this);

    if (body->isJoined()) {
        globalTamgu->threads[idthread].nbjoined++;
        threadcall->joined = true;
    }

	//we need to evaluate our arguments beforehand
	Tamgu* a;
	for (i = 0; i < arguments.size(); i++) {
        a = arguments[i]->Eval(environment, aNULL, idthread)->GetvalueforThread();
        if (!a->isProtected())
            a->Enablelock(is_tobelocked);
		a->Setreference();
		threadcall->arguments.push_back(a);
	}
    
    threadcall->thid = new std::thread(AThread, threadcall);
    if (threadcall->thid == NULL) {
        globalTamgu->threads[id].Clear();
        delete threadcall;
        return globalTamgu->Returnerror("Could not create a thread", idthread);
    }
    
	return aNULL;
}

//____________________________________________________________________________________
Tamgu* TamguInstructionGlobalVariableAFFECTATION::Eval(Tamgu* context, Tamgu* value, short idthread) {
    if (first) {
        variable = globalTamgu->threads[idthread].variables.get(varname).back();
        first = false;
    }

    value = instructions.vecteur[1]->Eval(variable, aAFFECTATION, idthread);
    
    if (value != variable) {
        if (value->isError())
            return value;
        
        variable->Putatomicvalue(value);
        value->Releasenonconst();
    }
    
    if (globalTamgu->isthreading)
        globalTamgu->Triggeronfalse(variable);
    
    return aTRUE;
}

Tamgu* TamguInstructionFunctionVariableAFFECTATION::Eval(Tamgu* var, Tamgu* value, short idthread) {
    var = globalTamgu->Getdeclaration(name, idthread);
    
    value = instructions.vecteur[1]->Eval(var, aAFFECTATION, idthread);
    
    if (value !=  var) {
        if (value->isError())
            return value;
        
        var->Putatomicvalue(value);
        value->Releasenonconst();
    }
    
    if (globalTamgu->isthreading)
        globalTamgu->Triggeronfalse(var);
    
    return aTRUE;
}

Tamgu* TamguInstructionVariableAFFECTATION::Eval(Tamgu* var, Tamgu* value, short idthread) {
    var = instructions.vecteur[0]->Eval(var, aNULL, idthread);
    value = instructions.vecteur[1]->Eval(var, aAFFECTATION, idthread);
    
    if (value !=  var) {
        if (value->isError())
            return value;
        
        var->Putatomicvalue(value);
        value->Releasenonconst();
    }
    
    if (globalTamgu->isthreading)
        globalTamgu->Triggeronfalse(var);
    
    return aTRUE;
}
    
Tamgu* TamguInstructionAtomicAFFECTATION::Eval(Tamgu* environment, Tamgu* value, short idthread) {
    if (directcall == 1) {
        value = globalTamgu->Getdeclaration(name, idthread);
        value = instructions.vecteur[1]->Eval(value, aAFFECTATION, idthread);
        if (value->isError())
            return value;
        if (globalTamgu->isthreading)
            globalTamgu->Triggeronfalse(value);
        return aTRUE;
    }
    
    Tamgu* var = variable->Eval(environment, aNULL, idthread);
    if (var->isError() || var->isNULL())
        return var;
    
	bool aff = var->checkAffectation();

    value = instructions.vecteur[1]->Eval(var, aAFFECTATION, idthread);

    if (!aff)
        var->Setaffectation(false);

	if (value == var) {
		if (globalTamgu->isthreading)
			globalTamgu->Triggeronfalse(var);
        
        if (directcall)
            return aTRUE;
        
        if (variable->directCall() && instructions.vecteur[1]->BType() != 255)
            directcall = 1;
        else
            directcall = 2;
        return aTRUE;
	}

	if (value->isError())
		return value;
	
	if (value == aNOELEMENT) {
		var->Putvalue(aNOELEMENT, idthread);
		return value;
	}

	//Then we might have an issue, if value is a sub element of variable...
	//for instance: v=v[0];
	//we need to prevent a Clear() on "variable" to delete this element...
	value->Setprotect(true);

	environment = var->Put(environment, value, idthread);

	value->Resetreferencenoprotect(0);

	if (environment->isError())
		return environment;

	if (globalTamgu->isthreading)
		globalTamgu->Triggeronfalse(var);

	return aTRUE;
}

Tamgu* TamguInstructionAFFECTATION::Eval(Tamgu* environment, Tamgu* value, short idthread) {
	Tamgu* variable = instructions.vecteur[0]->Eval(environment, aNULL, idthread);
	if (variable->isError())
		return variable;

	value = instructions.vecteur[0]->Function();
	bool relvar = false;

	if (value != NULL) {
		Tamgu* idx = value->Getindex();
		
		if (idx != value) {//It could be a useless assignment
			if (idx != NULL && idx->isError()) {
                variable->Releasenonconst();
				return idx;
			}
			relvar = true;
		}

		if (idx != NULL) {
			value = instructions.vecteur[1]->Eval(environment, aNULL, idthread);

			//for instance: v[0]=v[0][1];
			//we need to prevent a Clear() on "variable" to delete this element...
			value->Setprotect(true);

			idx = idx->Put(variable, value, idthread);

			value->Resetreferencenoprotect(0);

			if (relvar)
                variable->Releasenonconst();
			
			if (idx->isError())
				return idx;

			return aTRUE;
		}
	}

	bool aff = variable->checkAffectation();

	value = instructions.vecteur[1]->Eval(variable, aAFFECTATION, idthread);

	if (value == variable) {
        if (!aff)
            variable->Setaffectation(false);
		if (globalTamgu->isthreading)
			globalTamgu->Triggeronfalse(variable);
		return aTRUE;
	}

	if (value->isError()) {
		if (relvar)
            variable->Releasenonconst();
		return value;
	}

	//Then we might have an issue, if value is a sub element of variable...
	//for instance: v=v[0];
	//we need to prevent a Clear() on "variable" to delete this element...
	value->Setprotect(true);

	environment = variable->Put(environment, value, idthread);

    if (!aff)
        variable->Setaffectation(false);
	
	value->Resetreferencenoprotect(0);

	if (relvar)
        variable->Releasenonconst();

	if (environment->isError())
		return environment;

	if (globalTamgu->isthreading)
		globalTamgu->Triggeronfalse(variable);
	return aTRUE;
}

Tamgu* TamguInstructionSTREAM::Eval(Tamgu* environment, Tamgu* value, short idthread) {
	Tamgu* idx = instructions.vecteur[0]->Function();
	Tamgu* variable = instructions.vecteur[0]->Eval(environment, aNULL, idthread);

    short thetype = instructions.vecteur[1]->Type();
	if (thetype == a_callthread) {
		if (idx != NULL && idx->isIndex()) {
			value = variable->Eval(environment, idx, idthread);
            if (value->isNULL()) {
				if (variable->isValueContainer()) {
					variable->Put(idx, aNULL, idthread);
					value = variable->Eval(environment, idx, idthread);
				}
				else {
					//We create a Self variable...
					value = new TamguContent(((TamguIndex*)idx)->Evaluate(idthread), variable);
				}
			}
		}
		else
			value = variable;

		instructions.vecteur[1]->Push(value);
		instructions.vecteur[1]->Eval(environment, value, idthread);
		return aTRUE;
	}

    if (thetype == a_fibre) {
        if (idx != NULL && idx->isIndex()) {
            value = variable->Eval(environment, idx, idthread);
            if (value->isNULL())
                value = new TamguContent(((TamguIndex*)idx)->Evaluate(idthread), variable);
        }
        else
            value = variable;

        char aff = value->checkAffectation();
        instructions.vecteur[1]->Eval(value, aNULL, idthread);
        value->Setaffectation(aff);
        value->Release();
        return aTRUE;
    }

    return TamguInstructionAFFECTATION::Eval(environment,value,idthread);
}

Tamgu* TamguInstructionSWITCH::Eval(Tamgu* context, Tamgu* ke, short idthread) {
	//First our variable
	Tamgu* var = instructions.vecteur[0]->Eval(context, aNULL, idthread);
	//First case, we have only discrete values...
	if (usekeys == 1) {
		//Then first, we get the string out of our variable...
		string s = var->String();
		var->Releasenonconst();
		if (keys.find(s) == keys.end()) {
			//we test if we have a default value...
			if (instructions.vecteur[instructions.last - 2] == aDEFAULT) {
				return instructions.back()->Eval(context, aNULL, idthread);
			}
			return aNULL;
		}
        return instructions.vecteur[keys[s] + 1]->Eval(aNULL, aNULL, idthread);
	}

	//Then the instructions, the odd element is the value to compare with, the even element
	//the instruction to execute.
	long maxid = instructions.size();
	Tamgu* result;
    if (usekeys == 2) {
        TamguCallFunction callfunc(call);
        callfunc.arguments.push_back(var);
        callfunc.arguments.push_back(aNULL);
        
        for (long i = 1; i < maxid; i += 2) {
            result = instructions.vecteur[i]->Eval(context, aNULL, idthread);
            ke = instructions.vecteur[i + 1];
            
            if (result == aDEFAULT) {
                ke = ke->Eval(context, aNULL, idthread);
                var->Releasenonconst();
                return ke;
            }

            callfunc.arguments.vecteur[1] = result;
            if (callfunc.Eval(aNULL, aNULL, idthread)->Boolean() == true) {
                result->Releasenonconst();
                ke = ke->Eval(context, aNULL, idthread);
                var->Releasenonconst();
                return ke;
            }
            
            if (result->isError()) {
                var->Releasenonconst();
                return result;
            }
            result->Releasenonconst();
        }
        var->Releasenonconst();
        return aNULL;
    }
    
	for (long i = 1; i < maxid; i += 2) {
		result = instructions.vecteur[i]->Eval(context, aNULL, idthread);
		ke = instructions.vecteur[i + 1];

        if (result == aDEFAULT) {
            ke = ke->Eval(context, aNULL, idthread);
            var->Releasenonconst();
            return ke;
        }

		if (var->same(result)->Boolean() == true) {
			result->Releasenonconst();
            ke = ke->Eval(context, aNULL, idthread);
            var->Releasenonconst();
            return ke;
        }

		if (result->isError()) {
			var->Releasenonconst();
			return result;
		}
		result->Releasenonconst();
	}
	var->Releasenonconst();
	return aNULL;
}

Tamgu* TamguInstructionIF::Eval(Tamgu* context, Tamgu* value, short idthread) {
    
    value = instructions.vecteur[0]->Eval(aTRUE, aNULL, idthread);
    
    if (value->isError())
        return value;
    
    uchar i = (value->Boolean() == negation) + 1;
    value->Releasenonconst();
    return instructions.vecteur[i]->Eval(context, aNULL, idthread);
}

Tamgu* TamguInstructionOperationIfnot::Eval(Tamgu* context, Tamgu* result, short idthread) {
    short maxid = instructions.size();
    for (short i = 0; i < maxid; i++) {
        result = instructions.vecteur[i]->Eval(aTRUE, aNULL, idthread);
        if (!result->isConst() || result == aTRUE)
            return result;
        result->Releasenonconst();
    }
    return aNOELEMENT;
}
    
    
Tamgu* TamguInstructionOR::Eval(Tamgu* context, Tamgu* result, short idthread) {
	short maxid = instructions.size();
	for (short i = 0; i < maxid; i++) {
		context = instructions.vecteur[i];
		result = context->Eval(aTRUE, aNULL, idthread);
		if (result->Boolean() != context->isNegation()) {
			result->Releasenonconst();
			return aTRUE;
		}
		result->Releasenonconst();
	}
	return aFALSE;
}

Tamgu* TamguInstructionXOR::Eval(Tamgu* context, Tamgu* result, short idthread) {
    short maxid = instructions.size();
    context = instructions.vecteur[0];
    result = context->Eval(aTRUE, aNULL, idthread);
    bool val = false;
    if (result->Boolean() != context->isNegation())
        val = true;
    
    for (short i = 1; i < maxid; i++) {
        context = instructions.vecteur[i];
        result = context->Eval(aTRUE, aNULL, idthread);
        if (result->Boolean() != context->isNegation()) {
            if (!val) {
                result->Releasenonconst();
                return aTRUE;
            }
            val = true;
        }
        else {
            if (val) {
                result->Releasenonconst();
                return aTRUE;
            }
            val = false;
        }
        result->Releasenonconst();
    }
    return aFALSE;
}
    
Tamgu* TamguInstructionAND::Eval(Tamgu* context, Tamgu* result, short idthread) {
	short maxid = instructions.size();
	for (short i = 0; i < maxid; i++) {
		context = instructions.vecteur[i];
		result = context->Eval(aTRUE, aNULL, idthread);
		if (result->Boolean() == context->isNegation()) {
			result->Releasenonconst();
			return aFALSE;
		}
		result->Releasenonconst();
	}
	return aTRUE;
}

Tamgu* TamguInstructionWHILE::Eval(Tamgu* context, Tamgu* result, short idthread) {
	Tamgu* ktest = instructions.vecteur[0];
	Tamgu* bloc = instructions.vecteur[1];

    result = aNULL;
    negation = ktest->isNegation();

	_setdebugfull(idthread, this);

    bool testcond = false;
	while (!testcond && ktest->Eval(aTRUE, aNULL, idthread)->Boolean() != negation) {
		result->Releasenonconst();

		_debugpush(bloc);
		result = bloc->Eval(context, aNULL, idthread);
		_debugpop();

        testcond = result->needInvestigate() || globalTamgu->Error(idthread);
    }
    
    _cleandebugfull;

    if (testcond) {
        if (result->isReturned())
            return result;
        
        if (result == aBREAK)
            return aTRUE;
        return globalTamgu->Errorobject(idthread);
    }

    result->Releasenonconst();
	return aTRUE;
}


Tamgu* TamguInstructionUNTIL::Eval(Tamgu* context, Tamgu* result, short idthread) {


    negation = instructions.vecteur[0]->isNegation();
    
	Tamgu* ktest = instructions.vecteur[0];
	Tamgu* bloc = instructions.vecteur[1];
    result = aNULL;
    bool testcond = false;
    
	_setdebugmin(idthread);

	do {
		result->Releasenonconst();

		_debugpush(bloc);
		result = bloc->Eval(context, aNULL, idthread);
		_debugpop();

        testcond = result->needInvestigate() || globalTamgu->Error(idthread);
	}
    while (!testcond && ktest->Eval(aTRUE, aNULL, idthread)->Boolean() != negation);

    _cleandebugmin;
    
    if (testcond) {
        if (result->isReturned())
            return result;

        if (result == aBREAK)
            return aTRUE;
        
        return globalTamgu->Errorobject(idthread);
    }

	result->Releasenonconst();
	return aTRUE;
}

Tamgu* TamguInstructionIN::Eval(Tamgu* context, Tamgu* value, short idthread) {
	value = instructions.vecteur[0]->Eval(context, aNULL, idthread);
	Tamgu* object = instructions.vecteur[1]->Eval(context, aNULL, idthread);
    if (value->isRegular())
        context = value->in(context, object, idthread);
    else
        context = object->in(context, value, idthread);
    
	value->Releasenonconst();
	if (action == a_notin) {
        return booleantamgu[!context->Boolean()];
	}

	return context;
}

    
Tamgu* Tamgu::Loopin(TamguInstruction* ins, Tamgu* context, short idthread) {
    TamguIteration* itr = Newiteration(false);
    if (itr == aITERNULL)
        return globalTamgu->Returnerror("Cannot loop on this element...", idthread);

    Tamgu* var = ins->instructions.vecteur[0]->Instruction(0);
    var = var->Eval(context, aNULL, idthread);
    
    Tamgu* a = aNULL;
    bool testcond = false;
    for (itr->Begin(); itr->End() != aTRUE && !testcond; itr->Next()) {
        a->Releasenonconst();
        var->Putvalue(itr->IteratorKey(), idthread);
        
        a = ins->instructions.vecteur[1]->Eval(context, aNULL, idthread);
        
            //Continue does not trigger needInvestigate
        testcond = a->needInvestigate();
    }

    itr->Release();
    if (testcond) {
        if (a == aBREAK)
            return this;
        return a;
    }
    
    a->Releasenonconst();
    
    return this;
}
    
    
Tamgu* TamguInstructionFORINVALUECONTAINER::Eval(Tamgu* context, Tamgu* loop, short idthread) {
	loop = instructions.vecteur[0]->Instruction(1)->Eval(context, aNULL, idthread);
	context = loop->Loopin(this, context, idthread);
	loop->Release();
	return context;
}

Tamgu* TamguInstructionFORINVECTOR::Eval(Tamgu* context, Tamgu* loop, short idthread) {
	loop = instructions.vecteur[0];
	Tamgu* var = loop->Instruction(0)->Eval(context, aNULL, idthread);
	if (var->isFrameinstance())
		return TamguInstructionFORIN::Eval(context, loop, idthread);

	loop = loop->Instruction(1)->Eval(context, aNULL, idthread);
	
	Tamgu* a = aNULL;
    bool testcond = false;
	Tamgu* v;

	for (long i = 0; i < loop->Size() && !testcond; i++) {
        a->Releasenonconst();
        
		v = loop->getvalue(i);
		var->Putvalue(v, idthread);

		a = instructions.vecteur[1]->Eval(context, aNULL, idthread);

        testcond = a->needInvestigate() || executionbreak;
    }

    loop->Release();

    if (testcond) {
		//Continue does not trigger needInvestigate
		if (a->needInvestigate()) {
			if (a == aBREAK)
                return this;
			return a;
		}
	}
    
    a->Releasenonconst();
	return this;
}

Tamgu* TamguInstructionFORIN::Eval(Tamgu* context, Tamgu* loop, short idthread) {

	loop = instructions.vecteur[0]->Instruction(0);
	Tamgu* var = loop->Eval(context, aNULL, idthread);

	short idname = 0;
	short typevar = var->Type();
	//In some cases, we cannot duplicate the value...
	Tamgu* dom = NULL;

	if (var->isFrameinstance()) {
		idname = loop->Name();
		dom = globalTamgu->Declarator(idname, idthread);
	}

	loop = instructions.vecteur[0]->Instruction(1)->Eval(context, aNULL, idthread);

	bool cleanup = true;
	if (loop->isValueContainer() || !var->isLetSelf())
		cleanup = false;

	Tamgu* v = aNULL;
	Tamgu* a = aNULL;
    bool testcond = false;
	if (loop->isVectorContainer()) {
		for (long i = 0; i < loop->Size() && !testcond; i++) {
            a->Releasenonconst();

            v = loop->getvalue(i);

			if (dom != NULL) {
				if (!globalTamgu->Compatible(v->Type(), typevar)) {
					dom->Declare(idname, var);
					loop->Release();
					return globalTamgu->Returnerror("Incompatible type in loop", idthread);
				}
				dom->Declare(idname, v);
			}
			else {
				if (cleanup)
					var->Forcedclean();
				var->Putvalue(v, idthread);
			}

			a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
        }
        
        if (!a->isError() && dom != NULL)
            dom->Declare(idname, var);

        loop->Release();

        if (testcond) {
            //Continue does not trigger needInvestigate
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return this;
                return a;
            }
        }
    
        a->Releasenonconst();
		return this;
	}

    //--------------------------------------------- Non Vector Iteration ----
    
    TamguIteration* it = loop->Newiteration(false);
    if (it == aITERNULL) {
        if (globalTamgu->erroronkey)
            return globalTamgu->Returnerror("Cannot loop on a this value", idthread);
        return this;
    }

	bool getval = true;
	if (loop->isMapContainer())
		getval = false;

	for (it->Begin(); !testcond && it->End() != aTRUE; it->Next()) {
        a->Releasenonconst();

        v = getval ? it->IteratorValue() : v = it->IteratorKey();

		if (dom != NULL) {
			if (!globalTamgu->Compatible(v->Type(), typevar)) {
				dom->Declare(idname, var);
				delete it;
				loop->Release();
				return globalTamgu->Returnerror("Incompatible type in loop", idthread);
			}
			dom->Declare(idname, v);
		}
		else {
			if (cleanup)
				var->Forcedclean();
			var->Putvalue(v, idthread);
		}

		a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }

    if (!a->isError() && dom != NULL)
        dom->Declare(idname, var);
    
    delete it;
    loop->Release();

    if (testcond) {
		//Continue does not trigger needInvestigate
		if (a->needInvestigate()) {
			if (a == aBREAK)
                return this;
			return a;
		}
	}

    a->Releasenonconst();
	return this;
}

Tamgu* TamguInstructionFORVECTORIN::Eval(Tamgu* context, Tamgu* loop, short idthread) {

	Tamgu* var = instructions.vecteur[0]->Instruction(0)->Eval(context, aNULL, idthread);
	loop = instructions.vecteur[0]->Instruction(1)->Eval(context, aNULL, idthread);
	if (!loop->isVectorContainer())
		return globalTamgu->Returnerror("Can only loop on vectors here", idthread);

	TamguIteration* it = loop->Newiteration(false);

    Tamgu* a = aNULL;
    bool testcond = false;
    
	for (it->Begin(); it->End() != aTRUE && !testcond; it->Next()) {
        a->Releasenonconst();
		var->Setvalue(aNULL, it->IteratorValue(), idthread);

		a = instructions.vecteur[1]->Eval(context, aNULL, idthread);

        testcond = executionbreak || a->needInvestigate();
    }

    loop->Release();
    it->Release();

    if (testcond) {
		if (a->needInvestigate()) {
			if (a == aBREAK)
                return this;
			return a;
		}
    }
    
    a->Releasenonconst();
	return this;
}

Tamgu* TamguInstructionFORMAPIN::Eval(Tamgu* context, Tamgu* loop, short idthread) {
	Tamgu* var = instructions.vecteur[0]->Instruction(0)->Eval(context, aNULL, idthread);
	loop = instructions.vecteur[0]->Instruction(1)->Eval(context, aNULL, idthread);
	if (!loop->isMapContainer())
		return globalTamgu->Returnerror("Can only loop on a map here", idthread);

	TamguIteration* it = loop->Newiteration(false);

	Tamgu* a = aNULL;
    bool testcond = false;
	for (it->Begin(); it->End() != aTRUE && !testcond; it->Next()) {
        a->Releasenonconst();
        
		var->Setvalue(it, aNULL, idthread);

		a = instructions.vecteur[1]->Eval(context, aNULL, idthread);

        testcond = a->needInvestigate() || executionbreak;
    }
    
    loop->Release();
    it->Release();

    if (testcond) {
		if (a->needInvestigate()) {
			if (a == aBREAK)
				return this;
			return a;
		}
    }
    
    a->Releasenonconst();

    return aNULL;
}

Tamgu* TamguInstructionFILEIN::Eval(Tamgu* context, Tamgu* loop, short idthread) {
	Tamgu* var = instructions.vecteur[0]->Instruction(0)->Eval(context, aNULL, idthread);
	loop = instructions.vecteur[0]->Instruction(1)->Eval(context, aNULL, idthread);

	Tamgu* a = aNULL;
    bool testcond = false;

	while (!testcond && loop->Eval(var, aNULL, idthread)->Boolean()) {
        a->Releasenonconst();
		a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }
        
    if (testcond) {
		if (a->needInvestigate()) {
			if (a == aBREAK)
                return aNULL;
			return a;
		}
    }
    
    a->Releasenonconst();
	return aNULL;
}

Tamgu* TamguInstructionFOR::Eval(Tamgu* context, Tamgu* stop, short idthread) {
	Tamgu* a = instructions.vecteur[0]->Eval(context, aNULL, idthread);
	if (a->isError())
		return a;

    a = aNULL;
    bool testcond = false;
    for (stop = instructions.vecteur[1];
         !testcond && stop->Eval(context, aNULL, idthread)->Boolean() != stop->isNegation();
         instructions.vecteur[2]->Eval(context, aNULL, idthread)) {
		
        a->Releasenonconst();
        a = instructions.vecteur[3]->Eval(context, aNULL, idthread);

        testcond = a->needInvestigate() || executionbreak;
    }
    
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return this;
            return a;
        }
    }
    
    a->Releasenonconst();
	return aNULL;
}

Tamgu* TamguInstructionFORINRANGE::Eval(Tamgu* context, Tamgu* value, short idthread) {
	value = instructions.vecteur[0]->Instruction(0)->Eval(context, aNULL, idthread);

	switch (value->Type()) {
	case a_int:
		return ExecuteInteger((Tamguint*)value, context, idthread);
	case a_float:
		return ExecuteFloat((Tamgufloat*)value, context, idthread);
	case a_decimal:
		return ExecuteDecimal((Tamgudecimal*)value, context, idthread);
	case a_long:
		return ExecuteLong((Tamgulong*)value, context, idthread);
	case a_short:
		return ExecuteShort((Tamgushort*)value, context, idthread);
	}

	Tamguint v((long)0);
	return ExecuteInteger(&v, context, idthread);
}

Tamgu* TamguInstructionFORINRANGE::ExecuteInteger(Tamguint* value, Tamgu* context, short idthread) {
	long v, t, i;

	value->Setreference();

	v = instructions.vecteur[0]->Instruction(1)->Getinteger(idthread);
	t = instructions.vecteur[0]->Instruction(2)->Getinteger(idthread);
	i = instructions.vecteur[0]->Instruction(3)->Getinteger(idthread);

	Tamgu* a = aNULL;
    bool testcond = false;
	if (i < 0) {
		for (; v > t && !testcond; v += i) {
            a->Releasenonconst();
			value->value = v;
			a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
        }

        value->Resetreference();
        if (testcond) {
			if (a->needInvestigate()) {
				if (a == aBREAK)
					return aNULL;

				return a;
			}
        }
        a->Releasenonconst();
		return aNULL;
	}

    for (; v < t && !testcond; v += i) {
        a->Releasenonconst();
        value->value = v;
        a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }
    
    value->Resetreference();
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return aNULL;
            
            return a;
        }
    }
    
    a->Releasenonconst();
    return aNULL;
}

Tamgu* TamguInstructionFORINRANGE::ExecuteDecimal(Tamgudecimal* value, Tamgu* context, short idthread) {
	float v, t, i;
	value->Setreference();

	v = instructions.vecteur[0]->Instruction(1)->Getdecimal(idthread);
	t = instructions.vecteur[0]->Instruction(2)->Getdecimal(idthread);
	i = instructions.vecteur[0]->Instruction(3)->Getdecimal(idthread);

	Tamgu* a = aNULL;
    bool testcond = false;
	if (i < 0) {
        for (; v > t && !testcond; v += i) {
            a->Releasenonconst();
            value->value = v;
            a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
        }
        
        value->Resetreference();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return aNULL;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }

    for (; v < t && !testcond; v += i) {
        a->Releasenonconst();
        value->value = v;
        a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }
    
    value->Resetreference();
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return aNULL;
            
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}

Tamgu* TamguInstructionFORINRANGE::ExecuteFloat(Tamgufloat* value, Tamgu* context, short idthread) {
	double v, t, i;

	value->Setreference();

	v = instructions.vecteur[0]->Instruction(1)->Getfloat(idthread);
	t = instructions.vecteur[0]->Instruction(2)->Getfloat(idthread);
	i = instructions.vecteur[0]->Instruction(3)->Getfloat(idthread);

	Tamgu* a = aNULL;
    bool testcond = false;
    if (i < 0) {
        for (; v > t && !testcond; v += i) {
            a->Releasenonconst();
            value->value = v;
            a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
        }
        
        value->Resetreference();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return aNULL;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }

    for (; v < t && !testcond; v += i) {
        a->Releasenonconst();
        value->value = v;
        a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }
    
    value->Resetreference();
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return aNULL;
            
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}

Tamgu* TamguInstructionFORINRANGE::ExecuteLong(Tamgulong* value, Tamgu* context, short idthread) {
	BLONG v, t, i;

	value->Setreference();

	v = instructions.vecteur[0]->Instruction(1)->Getlong(idthread);
	t = instructions.vecteur[0]->Instruction(2)->Getlong(idthread);
	i = instructions.vecteur[0]->Instruction(3)->Getlong(idthread);

	Tamgu* a = aNULL;
    bool testcond = false;
    if (i < 0) {
        for (; v > t && !testcond; v += i) {
            a->Releasenonconst();
            value->value = v;
            a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
        }
        
        value->Resetreference();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return aNULL;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }
    
    for (; v < t && !testcond; v += i) {
        a->Releasenonconst();
        value->value = v;
        a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }
    
    value->Resetreference();
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return aNULL;
            
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}

Tamgu* TamguInstructionFORINRANGE::ExecuteShort(Tamgushort* value, Tamgu* context, short idthread) {
	short v, t, i;

	value->Setreference();

	v = instructions.vecteur[0]->Instruction(1)->Getshort(idthread);
	t = instructions.vecteur[0]->Instruction(2)->Getshort(idthread);
	i = instructions.vecteur[0]->Instruction(3)->Getshort(idthread);

	Tamgu* a = aNULL;
    bool testcond = false;
    if (i < 0) {
        for (; v > t && !testcond; v += i) {
            a->Releasenonconst();
            value->value = v;
            a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
        }
        
        value->Resetreference();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return aNULL;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }
    
    for (; v < t && !testcond; v += i) {
        a->Releasenonconst();
        value->value = v;
        a = instructions.vecteur[1]->Eval(context, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }
    
    value->Resetreference();
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return aNULL;
            
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}

Tamgu* TamguInstructionFORINRANGEINTEGER::Eval(Tamgu* context, Tamgu* a, short idthread) {
	long v, t, i;

	v = init->Getinteger(idthread);
	t = test->Getinteger(idthread);
	i = increment->Getinteger(idthread);

	Tamguint* value = (Tamguint*)recipient->Eval(context, aNULL, idthread);

    a = aNULL;
    bool testcond = false;
    if (i < 0) {
        for (; v > t && !testcond; v += i) {
            a->Releasenonconst();
            value->value = v;
            a = instruction->Eval(aNULL, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
        }
        
        value->Release();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return aNULL;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }

    for (; v < t && !testcond; v += i) {
        a->Releasenonconst();
        value->value = v;
        a = instruction->Eval(aNULL, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }
    
    value->Release();
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return aNULL;
            
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}

Tamgu* TamguInstructionFORINRANGEDECIMAL::Eval(Tamgu* context, Tamgu* a, short idthread) {
	float v, t, i;

	v = init->Getdecimal(idthread);
	t = test->Getdecimal(idthread);
	i = increment->Getdecimal(idthread);

	Tamgudecimal* value = (Tamgudecimal*)recipient->Eval(context, aNULL, idthread);

    a = aNULL;
    bool testcond = false;
    if (i < 0) {
        for (; v > t && !testcond; v += i) {
            a->Releasenonconst();
            value->value = v;
            a = instruction->Eval(aNULL, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
        }
        
        value->Release();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return aNULL;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }

    for (; v < t && !testcond; v += i) {
        a->Releasenonconst();
        value->value = v;
        a = instruction->Eval(aNULL, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }
    
    value->Release();
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return aNULL;
            
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}

Tamgu* TamguInstructionFORINRANGEFLOAT::Eval(Tamgu* context, Tamgu* a, short idthread) {
	double v, t, i;
	
	v = init->Getfloat(idthread);
	t = test->Getfloat(idthread);
	i = increment->Getfloat(idthread);

	Tamgufloat* value = (Tamgufloat*)recipient->Eval(context, aNULL, idthread);
    
    a = aNULL;
    bool testcond = false;
    if (i < 0) {
        for (; v > t && !testcond; v += i) {
            a->Releasenonconst();
            value->value = v;
            a = instruction->Eval(aNULL, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
        }
        
        value->Release();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return aNULL;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }

    for (; v < t && !testcond; v += i) {
        a->Releasenonconst();
        value->value = v;
        a = instruction->Eval(aNULL, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }
    
    value->Release();
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return aNULL;
            
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}

Tamgu* TamguInstructionFORINRANGELONG::Eval(Tamgu* context, Tamgu* a, short idthread) {
	BLONG v, t, i;

	v = init->Getlong(idthread);
	t = test->Getlong(idthread);
	i = increment->Getlong(idthread);

	Tamgulong* value = (Tamgulong*)recipient->Eval(context, aNULL, idthread);
    
    a = aNULL;
    bool testcond = false;
    if (i < 0) {
        for (; v > t && !testcond; v += i) {
            a->Releasenonconst();
            value->value = v;
            a = instruction->Eval(aNULL, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
        }
        
        value->Release();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return aNULL;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }

    for (; v < t && !testcond; v += i) {
        a->Releasenonconst();
        value->value = v;
        a = instruction->Eval(aNULL, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }
    
    value->Release();
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return aNULL;
            
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}

Tamgu* TamguInstructionFORINRANGESHORT::Eval(Tamgu* context, Tamgu* a, short idthread) {
	short v, t, i;

	v = init->Getshort(idthread);
	t = test->Getshort(idthread);
	i = increment->Getshort(idthread);

	Tamgushort* value = (Tamgushort*)recipient->Eval(context, aNULL, idthread);
    
    a = aNULL;
    bool testcond = false;
    if (i < 0) {
        for (; v > t && !testcond; v += i) {
            a->Releasenonconst();
            value->value = v;
            a = instruction->Eval(aNULL, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
        }
        
        value->Release();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return aNULL;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }

    for (; v < t && !testcond; v += i) {
        a->Releasenonconst();
        value->value = v;
        a = instruction->Eval(aNULL, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
    }
    
    value->Release();
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return aNULL;
            
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}

//------------------------------------------------------------------------------

Tamgu* TamguInstructionFORINRANGECONSTINTEGER::Eval(Tamgu* context, Tamgu* a, short idthread) {
    Tamguint* value = (Tamguint*)recipient->Eval(context, aNULL, idthread);
    long v = V;
    
    a = aNULL;
    bool testcond = false;
    if (i < 0) {
        while (v > t && !testcond) {
            value->value = v;
            a = instruction->Eval(aNULL, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
            v+=i;
        }
        
        value->Releasenonconst();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return this;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }
    
    while (v < t && !testcond) {
        value->value = v;
        a = instruction->Eval(aNULL, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
        v+=i;
    }
    
    value->Releasenonconst();
    
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return this;
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}
    
Tamgu* TamguInstructionFORINRANGECONSTDECIMAL::Eval(Tamgu* context, Tamgu* a, short idthread) {
    Tamgudecimal* value = (Tamgudecimal*)recipient->Eval(context, aNULL, idthread);
    float v = V;
    
    
    a = aNULL;
    bool testcond = false;
    if (i < 0) {
        while (v > t && !testcond) {
            value->value = v;
            a = instruction->Eval(aNULL, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
            v+=i;
        }
        
        value->Releasenonconst();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return this;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }
    
    while (v < t && !testcond) {
        value->value = v;
        a = instruction->Eval(aNULL, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
        v+=i;
    }
    
    value->Releasenonconst();
    
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return this;
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}
    
Tamgu* TamguInstructionFORINRANGECONSTFLOAT::Eval(Tamgu* context, Tamgu* a, short idthread) {
    double v=V;
    Tamgufloat* value = (Tamgufloat*)recipient->Eval(context, aNULL, idthread);
    
    a = aNULL;
    bool testcond = false;
    if (i < 0) {
        while (v > t && !testcond) {
            value->value = v;
            a = instruction->Eval(aNULL, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
            v+=i;
        }
        
        value->Releasenonconst();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return this;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }
    
    while (v < t && !testcond) {
        value->value = v;
        a = instruction->Eval(aNULL, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
        v+=i;
    }
    
    value->Releasenonconst();
    
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return this;
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}
    
Tamgu* TamguInstructionFORINRANGECONSTLONG::Eval(Tamgu* context, Tamgu* a, short idthread) {
    BLONG v=V;
    Tamgulong* value = (Tamgulong*)recipient->Eval(context, aNULL, idthread);

    a = aNULL;
    bool testcond = false;
    if (i < 0) {
        while (v > t && !testcond) {
            value->value = v;
            a = instruction->Eval(aNULL, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
            v+=i;
        }
        
        value->Releasenonconst();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return this;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }
    
    while (v < t && !testcond) {
        value->value = v;
        a = instruction->Eval(aNULL, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
        v+=i;
    }
    
    value->Releasenonconst();
    
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return this;
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}
    
Tamgu* TamguInstructionFORINRANGECONSTSHORT::Eval(Tamgu* context, Tamgu* a, short idthread) {
    short v = V;
    Tamgushort* value = (Tamgushort*)recipient->Eval(context, aNULL, idthread);

    a = aNULL;
    bool testcond = false;
    if (i < 0) {
        while (v > t && !testcond) {
            value->value = v;
            a = instruction->Eval(aNULL, aNULL, idthread);
            testcond = a->needInvestigate() || executionbreak;
            v+=i;
        }
        
        value->Releasenonconst();
        if (testcond) {
            if (a->needInvestigate()) {
                if (a == aBREAK)
                    return this;
                
                return a;
            }
        }
        a->Releasenonconst();
        return aNULL;
    }
    
    while (v < t && !testcond) {
        value->value = v;
        a = instruction->Eval(aNULL, aNULL, idthread);
        testcond = a->needInvestigate() || executionbreak;
        v+=i;
    }
    
    value->Releasenonconst();
    
    if (testcond) {
        if (a->needInvestigate()) {
            if (a == aBREAK)
                return this;
            return a;
        }
    }
    a->Releasenonconst();
    return aNULL;
}
//------------------------------------------------------------------------------
Tamgu* TamguInstructionTRY::Eval(Tamgu* res, Tamgu* ins, short idthread) {

	short last = instructions.size() - 1;
	bool catchbloc = false;
	if (instructions.vecteur[last]->Action() == a_catchbloc) {
		last--;
		catchbloc = true;
	}

    TamguDeclarationLocal* environment = globalTamgu->Providedeclaration(this, idthread, false);
    
    res = aNULL;
    bool testcond = false;
    _setdebugmin(idthread);
	for (short i = 0; i < last && !testcond; i++) {
        res->Releasenonconst();
        res = instructions.vecteur[i];
        
        _debugpush(res);
		res = res->Eval(environment, aNULL, idthread);
        _debugpop();
        
        testcond = res->needFullInvestigate() || globalTamgu->Error(idthread);
    }

    _cleandebugmin;
    if (testcond) {
        if (globalTamgu->Error(idthread)) {
            instructions.vecteur[last]->Eval(aNULL, aNULL, idthread);
            res = aFALSE;
            if (catchbloc)
                res = instructions.vecteur[last + 1]->Eval(aNULL, aNULL, idthread);
            //if res has no reference, then it means that it was recorded into the environment
            if (!res->Reference())
                environment->Release();
            else {
                res->Setreference();
                //we clean our structure...
                environment->Release();
                res->Protect();
            }

            return res;
        }
        
        if (executionbreak) {
            environment->Release();
            return aNULL;
        }
        
        //if res has no reference, then it means that it was recorded into the environment
        if (!res->Reference())
            environment->Release();
        else {
            res->Setreference();
            //we clean our structure...
            environment->Release();
            res->Protect();
        }
        
        return res;
    }
    
    environment->Release();
	return aNULL;
}


Tamgu* TamguInstructionCATCH::Eval(Tamgu* context, Tamgu* a, short idthread) {
	long size = instructions.size();


	if (size == 1) {
        a = instructions.vecteur[0];
        _setdebugfull(idthread, a);
		a = a->Eval(context, aNULL, idthread);
		_cleandebugfull;
		return a;
	}

    _setdebugmin(idthread);

    a = aNULL;
    bool testcond = false;
	for (long i = 0; i < size && !testcond; i++) {
        a->Releasenonconst();

        a = instructions.vecteur[i];

		_debugpush(a);
		a = a->Eval(aNULL, aNULL, idthread);
		_debugpop();

        testcond = executionbreak || a->needFullInvestigate();
    }
    
    if (testcond) {
		if (a->needFullInvestigate()) {
			_cleandebugmin;
			return a;
		}
	}

    a->Releasenonconst();

	_cleandebugmin;
	return aNULL;
}
    
    wstring TamguSQUARE::UString() {
        return L"²";
    }
    
    wstring TamguCallSQUARE::UString() {
        return L"²";
    }
    
    wstring TamguCUBE::UString() {
        return L"³";
    }
    
    wstring TamguCallCUBE::UString() {
        return L"³";
    }

