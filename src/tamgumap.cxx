/*
 *  Tamgu (탐구)
 *
 * Copyright 2019-present NAVER Corp.
 * under BSD 3-clause
 */
/* --- CONTENTS ---
 Project    : Tamgu (탐구)
 Version    : See tamgu.cxx for the version number
 filename   : tamgumap.cxx
 Date       : 2017/09/01
 Purpose    : map implementation
 Programmer : Claude ROUX (claude.roux@naverlabs.com)
 Reviewer   :
*/

#include "tamgu.h"
#include "tamgumap.h"
#include "tamgulist.h"
#include "tamguivector.h"
#include <memory>

//We need to declare once again our local definitions.
Exporting basebin_hash<mapMethod>  Tamgumap::methods;
Exporting hmap<string, string> Tamgumap::infomethods;
Exporting bin_hash<unsigned long> Tamgumap::exported;

Exporting short Tamgumap::idtype = a_map;

//If we have variables in our map, we create them on the fly...
//Else there might be some expected values or keys in the map...
//constant is 1, if we have only variables. At the end of the process, when constant is 1, we replace it with 0
//constant is 2, if we have only constants.
//constant is 3, it is an error... A mix of constants and variables...
//constant is 4, if we have only constants and a pipe (a split)
void TamguConstmap::Prepare(Tamgu* env, short idthread) {
    evaluate = true;
    Tamgu* v;
    constant = 0;
    for (size_t i = 0; i < values.size(); i++) {
        v = keys[i];
        if (v != aPIPE) {
            if (v->isCallVariable()) {
                if (constant == 2) {
                    constant = 3;
                    return;
                }
                constant = 1;
                v->Put(env, aNULL, idthread);
            }
            else {
                if (v->isVariable()) {
                    if (constant == 2) {
                        constant = 3;
                        return;
                    }
                    constant = 1;
                    v->Eval(env, aNULL, idthread);
                }
                else {
                    if (v->isConst()) {
                        if (constant == 1) {
                            constant = 3;
                            return;
                        }
                    }
                    constant = 2;
                    v->Prepare(env, idthread);
                }
            }
        }
        else {
            if (constant == 2)
                constant = 4;
        }

        v = values[i];
        if (v->isCallVariable())
            v->Put(env, aNULL, idthread);
        else
            if (v->isVariable())
                v->Eval(env, aNULL, idthread);
            else
                v->Prepare(env, idthread);
    }

    if (constant == 1)
        constant = 0;
}

bool TamguConstmap::Checkvariable() {
    if (values.size() != 1)
        return false;

    if (values[0]->isCallVariable() && keys[0]->isCallVariable())
        return true;

    return false;
}

bool TamguConstmap::Setvalue(Tamgu* iter, Tamgu* value, short idthread, bool strict) {


    Tamgu* k = ((TamguIteration*)iter)->IteratorKey();
    keys[0]->Setaffectation(true);
    value = keys[0]->Eval(aNULL, aNULL, idthread);
    value->Put(aNULL, k, idthread);

    Tamgu* v = ((TamguIteration*)iter)->IteratorValue();
    values[0]->Setaffectation(true);
    value = values[0]->Eval(aNULL, aNULL, idthread);
    value->Put(aNULL, v, idthread);

    return true;
}

