/*
 *  Tamgu (탐구)
 *
 * Copyright 2019-present NAVER Corp.
 * under BSD 3-clause
 */
/* --- CONTENTS ---
 Project    : Tamgu (탐구)
 Version    : See tamgu.cxx for the version number
 filename   : jag.h
 Date       : 2017/09/01
 Purpose    : Jag (작) class 
 Programmer : Claude ROUX (claude.roux@naverlabs.com)
 Reviewer   :
 */

#ifndef jag_h
#define jag_h

#define solo_line 0
#define beg_line 1
#define concat_line 2

#define u_del_linked -2
#define u_del -1
#define u_modif 0
#define u_modif_linked 1
#define u_ins 2
#define u_ins_linked 3

#define l_str 1
#define l_com 2
#define l_com_one 3

#ifdef APPLE
#define openMode std::ios::in|std::ios::binary
#else
#define openMode ios::in|ios::binary
#endif

class jag_editor;

extern jag_editor* JAGEDITOR;

long VirtualIndentation(string& codestr);

#ifdef PASDECOULEUR
const char m_current[] = {0,0};
const char m_red[] = {0,0};
const char m_redital[] = {0,0};
const char m_redbold[] = {0,0};
const char m_green[] = {0,0};
const char m_dore[] = {0,0};
const char m_blue[] = {0,0};
const char m_gray[] = {0,0};
const char m_lightgray[] = {0,0};
const char m_selectgray[] = {0,0};
#else
const char m_current[] = {27, '[', '0', 'm', 0};
const char m_red[] = {27, '[', '0', ';', '3','1', ';','4','9','m',0};
const char m_redital[] = {27, '[', '3', ';', '3','1', ';','4','9','m',0};
const char m_redbold[] = {27, '[', '1', ';', '3','1', ';','4','9','m',0};
const char m_green[] = {27, '[', '0', ';', '3','2', ';','4','9','m',0};
const char m_dore[] = {27, '[', '0', ';', '3','3', ';','4','9','m',0};
const char m_blue[] = {27, '[', '0', ';', '3','4', ';','4','9','m',0};
const char m_gray[] = {27, '[', '0', ';', '9','0', ';','4','9','m',0};
const char m_lightgray[] = {27, '[', '0', ';', '9','0', ';','4','9','m',0};

const char m_selectgray[] = {27, '[', '7', ';', '9','3', ';','4','0','m',0};
#endif

const string colordenomination[] = {"strings", "methods", "keywords", "functions", "comments"};

    //Background
const int m_clbg[] = {40, 41, 42, 43, 44, 45, 46, 47, 49, 100, 101, 102, 103, 104, 105, 106, 107, 0};
    //Foreground
const int m_clfg[] =  {30, 31, 32, 33, 34, 35, 36, 37, 39, 90, 91, 92, 93, 94, 95, 96, 97, 0};
    //Formatting
const int m_attr[] = {0, 1, 2, 4, 5, 7, -1};

    //action, moving the cursor...
const char m_left[] = {27, '[', '1', 68, 0};
const char m_right[] = {27, '[', '1', 67, 0};
const char m_down[] = {27, '[', '1', 66, 0};
const char m_up[] = {27, '[', '1', 65, 0};
    //On macos you might need to insert these key combination in the keyboard preference as: \033[1;5A and \033[1;5B
const char c_up[] = {27, 91, 49, 59, 53, 65, 0};
const char c_down[] = {27, 91, 49, 59, 53, 66, 0};

    //arrow moving
const char right[] = {27, 91, 67, 0};
const char left[] = {27, 91, 68, 0};
const char a_right[] = {27, 102, 0};
const char a_left[] = {27, 98, 0};
const char c_right[] = {27, 91, 49, 59, 53, 67, 0};
const char c_left[] = {27, 91, 49, 59, 53, 68, 0};

const char m_clear[] = {27, 91, '2', 'J', 0};
const char m_scrollup[] = {27, 91, '1', 'S', 0};
const char m_scrolldown[] = {27, 91, '1', 'T', 0};
const char m_home[] = {27, 91, 'H', 0};

const char down[] = {27, 91, 66, 0};
const char up[] = {27, 91, 65, 0};
const char del[] = {27, 91, 51, 126, 0};