Tamgu* TamguConstmap::same(Tamgu* value) {
    short idthread = globalTamgu->GetThreadid();

    if (!value->isMapContainer())
        return aFALSE;

    Tamgu* a;
    Tamgu* v;

    std::unique_ptr<TamguIteration> it(value->Newiteration(false));

    Locking _lock(this);

    it->Begin();
    for (size_t i = 0; i < values.size(); i++) {
        a = keys[i];
        if (a == aPIPE) {
            //Then we are in a split...
            //the value is a map...
            a = values[i];
            if (it->End() != aTRUE) {
                TamguIndex idx(true);
                idx.left = it->IteratorKey();
                idx.right = aNULL;
                v = value->Eval(aNULL, &idx, idthread);
            }
            else
                v = value->Newinstance(idthread);
        }
        else {
            if (it->End() == aTRUE)
                return aFALSE;

            v = it->IteratorKey();

            if (a->isCallVariable()) {
                a->Setaffectation(true);
                a = a->Eval(aNULL, aNULL, idthread);
                a->Putvalue(v, idthread);
            }
            else {
                a = a->Eval(aNULL, aNULL, idthread);
                if (a != aNOELEMENT && a->same(v) == aFALSE)
                    return aFALSE;
            }
            a = values[i];
            v = it->IteratorValue();
        }

        if (a->isCallVariable()) {
            a->Setaffectation(true);
            a = a->Eval(aNULL, aNULL, idthread);
            a->Putvalue(v, idthread);
            continue;
        }
        
        if (a->isVectorContainer()) {
            if (!v->isVectorContainer())
                return aFALSE;

            a = a->Putvalue(v, idthread);
            if (a->isError())
                return a;
            continue;
        }

        if (a->isMapContainer()) {
            if (!v->isMapContainer())
                return aFALSE;
            a = a->Putvalue(v, idthread);
            if (a->isError())
                return a;
            continue;
        }

        a = a->Eval(aNULL, aNULL, idthread);
        if (a != aNOELEMENT && a->same(v) == aFALSE)
            return aFALSE;

        it->Next();
    }

    return aTRUE;
}

class CleanKeys {
    public:

    VECTE<Tamgu*> vkeys;

    void push_back(Tamgu* a) {
        vkeys.push_back(a);
    }

    CleanKeys(Tamgu* a, char constant) {
        if (constant == 2)
            return;

        TamguIteration* it = a->Newiteration(false);
        for (it->Begin(); it->End() == aFALSE; it->Next())
            vkeys.push_back(it->Key());
        it->Release();
    }

    ~CleanKeys() {
        for (int i = 0; i < vkeys.last; i++)
            vkeys[i]->Release();
    }

    inline Tamgu* operator [](long pos) {
        return vkeys.vecteur[pos];
    }

    void remove(Tamgu* a) {
        for (long i = 0; i < vkeys.last; i++) {
            if (vkeys.vecteur[i]->same(a) == aTRUE) {
                vkeys.vecteur[i]->Release();
                vkeys.vecteur[i] = aNOELEMENT;
                return;
            }
        }
    }

    Tamgu* first() {
        if (vkeys.size())
            return vkeys[0];
        return aNOELEMENT;
    }
    
    void pop_first() {
        if (vkeys.size())
            vkeys.erase(0);
    }
    
    long size() {
        return vkeys.last;
    }

    long remaining() {
        long nb = 0;
        for (int i = 0; i < vkeys.last; i++)  {
            if (vkeys[i] != aNOELEMENT)
                nb++;
        }
        return nb;
    }

};

Tamgu* TamguConstmap::Put(Tamgu* index, Tamgu* value, short idthread) {
    if (!evaluate)
        return this;

    if (!value->isMapContainer()) {
        if (index->isError())
            return aRAISEERROR;
        return globalTamgu->Returnerror("Wrong affectation", idthread);
    }

    if (constant == 3) {
        if (index->isError())
            return aRAISEERROR;
        return globalTamgu->Returnerror("Cannot mix constant keys with variables", idthread);
    }

    Tamgu* a;
    Tamgu* v = aNULL;
    long j = 0;

    //std::unique_ptr<TamguIteration> it(value->Newiteration(false));
    CleanKeys vkeys(value, constant);

    Locking _lock(this);

    for (size_t i = 0; i < values.size(); i++) {
        a = keys[i];
        if (a == aPIPE) {
            //Then we are in a split...
            //the value is a map...
            a = values[i];
            if (a == aUNIVERSAL)
                return this;
            
            v = value->Newinstance(idthread);
            //we store the remaining values in a new map, that will be assigned to a
            if (constant) {
                //In this case, we get rid of all keys that were already taken into account...
                //See below in section: constant==4
                for (j = 0; j < vkeys.size(); j++) {
                    if (vkeys[j] != aNOELEMENT)
                        v->Push(vkeys[j], value->Value(vkeys[j]));
                }
            }
            else {
                for (; j < vkeys.size(); j++) {
                    v->Push(vkeys[j], value->Value(vkeys[j]));
                }
            }
        }
        else {
            if (constant) {
                if (a == aUNIVERSAL) {
                    a = vkeys.first();
                    vkeys.pop_first();
                }
                else {
                    if (constant == 4)
                        vkeys.remove(a);
                }
                
                v = value->Value(a);
                if (v == aNOELEMENT) {
                    if (index->isError())
                        return aRAISEERROR;
                    return globalTamgu->Returnerror("Unknown key", idthread);
                }
                
                a = values[i];
            }
            else {
                if (j >= vkeys.size()) {
                    if (index->isError())
                        return aRAISEERROR;
                    return globalTamgu->Returnerror("Input map is smaller than pattern", idthread);
                }

                v = vkeys[j++];
                if (a->isCallVariable()) {
                    a->Setaffectation(true);
                    a = a->Eval(aNULL, aNULL, idthread);
                    a->Putvalue(v, idthread);
                }
                else {
                    if (a->isVariable()) {
                        //Then it should have created before hand...
                        a = globalTamgu->Getdeclaration(a->Name(), idthread);
                        a->Putvalue(v, idthread);
                    }
                    else {
                        a = a->Eval(aNULL, aNULL, idthread);
                        if (a->same(v) == aFALSE) {
                            if (index->isError())
                                return aRAISEERROR;
                            return globalTamgu->Returnerror("Unknown key", idthread);
                        }
                    }
                }
                a = values[i];
                v = value->Value(v);
            }
        }

        if (a->isCallVariable()) {
            a->Setaffectation(true);
            a = a->Eval(aNULL, aNULL, idthread);
            a->Putvalue(v, idthread);
            continue;
        }
        if (a->isVariable()) {
            //Then it should have created before hand...
            a = globalTamgu->Getdeclaration(a->Name(), idthread);
            a->Putvalue(v, idthread);
            continue;
        }

        if (a->isVectorContainer()) {
            if (!v->isVectorContainer()) {
                if (index->isError())
                    return index;
                return globalTamgu->Returnerror("Mismatch assignment, expecting two vector containers.", idthread);
            }
            a = a->Put(index, v, idthread);
            if (a->isError())
                return a;
            continue;
        }

        if (a->isMapContainer()) {
            if (!v->isMapContainer()) {
                if (index->isError())
                    return index;
                return globalTamgu->Returnerror("Mismatch assignment, expecting two map containers.", idthread);
            }
            a = a->Put(index, v, idthread);
            if (a->isError())
                return a;
            continue;
        }

        a = a->Eval(aNULL, aNULL, idthread);
        if (a->same(v) == aFALSE) {
            if (index->isError())
                return aRAISEERROR;
            return globalTamgu->Returnerror("Mismatch assignment", idthread);
        }
    }
    
    return this;
}

Tamgu* TamguConstmap::Eval(Tamgu* index, Tamgu* value, short idthread) {
    if (affectation && evaluate)
        return this;

    Locking _lock(this);

    Tamgumap* kmap = globalTamgu->Providemap();
    BLONG it;
    BLONG sz = values.size();

    if (isEvaluate()) {
        Tamgu* k;
        Tamgu* v;
        for (it = 0; it < sz; it++) {
            k = keys[it]->Eval(aNULL, aNULL, idthread);
            v = values[it]->Eval(aNULL, aNULL, idthread);
            kmap->Push(k, v);
            k->Release();
            v->Release();
        }
    }
    else {
        for (it = 0; it < sz; it++)
            kmap->Push(keys[it], values[it]);
    }
    return kmap;
}