const char homekey[] = {27, 91, 72, 0};
const char endkey[] = {27, 91, 70, 0};

const char back = 13;

const char cursor_position[] = {27, 91, '6', 'n', 0};

class editor_lines {
public:
    jag_editor* jag;
    vector<wstring> lines;
    vector<char> status;
    vector<long> numeros;
    hmap<long, bool> checks;
    
    editor_lines(jag_editor* j) {
        jag = j;
    }
    
    bool check(long p) {
        if (checks.find(p) != checks.end())
            return true;
        return false;
    }
    
    void setcode(wstring& code);
    
    void updatesize();
    
    void numbers() {
        long nb = 0;
        numeros.clear();
        for (long i = 0; i < lines.size(); i++) {
            if (status[i] != 2)
                nb++;
            numeros.push_back(nb);
        }
        
        updatesize();
    }
    
    long getlinenumber(long l) {
        for (long i = 0; i < numeros.size(); i++) {
            if (numeros[i] == l)
                return i;
        }
        return 0;
    }
    
    wstring code() {
        wstring c;
        bool rc = false;
        for (long i = 0; i < lines.size(); i++) {
            if (status[i] != concat_line) {
                if (rc) {
                    c += L"\n";
                    rc = false;
                }
                c += lines[i];
                if (status[i] == solo_line)
                    c += '\n';
                continue;
            }
            c += lines[i];
            rc = true;
        }
        return c;
    }
    
    wstring code(long first, long end) {
        wstring c;
        bool rc = false;
        if (first < 0)
            first = 0;
        if (end > lines.size())
            end = lines.size();
        for (long i = first; i < end; i++) {
            if (status[i] != concat_line) {
                if (rc) {
                    c += L"\n";
                    rc = false;
                }
                c += lines[i];
                if (status[i] == solo_line)
                    c += '\n';
                continue;
            }
            c += lines[i];
            rc = true;
        }
        return c;
    }
    
    wstring& operator[](long pos) {
        if (pos == lines.size())
            push(L"");
        return lines[pos];
    }
    
    long size() {
        return lines.size();
    }
    
    long indent(long p) {
        long i = p;
        wchar_t c;
        while (i >= 0) {
            c = lines[i][0];
            if (c != 32 && c!= 9 && c)
                break;
            i--;
        }
        wstring cd = code(i, p + 1);
        string ccd;
        s_unicode_to_utf8(ccd, cd);
        return VirtualIndentation(ccd);
    }
    
    long splitline(wstring& l, long linenumber, vector<wstring>& subs);
    
    char Status(long pos) {
        if (pos >= 0 && pos < lines.size())
            return status[pos];
        return 0;
    }
    
    //Check if we can go up to the end of the line
    bool eol(long p) {
        if (Status(p) != solo_line &&  Status(p+1) == concat_line)
            return false;
        return true;
    }
        //the line is cut after pos (either for destruction of copy)
        //the line is cut after pos (either for destruction of copy)
    char updatestatus(long pos);
    
    void erase(long pos) {
        lines.erase(lines.begin()+pos);
        status.erase(status.begin()+pos);
    }
    
    void erase(long pos, long end) {
        if (end >= lines.size())
            end = -1;
        
        if (end == -1) {
            lines.erase(lines.begin()+pos, lines.end());
            status.erase(status.begin()+pos, status.end());
        }
        else {
            lines.erase(lines.begin()+pos, lines.begin() + end);
            status.erase(status.begin()+pos, status.begin() + end);
        }
    }

    void insert(long pos, wstring& sub, char st) {
        lines.insert(lines.begin() + pos, sub);
        status.insert(status.begin()+pos, st);
        updatesize();
    }
    
    void insert(long pos, wstring& sub) {
        lines.insert(lines.begin() + pos, sub);
        status.insert(status.begin()+pos, solo_line);
        updatesize();
    }
    
    void inserting(long pos, wstring sub) {
        lines.insert(lines.begin() + pos, sub);
        status.insert(status.begin()+pos, solo_line);
        updatesize();
    }
    