//-------------------------------------------------------------------------
//MethodInitialization will add the right references to "name", which is always a new method associated to the object we are creating
void Tamgumap::AddMethod(TamguGlobal* global, string name, mapMethod func, unsigned long arity, string infos) {
    short idname = global->Getid(name);
    methods[idname] = func;
    infomethods[name] = infos;
    exported[idname] = arity;
}



    void Tamgumap::Setidtype(TamguGlobal* global) {
        Tamgumap::idtype = global->Getid("map");
    }

   bool Tamgumap::InitialisationModule(TamguGlobal* global, string version) {
    methods.clear();
    infomethods.clear();
    exported.clear();


    Tamgumap::idtype = global->Getid("map");

    Tamgumap::AddMethod(global, "clear", &Tamgumap::MethodClear, P_NONE, "clear(): clear the container.");

    Tamgumap::AddMethod(global, "items", &Tamgumap::MethodItems, P_NONE, "items(): Return a vector of {key:value} pairs.");
    
    Tamgumap::AddMethod(global, "invert", &Tamgumap::MethodInvert, P_NONE, "invert(): return a map with key/value inverted.");
    Tamgumap::AddMethod(global, "find", &Tamgumap::MethodFind, P_ONE, "find(value): test if a value belongs to the map and return 'true' or the corresponding keys.");
    
    Tamgumap::AddMethod(global, "join", &Tamgumap::MethodJoin, P_TWO, "join(string sepkey,string sepvalue): Produce a string representation for the container.");
    global->returntypes[global->Getid("join")] = a_string;

    Tamgumap::AddMethod(global, "test", &Tamgumap::MethodTest, P_ONE, "test(key): Test if key belongs to the map container.");
    Tamgumap::AddMethod(global, "keys", &Tamgumap::MethodKeys, P_NONE, "keys(): Return the map container keys as a vector.");
    global->returntypes[global->Getid("keys")] = a_vector;

    Tamgumap::AddMethod(global, "values", &Tamgumap::MethodValues, P_NONE, "values(): Return the map container values as a vector.");
    global->returntypes[global->Getid("values")] = a_vector;

    Tamgumap::AddMethod(global, "sum", &Tamgumap::MethodSum, P_NONE, "sum(): return the sum of elements.");
    global->returntypes[global->Getid("sum")] = a_float;

    Tamgumap::AddMethod(global, "product", &Tamgumap::MethodProduct, P_NONE, "product(): return the product of elements.");
    global->returntypes[global->Getid("product")] = a_float;

    Tamgumap::AddMethod(global, "pop", &Tamgumap::MethodPop, P_ONE, "pop(key): Erase an element from the map");
    Tamgumap::AddMethod(global, "merge", &Tamgumap::MethodMerge, P_ONE, "merge(v): Merge v into the vector.");

    global->newInstance[Tamgumap::idtype] = new Tamgumap(global);
    #ifdef OLDBACKCOMP
    global->newInstance[global->Getid("smap")] = new Tamgumap(global);
    global->newInstance[global->Getid("maps")] = new Tamgumap(global);

    global->RecordMethods(global->Getid("maps"), Tamgumap::exported);
    global->RecordMethods(global->Getid("smap"), Tamgumap::exported);
    #endif
    global->RecordMethods(Tamgumap::idtype, Tamgumap::exported);

    global->newInstance[a_constmap] = new TamguConstmap(global);
    global->RecordMethods(a_constmap, Tamgumap::exported);
    
    return true;
}


Exporting TamguIteration* Tamgumap::Newiteration(bool direction) {
    return new TamguIterationmap(this, direction);
}



Exporting Tamgu* Tamgumap::in(Tamgu* context, Tamgu* a, short idthread) {
    //Three cases along the container type...
    //It is a Boolean, we expect false or true
    //It is an integer, we expect a position in v
    //It is a container, we are looking for all positions...

    string val = a->String();

     if (context->isVectorContainer()) {
        Tamgusvector* v = (Tamgusvector*)Selectasvector(context);
        Doublelocking _lock(this, v);
        if (values.find(val) != values.end())
            v->values.push_back(val);

        return v;
    }

   if (context->isString()) {
        Locking _lock(this);
        if (values.find(val) != values.end())
            return globalTamgu->Providestring(val);
        return aNOELEMENT;
    }

    Locking _lock(this);
    if (values.find(val) != values.end())
        return aTRUE;

    return aFALSE;

}

Exporting Tamgu* Tamgumap::MethodFind(Tamgu* context, short idthread, TamguCall* callfunc) {
    //Three cases along the container type...
    //It is a Boolean, we expect false or true
    //It is an integer, we expect a position in v
    //It is a container, we are looking for all positions...

    Tamgu* a = callfunc->Evaluate(0, context, idthread);

    if (context->isBoolean()) {
        Locking _lock(this);
        for (auto& it : values) {
            if (it.second->same(a) == aTRUE)
                return aTRUE;
        }
        return aFALSE;
    }

    if (context->isVectorContainer()) {
        Tamgusvector* v = (Tamgusvector*)Selectasvector(context);
        Doublelocking _lock(this, v);

        for (auto& it : values) {
            if (it.second->same(a) == aTRUE)
                v->values.push_back(it.first);
        }
        return v;
    }

    Locking _lock(this);
    for (auto& it : values) {
        if (it.second->same(a) == aTRUE)
            return globalTamgu->Providestring(it.first);
    }

    return aNULL;

}