    void push_back(wstring& sub) {
        lines.push_back(sub);
        status.push_back(solo_line);
        if (numeros.size())
            numeros.push_back(numeros.back()+1);
        else
            numeros.push_back(1);
        updatesize();
    }
    
    void push(wstring sub) {
        lines.push_back(sub);
        status.push_back(solo_line);
        if (numeros.size())
            numeros.push_back(numeros.back()+1);
        else
            numeros.push_back(1);
        updatesize();
    }
    
    void pop_back() {
        lines.pop_back();
        status.pop_back();
        numeros.pop_back();
    }
    
    wstring getoneline(long pos, long& end) {
        if (status[pos] == solo_line) {
            end = pos;
            return lines[pos];
        }
        
        wstring line = lines[pos++];
        while (pos < lines.size() && Status(pos) == concat_line)
            line += lines[pos++];
        
        end = pos-1;
        return line;
    }
    
    void clear() {
        checks.clear();
        lines.clear();
        status.clear();
        numeros.clear();
    }
    
    wstring back() {
        return lines.back();
    }
    
    bool checkfullsize(wstring&, bool&);
    bool checksize(long p);
    void undo(wstring& l, long p, char a);
    
    void replaceline(long p, long end, wstring& line) {
        bool equal = false;
        if (checkfullsize(line, equal)) {
                //We need to protect the last line
            vector<wstring> subs;
            splitline(line, numeros[p], subs);
            long u;
            for (u = 0; u < subs.size(); u++) {
                if ((p+u) < end)
                    lines[p+u] =  subs[u];
                else {
                    lines.insert(lines.begin()+p+u, subs[u]);
                    status.insert(status.begin()+p+u, concat_line);
                }
            }
            
            if ((p+u) < end)
                erase(p+u, end);
            numbers();
            return;
        }
        lines[p] = line;
    }
    
    void refactoring(long p);
    
    
};
    ///------------------------------------------------------------------------------------

class editor_keep {
public:
    list<wstring> l_keeplines;
    list<long> l_keeppos;
    list<char> l_keepactions;
    list<long> l_keepposinstring;
    list<long> l_keepcurrentline;
    list<long> l_keeptop;
    list<char> l_keepstatus;
    
    void pop() {
        l_keeplines.pop_back();
        l_keeppos.pop_back();
        l_keepactions.pop_back();
        l_keepcurrentline.pop_back();
        l_keepposinstring.pop_back();
        l_keeptop.pop_back();
        l_keepstatus.pop_back();
    }
    
    void move(wstring& l, editor_keep& e) {
        l_keeplines.push_back(l);
        l_keeptop.push_back(e.l_keeptop.back());
        l_keeppos.push_back(e.l_keeppos.back());
        l_keepactions.push_back(e.l_keepactions.back());
        l_keepposinstring.push_back(e.l_keepposinstring.back());
        l_keepcurrentline.push_back(e.l_keepcurrentline.back());
        l_keepstatus.push_back(e.l_keepstatus.back());
        e.pop();
    }
    
    void clear() {
        l_keeplines.clear();
        l_keeppos.clear();
        l_keepactions.clear();
        l_keepcurrentline.clear();
        l_keepposinstring.clear();
        l_keeptop.clear();
        l_keepstatus.clear();
    }
    
    void store(long postop, wstring& line, long pos, char action, long currentline, long posinstring, char status) {
        l_keeptop.push_back(postop);
        l_keeplines.push_back(line);
        l_keeppos.push_back(pos);
        l_keepactions.push_back(action); //deletion in this case in a modification
        l_keepcurrentline.push_back(currentline);
        l_keepposinstring.push_back(posinstring);
        l_keepstatus.push_back(status);
    }
    
    void storein(editor_keep& e) {
        e.l_keeptop = l_keeptop;
        e.l_keeplines = l_keeplines;
        e.l_keeppos = l_keeppos;
        e.l_keepactions = l_keepactions;
        e.l_keepcurrentline = l_keepcurrentline;
        e.l_keepposinstring = l_keepposinstring;
        e.l_keepstatus = l_keepstatus;
    }
    
    void prune() {
        if (l_keepactions.size() >= 10000) {
            list<wstring>::iterator srange_end = l_keeplines.begin();
            std::advance(srange_end,1000);
            l_keeplines.erase(l_keeplines.begin(), srange_end);
            
            list<long>::iterator prange_end = l_keeppos.begin();
            std::advance(prange_end,1000);
            l_keeppos.erase(l_keeppos.begin(), prange_end);
            
            list<char>::iterator arange_end = l_keepactions.begin();
            std::advance(arange_end,1000);
            l_keepactions.erase(l_keepactions.begin(), arange_end);
            
            list<long>::iterator psrange_end = l_keepposinstring.begin();
            std::advance(psrange_end,1000);
            l_keepposinstring.erase(l_keepposinstring.begin(), psrange_end);
            
            list<long>::iterator crange_end = l_keepcurrentline.begin();
            std::advance(crange_end,1000);
            l_keepcurrentline.erase(l_keepcurrentline.begin(), crange_end);
            
            list<long>::iterator trange_end = l_keeptop.begin();
            std::advance(trange_end,1000);
            l_keeptop.erase(l_keeptop.begin(), trange_end);
            
            list<char>::iterator strange_end = l_keepstatus.begin();
            std::advance(strange_end,1000);
            l_keepstatus.erase(l_keepstatus.begin(), strange_end);
        }
    }
    
    bool empty() {
        if (l_keeplines.size() == 0)
            return true;
        return false;
    }
    
    void display() {
        string s;
        wstring w = l_keeplines.back();
        s_unicode_to_utf8(s, w);
        cout << s << endl;
        cout << l_keeppos.back() << endl;
        cout << l_keepactions.back() << endl;
        cout << l_keepcurrentline.back() << endl;
        cout << l_keepposinstring.back() << endl;
        cout << l_keeptop.back() << endl;
        cout << l_keepstatus.back() << endl;
    }
    
};

typedef enum { x_none, x_goto, x_find, x_replace, x_write, x_count, x_delete, x_copy, x_cut, x_copying, x_deleting, x_cutting, x_load, x_exitprint} x_option;
class Jag_automaton;

class jag_editor {
public:
    vector<wstring> commandlines;
    long poscommand;
    int prefixsize;
    
    stringstream localhelp;

    std::wstringstream st;
    
    editor_lines lines;
    
    vector<long> poslines;
    
    editor_keep undos;
    editor_keep redos;
    
    struct winsize wns;
    Jag_automaton* findrgx;
    
    long row_size, col_size;
    
    long pos;
    long posinstring;
    long currentposinstring;
    long currentfindpos;
    long currentline;

    long margin;
    long spacemargin;

    int xcursor, ycursor;
    string thecurrentfilename;
    string prefix;
    wstring wprefix;
    wstring line;
    wstring currentfind;
    wstring currentreplace;
    wstring kbuffer;
    wstring copybuffer;
    
    bool echochar;
    bool replaceall;
    bool modified;
    bool tooglehelp;
    bool updateline;
    bool regularexpressionfind;
    bool noprefix;
    
    vector<long> longstrings;
    x_option option;
    vector<string> colors;

    struct termios oldterm;

    jag_editor();
    ~jag_editor();

    
    virtual void displaythehelp(long noclear = 0);
    
    void setpathname(string path) {
        thecurrentfilename =  path;
    }
    
    string pathname() {
        return thecurrentfilename;
    }
    
    wstring wpathname() {
        wstring name = wconvert(thecurrentfilename);
        return name;
    }
    