Exporting void Tamgumap::Cleanreference(short inc) {
    Locking* _lock = _getlock(this);
    for (auto& a : values)
        a.second->Removecontainerreference(inc);

    _cleanlock(_lock);
}

Exporting void Tamgumap::Setreference(short inc) {
    if (loopmark)
        return;
    
    reference += inc;
    protect = false;
    loopmark=true;
    
    Locking* _lock = _getlock(this);
    for (auto& it : values)
        it.second->Addreference(inc);
    _cleanlock(_lock);
    
    loopmark=false;
}

Exporting void Tamgumap::Setreference() {
    if (loopmark)
        return;
    
    ++reference;
    protect = false;
    loopmark=true;
    
    Locking* _lock = _getlock(this);
    for (auto& it : values)
        it.second->Addreference(1);
    _cleanlock(_lock);
    
    loopmark=false;
}

static void resetMap(Tamgumap* kmap, short inc) {
    kmap->reference -= inc;
    
    Locking* _lock = _getlock(kmap);
    
    hmap<string, Tamgu*>& values = kmap->values;
    if (values.size() == 0) {
        _cleanlock(_lock);
        return;
    }
    
    for (auto& itx : values)
        itx.second->Removereference(inc);
    _cleanlock(_lock);
}

Exporting void Tamgumap::Resetreference(short inc) {
    if ((reference + containerreference - inc) > 0)
        resetMap(this, inc);
    else {
        resetMap(this, inc + 1 - protect);
        if (!protect) {
            if (idtracker != -1)
                globalTamgu->RemoveFromTracker(idtracker);
            delete this;
        }
    }
}

Exporting void Tamgumapbuff::Resetreference(short inc) {
    if ((reference + containerreference - inc) > 0)
        resetMap(this, inc);
    else {
        resetMap(this, inc + 1 - protect);
        if (!protect) {
            reference = 0;
            protect = true;

            values.clear();
            used = false;
            if (!globalTamgu->globalLOCK)
                globalTamgu->mapempties.push_back(idx);
        }
    }
}


Exporting Tamgu* Tamgumap::Push(Tamgu* k, Tamgu* v) {
    Locking _lock(this);
    string s = k->String();
    if (values.find(s) != values.end()) {
        Tamgu* kval = values[s];
        values.erase(s);
        kval->Removereference(reference + 1);
    }

    v = v->Atom();
    values[s] = v;
    v->Addreference(reference+1);
    return aTRUE;
}

Exporting Tamgu* Tamgumap::Pop(Tamgu* kkey) {
    string k = kkey->String();
    Locking _lock(this);
    if (values.find(k) != values.end()) {
        kkey = values[k];
        values.erase(k);
        kkey->Removereference(reference + 1);
        return aTRUE;
    }
    return aFALSE;
}

Exporting void Tamgumap::Clear() {
    hmap<string, Tamgu*>::iterator itx;
    for (auto& itx : values)
        itx.second->Removereference(reference + 1);
    values.clear();
}



Exporting string Tamgumap::String() {
    Locking _lock(this);
    if (loopmark)
        return("{...}");
    TamguCircular _c(this);
    stringstream res;
    
    res << "{";
    bool beg = true;
    string sx;
    for (auto& it : values) {
        if (beg == false)
            res << ",";
        beg = false;
        sx = it.first;
        stringing(res, sx);
        res << ":";
        sx = it.second->StringToDisplay();
        if (!it.second->isString() || it.second->isContainer())
            res << sx;
        else
            stringing(res, sx);
    }
    res << "}";
    return res.str();
}

Exporting string Tamgumap::JSonString() {
    Locking _lock(this);
    if (loopmark)
        return("");
    TamguCircular _c(this);
    stringstream res;
    
    res << "{";
    bool beg = true;
    string sx;
    for (auto& it : values) {
        if (beg == false)
            res << ",";
        beg = false;
        sx = it.first;
        jstringing(res, sx);
        res << ":" << it.second->JSonString();
    }
    res << "}";
    return res.str();
}


Exporting long Tamgumap::Integer() {
    Locking _lock(this);
    return values.size();
}

Exporting double Tamgumap::Float() {
    Locking _lock(this);
    return values.size();
}

Exporting BLONG Tamgumap::Long() {
    Locking _lock(this);
    return values.size();
}

Exporting bool Tamgumap::Boolean() {
    Locking _lock(this);
    if (values.size() == 0)
        return false;
    return true;
}


//Basic operations
Exporting long Tamgumap::Size() {
    Locking _lock(this);
    return values.size();
}


Tamgu*  Tamgumap::Put(Tamgu* idx, Tamgu* ke, short idthread) {
    if (!idx->isIndex()) {
        if (ke == this)
            return aTRUE;

        if (ke->isNULL()) {
            Clear();
            return aTRUE;
        }
        
        if (ke->isMapContainer()) {
            Doublelocking _lock(this, ke);
            Clear();
            TamguIteration* itr = ke->Newiteration(false);
            for (itr->Begin(); itr->End() == aFALSE; itr->Next())
                Push(itr->Keystring(), itr->Value());
            itr->Release();
            return aTRUE;
        }

        char ch[20];
        if (ke->isVectorContainer()) {
            Doublelocking _lock(this, ke);
            Clear();
            long nb = 0;
            for (long it = 0; it < ke->Size(); ++it) {
                sprintf_s(ch, 20, "%ld", nb);
                Push(ch, ke->getvalue(it));
                nb++;
            }
            return aTRUE;
        }
        if (ke->Type() == a_list) {
            Doublelocking _lock(this, ke);
            Tamgulist* kvect = (Tamgulist*)ke;
            Clear();
            long nb = 0;

            for (auto& it : kvect->values) {
                sprintf_s(ch, 20, "%ld", nb);
                Push(ch, it);
                nb++;
            }
            return aTRUE;
        }
        ke = ke->Map(idthread);
        if (!ke->isMapContainer())
            return globalTamgu->Returnerror("Wrong map initialization", idthread);
        Locking* _lock = _getlock(this);
        Clear();
        if (ke->Type() == a_map) {
            Tamgumap* kmap = (Tamgumap*)ke;
            //We copy all values from ke to this
            
            for (auto& it : kmap->values)
                Push(it.first, it.second);
        }
        else {
            TamguIteration* itr = ke->Newiteration(false);
            Tamgu* a;
            for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
                a=itr->IteratorValue();
                a=a->Atom();
                values[itr->Keystring()] = a;
                a->Addreference(reference+1);
            }
            itr->Release();
        }
        ke->Release();
        _cleanlock(_lock);
        return aTRUE;
    }
    Push(STR(idx->String()), ke);
    return aTRUE;
}


Tamgu* Tamgumap::Eval(Tamgu* contextualpattern, Tamgu* idx, short idthread) {

    Locking _lock(this);

    if (!idx->isIndex()) {
        //particular case, the contextualpattern is a vector, which means that we expect a set of keys
        //as a result
                if (contextualpattern->isMapContainer())
            return this;
        
       //particular case, the contextualpattern is a vector, which means that we expect a set of keys
        //as a result
        if (contextualpattern->isVectorContainer() || contextualpattern->Type() == a_list) {
            Tamgu* vect = contextualpattern->Newinstance(idthread);
            
            for (auto& it : values)
                vect->Push(globalTamgu->Providestring(it.first));
            return vect;
        }

        if (contextualpattern->isNumber()) {
            long v = Size();
            return globalTamgu->Provideint(v);
        }

        return this;
    }

    Tamgu* key;
    if (idx->isInterval()) {
        Tamgumap* kmap = globalTamgu->Providemap();
        key = ((TamguIndex*)idx)->left->Eval(aNULL, aNULL, idthread);
        Tamgu* keyright = ((TamguIndex*)idx)->right->Eval(aNULL, aNULL, idthread);
        string vleft = key->String();
        string vright = keyright->String();
        hmap<string, Tamgu*>::iterator it = values.find(vleft);
        if (it == values.end() && key != aNULL) {
            key->Release();
            return kmap;
        }
        if (key == aNULL)
            it = values.begin();
        key->Release();
        hmap<string, Tamgu*>::iterator itr = values.end();
        if (keyright != aNULL) {
            itr = values.find(vright);
            if (itr == values.end()) {
                keyright->Release();
                return kmap;
            }
        }
        keyright->Release();
        for (; it != values.end(); it++) {
            kmap->Push(it->first, it->second);
            if (it == itr)
                return kmap;
        }

        if (itr != values.end())
            kmap->Clear();
        return kmap;

    }

    key = ((TamguIndex*)idx)->left->Eval(aNULL, aNULL, idthread);
    
    if (key == aNULL) {
        if (globalTamgu->erroronkey)
            return globalTamgu->Returnerror("Wrong index", idthread);
        return aNOELEMENT;
    }

    string skey = key->String();
    key->Release();

    Tamgu* kval = Value(skey);
    if (kval == aNOELEMENT) {
        if (globalTamgu->erroronkey)
            return globalTamgu->Returnerror("Wrong index", idthread);
        return aNOELEMENT;
    }
    return kval;
}