    void screensizes() {
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &wns);
        row_size = wns.ws_row - 2; //we need to keep a final line on screen for operators
        col_size = wns.ws_col - margin;
        if (row_size < 0)
            row_size = 100;
        if (col_size < 0)
            col_size = 100;
    }
    
    void setscrolling();
    
    void resetscrolling();
    
    long colsize() {
        return col_size;
    }
    
    void clearst() {
        st.str(L"");
        st.clear();
    }
    
    virtual bool emode() {
        if (option == x_none)
            return true;
        return false;
    }
    
    //------------------------------------------------------------------------------------------------
    //Undo/Redo
    //------------------------------------------------------------------------------------------------
    
    void setnoprefix() {
        noprefix = 1-noprefix;
        if (noprefix) {
            prefix = "";
            wprefix = L"";
            prefixsize = 0;
            margin = 2;
            spacemargin = 1;
        }
        else {
            margin = 10;
            spacemargin = 9;
            prefix = "작";
            wprefix = L"작";
            setprefixesize(lines.size());
        }
        resetscreen();
    }
    
    bool isc() {
        if (thecurrentfilename != "") {
            if (thecurrentfilename.find(".java") != -1)
                return true;
            if (thecurrentfilename.find(".c") != -1)
                return true;
            if (thecurrentfilename.find(".h") != -1)
                return true;
        }
        return false;
    }
    
    bool istamgu() {
        if (thecurrentfilename != "") {
            if (thecurrentfilename.find(".tmg") != -1)
                return true;
        }
        return false;
    }
    
    bool ispy() {
        if (thecurrentfilename != "") {
            if (thecurrentfilename.find(".py") != -1)
                return true;
        }
        return false;
    }
    
    void undo(wstring& l, long p, char a) {
        if (!emode() || p >= lines.size())
            return;
        
        modified = true;
        redos.clear();
        undos.prune();
        undos.store(poslines[0], l, p, a, currentline, posinstring, lines.Status(p));
    }
    
    void processredos();
    
    void processundos();
    
        //------------------------------------------------------------------------------------------------
        //Syntactic coloration...
        //------------------------------------------------------------------------------------------------
    
    void colorring(const char* txt, vector<long>& limits);
    virtual string coloringline(wstring& l, long p = -1, bool select = false);
    void vsplit(wstring& thestr, wstring thesplitter, vector<wstring>& vs);
    
        //------------------------------------------------------------------------------------------------
        //Cursor movements...
        //------------------------------------------------------------------------------------------------
    
    void backtoline();
    void selectfound(long l, long r);
    void movetoposition();
    void movetobeginning();
    void movetoend(bool remove = true);
    void movetolastline();
    void movetoline(long e);
    void gotoline(long p);
    virtual bool updown(char drt, long& pos);
    
    void getcursor();
    
    void toggletopbottom() {
        if (poslines.size() == 0)
            return;
        
        if (currentline == 0)
            currentline = poslines.size() -1;
        else
            currentline = 0;
        
        movetoline(currentline);
        pos = poslines[currentline];
        line = lines[pos];
        posinstring = line.size();
        movetoend();
    }
    
        //------------------------------------------------------------------------------------------------
        //Size calculations...
        //------------------------------------------------------------------------------------------------
    
        //Since there is always a prefix at the beginning of the line, we compute it here...
    
    long prefixesize(long sz) {
        if (noprefix)
            return 0;
        return (sz > 9999 ? 5 : sz > 999 ? 4: sz > 99 ? 3 : sz > 9 ? 2 : 1);
    }
    
    void setprefixesize(long sz) {
        if (noprefix) {
            prefixsize = 0;
            return;
        }
        prefixsize = sz > 9999 ? 5 : sz > 999 ? 4: sz > 99 ? 3 : sz > 9 ? 2 : 1 ;
    }

    long prefixe() {
        if (noprefix)
            return 0;
        return (4+prefixsize);
    }
    
    virtual long prefixego() {
        wstring s;
        switch(option) {
            case x_goto:
                s = L"Line:";
                break;
            case x_find:
                if (regularexpressionfind)
                    s = L"Find(rgx):";
                else
                    s = L"Find:";
                break;
            case x_replace:
                if (regularexpressionfind)
                    s = L"Find(rgx):";
                else
                    s = L"Find:";
                s += currentfind;
                s += L"  Replace:";
                break;
            case x_write:
                s = L"File:";
                break;
            case x_count:
                s = L"Count:";
                break;
            case x_delete:
            case x_copy:
            case x_cut:
            case x_deleting:
            case x_copying:
            case x_cutting:
            case x_load:
                s = st.str();
                break;
            default:
                return prefixe();
        }
        return s.size();
    }
    
    long splitline(wstring& l, long linenumber, vector<wstring>& subs) {
            //we compute the position of each segment of l on string...
        
        long ps = pos;
        pos = linenumber;
        long sz = cjk_size(l);
        if ((l.size()+sz) < col_size) {
            subs.push_back(l);
            pos = ps;
            return 1;
        }
        
        long p = 0;
        wstring code;
        if (!sz) {
            while (p < l.size()) {
                code = l.substr(p, col_size);
                subs.push_back(code);
                p += col_size;
            }
            pos = ps;
            return subs.size();
        }
        
        
        while (p < l.size()) {
            code = l.substr(p, col_size);
            while (fullsize(code) > col_size)
                code.pop_back();
            subs.push_back(code);
            p += code.size();
        }
        pos = ps;
        return subs.size();
    }
    
        //The main problem here is that emojis can be composed...
        //An emoji can be composed of up to 7 emojis...
    void forwardemoji();
    void backwardemoji();
    
        //We find the beginning of each emoji, skipping composed ones...
        //We build a string with no composed emoji, to match the position of the cursor...
    void cleanlongemoji(wstring& s, wstring& cleaned, long p);
    
        //This size is computed to take into account Chinese/Japanese/Korean characters...
        //These characters can occupy up to two columns... We also take into account the tab size
    long taille(wstring& s);
    
    long cjk_size(wstring& l) {
        wstring cleaned;
        cleanlongemoji(l, cleaned, l.size());
        long sz = taille(cleaned);
        return (sz - cleaned.size());
    }
    
    
    
    long fullsize(wstring& l) {
        wstring s;
        cleanlongemoji(l, s, l.size());
        return taille(s);
    }
    
    long size_upto(wstring& l, long p) {
        wstring s;
        cleanlongemoji(l, s, p);
        return taille(s);
    }
    
    long linesize() {
        if (emode())
            return lines[poslines[currentline]].size();
        return line.size();
    }
    
        //--------------------------------------------------------------------------------
        //Display methods....
        //--------------------------------------------------------------------------------
    void clearscreen();
    void clearline();
    void displayonlast(bool bck);
    void displayonlast(wstring w, bool bck = false);
    void displayonlast(string s, bool bck);
    virtual void displaygo(bool full);
    
    //We detect long commented lines or long strings
    virtual void scanforlonglines();
    
    void resetlist(long i) {
        poslines.clear();
        long mx = i + row_size;
        
        while (i <= mx) {
            poslines.push_back(i);
            i++;
        }
    }
    
    void displaylist(long beg);

    virtual void printline(long n, string l) {
        if (noprefix)
            cout << back << l;
        else
            cout << back << m_dore << prefix << m_current << m_lightgray << std::setw(prefixsize) << n << "> " << m_current << l;
    }
    
    virtual void printline(long n) {
        if (noprefix)
            cout << back;
        else
            cout << back << m_dore << prefix << m_current << m_lightgray << std::setw(prefixsize) << n << "> " << m_current;
    }

    virtual void printline(long n, wstring& l, long i = -1) {
        if (noprefix)
            cout << back << coloringline(l, i);
        else
            cout << back << m_dore << prefix << m_current << m_lightgray << std::setw(prefixsize) << n << "> " << m_current << coloringline(l, i);
    }

        //------------------------------------------------------------------------------------------------
        //Deletion methods...
        //------------------------------------------------------------------------------------------------
    
        //The deletion of a character is different if it is an emoji...
    long deleteachar(wstring& l, bool last, long pins);
    void deletechar();
    
        //Delete all characters after the cursor
    void deleteallafter() {
        undo(lines[pos],pos, u_modif); //The value is negative to indicate a deletion
        
        wstring code = lines[poslines[currentline]];
        kbuffer = code.substr(posinstring, code.size());
        code = code.substr(0, posinstring);
        lines[poslines[currentline]] = code;
        lines.refactoring(poslines[currentline]);
        displaylist(poslines[0]);
        movetoline(currentline);
        movetoposition();
    }
    
        //moveup means that the cursor must be positionned on the line above...
    void deleteline(char moveup);
    
        //------------------------------------------------------------------------------------------------
        //formating method...
        //------------------------------------------------------------------------------------------------
    
    void indentcode(long ps, bool lisp) {
        if (!lines.size())
            return;
        
        wstring code = lines.code();
        
        string codeindente;
        string cd = convert(code);
        long nbspaces = 3;
        IndentCode(cd, codeindente, nbspaces, lisp);
        lines.clear();
        code = wconvert(codeindente);
        code += L"\n\n";
        
        lines.setcode(code);
        
        displaylist(poslines[0]);
        movetoline(currentline);
        movetoposition();
    }
    
    virtual void setcode(string& c) {
        wstring code = wconvert(c);
        lines.setcode(code);
        displaylist(0);
        line = L"";
        currentline = 0;
        movetoline(0);
        if (poslines.size()) {
            line = lines[0];
            posinstring = line.size();
            movetoend();
        }
    }
    
        //------------------------------------------------------------------------------------------------
        //search method...
        //------------------------------------------------------------------------------------------------
    
    bool search(wstring& l, long& first, long& last, long ps);
    void processgo();
    bool processfind();
    long processcount();
    void processreplace();
    bool findnext();
    
    void resetsearch();
    
        //------------------------------------------------------------------------------------------------
        //command methods...
        //------------------------------------------------------------------------------------------------
    virtual bool writetofile() {
        wstring code = lines.code();
        
        ofstream wd(thecurrentfilename, ios::binary);
        if (wd.fail())
            return false;
        wd << convert(code);
        wd.close();
        return true;
    }
    
    string getch();
    long getbuffsize();
    bool check_utf8(string& buff, string& buffer);
    
    virtual bool loadfile(wstring& name) {
        if (!loadfile(convert(name))) {
            clearst();
            st << L"Cannot load:" << name;
            displayonlast(true);
            return false;
        }
        return true;
    }
    
    virtual bool loadfile(string name) {
        setpathname(name);
        ifstream rd(pathname(), openMode);
        if (rd.fail())
            return false;
        
        string code = "";
        string line;
        while (!rd.eof()) {
            getline(rd, line);
            line = Trimright(line);
            code += line + "\n";
        }
        Trimright(code);
        code += "\n\n";
        setcode(code);
        return true;
    }

    string convert(wstring& w) {
        string s;
        s_unicode_to_utf8(s, w);
        return s;
    }
    
    wstring wconvert(string& s) {
        wstring w;
        s_utf8_to_unicode(w, USTR(s), s.size());
        return w;
    }
    
    
        //A CR was hit, we need either to modify the current line or to add a new one...
        //If the cursor was in the middle of a line, then we split it in two
    
    long handlemultiline() ;
    long handlingeditorline(bool computespace = true);
    
    void evaluateescape(string& buff);
    virtual void init();
    virtual void clear();

    
    //This is the main instruction to add characters into the editor
    void addabuffer(wstring& b, bool instring);
    
    void handleblock(wstring& bl) {
        
            //We keep track of the initial form of the line...
        undo(lines[pos],pos, u_modif); //The value is negative to indicate a deletion
        
        wstring b;
        for (long j = 0; j < bl.size(); j++) {
            b = bl[j];
            if (b[0] == 10) {
                pos = handlingeditorline(false);
                continue;
            }
            addabuffer(b, false);
        }
        displaylist(poslines[0]);
        movetoline(currentline);
        movetoposition();
    }
    
    void convertmetas();
    //This section handles combined commands introduced with Ctrl-x
    virtual bool checkcommand(char);
    void handlecommands();
    
        //This a case of copy/paste within the editor, we need to remove the prefixes
    void cleanheaders(wstring& w);
    //This is the main method that launches the terminal
    virtual void launchterminal(char loadedcode);
    bool checkaction(string&, long& first, long& last, bool lsp = false);
    
    virtual void addcommandline(wstring& w) {}
    
    virtual void terminate();
    
    virtual void resetscreen() {
        modified = true;
        screensizes();
        wstring code = lines.code();
        lines.setcode(code);
        displaylist(poslines[0]);
        movetoline(currentline);
        posinstring = 0;
        movetobeginning();
    }
    
    bool checksize(wstring& l) {
        if (fullsize(l) > col_size)
            return true;
        return false;
    }
    
    bool checksizeequal(wstring& l, bool& equal) {
        long ll = fullsize(l);
        if (ll == col_size) {
            equal = true;
            return true;
        }
        
        if (ll > col_size)
            return true;
        return false;
    }
    
    string thecode() {
        wstring c = lines.code();
        return convert(c);
    }
    
};

#endif