Tamgu* Tamgumap::same(Tamgu* a) {

    if (a->Type() != a_map)
        return Mapcompare(this, a, NULL);

    Tamgumap* m = (Tamgumap*)a;

    Doublelocking _lock(this, m);
    if (m->values.size() != values.size())
        return aFALSE;
    hmap<string, Tamgu*>::iterator it = m->values.begin();
    while (it != m->values.end()) {
        if (values.find(it->first) == values.end())
            return aFALSE;
        if (it->second->same(values[it->first]) == aFALSE)
            return aFALSE;
        it++;
    }
    return aTRUE;
}

Exporting Tamgu* Tamgumap::xorset(Tamgu* b, bool itself) {

    Doublelocking _lock(this, b);


    Tamgumap* res;
    
    
    if (b->isMapContainer()) {
        TamguIteration* itr = b->Newiteration(false);

        res = globalTamgu->Providemap();
        hmap<string, Tamgu*> keys;

        for (auto& it : values)
            keys[it.first] = it.second;
            
        string v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Keystring();
            if (keys.find(v) == keys.end())
                keys[v]=itr->IteratorValue();
            else {
                if (values[v]->same(itr->IteratorValue()) == aTRUE)
                    keys.erase(v);
            }
        }
        itr->Release();

        for (auto& a : keys)
            res->Push(a.first,a.second);

        return res;


    }


    if (itself)
        res = this;
    else
        res = (Tamgumap*)Atom(true);

    
    for (auto& it : values)
        it.second->xorset(b, true);
    return res;
}

Exporting Tamgu* Tamgumap::orset(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    
    Tamgumap* res;
    if (itself)
        res = this;
    else
        res = (Tamgumap*)Atom(true);

    if (b->isMapContainer()) {
        res->Merging(b);
        return res;
    }

    for (auto& it : res->values)
        it.second->orset(b, true);
    return res;

}

Exporting Tamgu* Tamgumap::andset(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    
    Tamgumap* res;
    if (b->isMapContainer()) {
        TamguIteration* itr = b->Newiteration(false);

        res = globalTamgu->Providemap();
        Tamgu* v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->IteratorValue();
            if (values.find(itr->Keystring()) != values.end() && values[itr->Keystring()]->same(v) == aTRUE)
                res->Push(itr->Keystring(), v);
        }
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumap*)Atom(true);

    for (auto& it : res->values)
        it.second->andset(b, true);
    return res;
}

Exporting Tamgu* Tamgumap::plus(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumap* res;
    if (b->isMapContainer()) {
        hmap<string, Tamgu*>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = globalTamgu->Providemap();
        Tamgu* v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->IteratorValue();
            it = values.find(itr->Keystring());
            if (it != values.end()) {
                res->Push(it->first, it->second);
                res->values[it->first]->plus(v, true);
            }
        }
        
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumap*)Atom(true);

    for (auto& it : res->values)
        it.second->plus(b, true);
    return res;
}

Exporting Tamgu* Tamgumap::minus(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumap* res;
    if (b->isMapContainer()) {
        hmap<string, Tamgu*>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = globalTamgu->Providemap();
        Tamgu* v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->IteratorValue();
            it = values.find(itr->Keystring());
            if (it != values.end()) {
                res->Push(it->first, it->second);
                res->values[it->first]->minus(v, true);
            }
        }
        
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumap*)Atom(true);

    for (auto& it : res->values)
        it.second->minus(b, true);
    return res;
}

Exporting Tamgu* Tamgumap::multiply(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumap* res;
    if (b->isMapContainer()) {
        hmap<string, Tamgu*>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = globalTamgu->Providemap();
        Tamgu* v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->IteratorValue();
            it = values.find(itr->Keystring());
            if (it != values.end()) {
                res->Push(it->first, it->second);
                res->values[it->first]->multiply(v, true);
            }
        }
        
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumap*)Atom(true);

    for (auto& it : res->values)
        it.second->multiply(b, true);
    return res;
}

Exporting Tamgu* Tamgumap::divide(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumap* res;
    Tamgu* v;
    Tamgu* r;

    if (b->isMapContainer()) {
        hmap<string, Tamgu*>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = globalTamgu->Providemap();
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->IteratorValue();
            it = values.find(itr->Keystring());
            if (it != values.end()) {
                r = res->values[it->first]->divide(v, true);
                if (r->isError()) {
                    res->Release();
                    itr->Release();
                    return r;
                }
            }
        }
        
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumap*)Atom(true);

    for (auto& it : res->values) {
        r = it.second->divide(b, true);
        if (r->isError()) {
            res->Release();
            return r;
        }
    }

    return res;
}

Exporting Tamgu* Tamgumap::mod(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumap* res;
    Tamgu* v;
    Tamgu* r;

    if (b->isMapContainer()) {
        hmap<string, Tamgu*>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = globalTamgu->Providemap();
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->IteratorValue();
            it = values.find(itr->Keystring());
            if (it != values.end()) {
                r = res->values[it->first]->mod(v, true);
                if (r->isError()) {
                    res->Release();
                    itr->Release();
                    return r;
                }
            }
        }
        
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumap*)Atom(true);

    for (auto& it : res->values) {
        r = it.second->mod(b, true);
        if (r->isError()) {
            res->Release();
            return r;
        }
    }

    return res;
}

Exporting Tamgu* Tamgumap::shiftright(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumap* res;
    if (b->isMapContainer()) {
        hmap<string, Tamgu*>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = globalTamgu->Providemap();
        Tamgu* v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->IteratorValue();
            it = values.find(itr->Keystring());
            if (it != values.end()) {
                res->Push(it->first, it->second);
                res->values[it->first]->shiftright(v, true);
            }
        }
        
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumap*)Atom(true);

    for (auto& it : res->values)
        it.second->shiftright(b, true);
    return res;
}

Exporting Tamgu* Tamgumap::shiftleft(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumap* res;
    if (b->isMapContainer()) {
        hmap<string, Tamgu*>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = globalTamgu->Providemap();
        Tamgu* v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->IteratorValue();
            it = values.find(itr->Keystring());
            if (it != values.end()) {
                res->Push(it->first, it->second);
                res->values[it->first]->shiftleft(v, true);
            }
        }
        
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumap*)Atom(true);

    for (auto& it : res->values)
        it.second->shiftleft(b, true);
    return res;
}

Exporting Tamgu* Tamgumap::power(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumap* res;
    if (b->isMapContainer()) {
        hmap<string, Tamgu*>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = globalTamgu->Providemap();
        Tamgu* v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->IteratorValue();
            it = values.find(itr->Keystring());
            if (it != values.end()) {
                res->Push(it->first, it->second);
                res->values[it->first]->power(v, true);
            }
        }
        
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumap*)Atom(true);

    for (auto& it : res->values)
        it.second->power(b, true);
    return res;
}


Exporting Tamgu* Tamgumap::Loopin(TamguInstruction* ins, Tamgu* context, short idthread) {
    Locking _lock(this);
    Tamgu* var = ins->instructions.vecteur[0]->Instruction(0);
    var = var->Eval(context, aNULL, idthread);

    hmap<string, Tamgu*>::iterator it;
    
    Tamgu* a;
    vector<string> keys;

    for (it=values.begin(); it != values.end(); it++)
        keys.push_back(it->first);

    for (long i = 0; i < keys.size(); i++) {

        var->storevalue(keys[i]);

        a = ins->instructions.vecteur[1]->Eval(context, aNULL, idthread);

        //Continue does not trigger needInvestigate
        if (a->needInvestigate()) {
            if (a == aBREAK)
                break;
            return a;
        }

        a->Release();
    }

    return this;

}
