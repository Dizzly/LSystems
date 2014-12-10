////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//

//a data structure containing information about a single recursion of the LSystem
class LSystemState
{
public:
    LSystemState(LSystemState* prev = NULL) : readIndex(0), prevState(prev), level(0)
    {

    }
    ~LSystemState(){}

    LSystemState(const LSystemState& cpy)
    {
        readIndex = cpy.readIndex;
        level = cpy.level;
        prevState = cpy.prevState;
        userPointer = cpy.userPointer;
        state_.resize(cpy.state_.size());
        memcpy(state_.data(), cpy.state_.data(), cpy.state_.size());
    }

    void operator =(const LSystemState& cpy)
    {
        readIndex = cpy.readIndex;
        level = cpy.level;
        prevState = cpy.prevState;
        userPointer = cpy.userPointer;
        state_.resize(0);
        state_.resize(cpy.state_.size());
        memcpy(state_.data(), cpy.state_.data(), cpy.state_.size());
    }

    int readIndex;//variable indicating the read position in the previous state
    int level;//level of recursion depth
    const LSystemState* prevState;//last state behind it
    octet::dynarray<char> state_;//

    static void* userPointer;
};

struct LSystemDrawInfo{
public:
    LSystemDrawInfo() :sectionLength(0),
        sectionLengthReduction(0),
        sectionWidth(0),
        sectionWidthReduction(0),
        minXRot(0),
        maxXRot(0),
        minYRot(0),
        maxYRot(0),
        minZRot(0),
        maxZRot(0),
        randomize(false)
    {

    }
    void Combine(LSystemDrawInfo* di)
    {
        if (di->sectionLength)sectionLength = di->sectionLength;
        if (di->sectionLengthReduction)sectionLengthReduction = di->sectionLengthReduction;
        if (di->sectionWidth)sectionWidth = di->sectionWidth;
        if (di->sectionWidthReduction)sectionWidthReduction = di->sectionWidthReduction;

        if (di->minXRot)minXRot = di->minXRot;
        if (di->minYRot)minYRot = di->minYRot;
        if (di->minZRot)minZRot = di->minZRot;

        if (di->maxXRot)maxXRot = di->maxXRot;
        if (di->maxYRot)maxYRot = di->maxYRot;
        if (di->maxZRot)maxZRot = di->maxZRot;

        if (di->randomize)randomize = di->randomize;
    }
    float sectionLength;
    float sectionLengthReduction;

    float sectionWidth;
    float sectionWidthReduction;

    float minXRot;
    float minYRot;
    float minZRot;

    float maxXRot;
    float maxYRot;
    float maxZRot;

    bool randomize;
private:
};

//an abstract base class for creation of a intergrated graphics tool, not necessary
class LSystemVisualizer
{
public:
    virtual void Init(LSystemDrawInfo* info) = 0{};
    virtual void DrawLine() = 0{};

    virtual void RotatePositive() = 0{};
    virtual void RotateNegative() = 0{};

    virtual void PushStack() = 0{};
    virtual void PopStack() = 0{};

    virtual void DrawLeaf(){};
    virtual void Rotate(){};
    virtual void Custom(){};

    virtual void Finished(){};

    virtual void SetState(LSystemState* state) = 0{};
};

//the recursion engine for use in parsing each LSystem recursion to the next, holds most of the information about rules
class LSystem
{ //make these variables private
public:
    enum KEY_SYMBOLS{
        KEY_NULL = 0,
        KEY_DRAW,
        KEY_PLUS_ROTATE,
        KEY_MINUS_ROTATE,
        KEY_PUSH,
        KEY_POP,
        KEY_ROTATE,
        KEY_LEAF,
        KEY_CUSTOM,

        KEY_MAX
    };


    typedef void(*VarFunc)(LSystemState*);
public:
    LSystem():info_(NULL){}
    ~LSystem(){ delete(info_); }

    void Iterate(int n)
    {
        for (int i = 0; i < n; ++i)
        {
            Iterate();
        }
    }
    void Iterate()
    {
        assert(stateVec_.size()>0);
        stateVec_.push_back(new LSystemState(stateVec_.back()));
        stateVec_.back()->level = stateVec_.size()-1;
        const LSystemState* prevState = stateVec_.back()->prevState;
        assert(prevState->state_.size() > 0);
        for (int i = 0; i < prevState->state_.size(); ++i)
        {
            stateVec_.back()->readIndex = i;
            ProcessSymbol(prevState->state_[i]);
        }
    }

    void Decrement(int n){
        for (int i = 0; i < n; ++i)
        {
            Decrement();
        }
    }
    void Decrement()
    {
        stateVec_.pop_back();
    }

    void Collapse()
    {
        *stateVec_.data() = stateVec_.back();
        stateVec_.resize(1);
        stateVec_.back()->prevState = nullptr;
    }

    void Visualize(LSystemVisualizer* viz){
        if (viz)
        {
            LSystemState* state = stateVec_.back();
            viz->SetState(state);
            info_ ? viz->Init(info_) : viz->Init(NULL);
            for (int i = 0; i < state->state_.size(); ++i)
            {
                CallKey(referenceMap_[state->state_[i]].key, viz);
            }
            viz->Finished();
        }
    }

    void AddAlphabetSymbol(char symb)
    {
        alphabet_.push_back(symb);
    }

    void SetAxiom(const char* c, int size)
    {
        if (stateVec_.size() == 0)
        {
            axiom_.resize(size);
            memcpy(axiom_.data(), c, size);
            stateVec_.push_back(new LSystemState());
            stateVec_.back()->state_ = axiom_;
            stateVec_.back()->level = 1;
        }
    }

    void AddBasicRules(char c, octet::string& str)
    {
        referenceMap_[c].str = str;
    }

    void SetKeyDecl(char c, KEY_SYMBOLS k)
    {
        referenceMap_[c].key = k;
    }

    void AddRuleFunction(char c, VarFunc func)
    {
        referenceMap_[c].func = func;
    }

    void SetDrawInfo(LSystemDrawInfo* info)
    {
        info_ = info;
    }

    LSystemDrawInfo* GetDrawInfo(){
        return info_;
    }

    const LSystemState* GetCurrentState()
    {
        return stateVec_.back();
    }
private:
    struct SymbolRef
    {
        SymbolRef() : func(NULL), key(KEY_NULL){}
        octet::string str;
        VarFunc func;
        KEY_SYMBOLS key;
    };

private:


    void ProcessSymbol(const char c)
    {
        LSystemState* state = stateVec_.back();
        SymbolRef r = referenceMap_[c];
        if (r.str.size())
        {
            if (state->state_.size() > 0)
            {
                int oldSize = state->state_.size();
                state->state_.resize(state->state_.size() + r.str.size());
                memcpy(state->state_.data() + oldSize, r.str.c_str(), r.str.size());
            }
            else
            {
                state->state_.resize(r.str.size());
                memcpy(state->state_.data(), r.str.c_str(), r.str.size());
            }
        }
        else
        {
            state->state_.push_back(c);
        }

        if (r.func)
        {
            r.func(state);
        }
    }

    void CallKey(int key,LSystemVisualizer* viz)
    {
        switch (key)
        {
        case(KEY_DRAW) :
            viz->DrawLine();
            break;
        case(KEY_PLUS_ROTATE) :
            viz->RotatePositive();
            break;
        case(KEY_MINUS_ROTATE) :
            viz->RotateNegative();
            break;
        case(KEY_PUSH) :
            viz->PushStack();
            break;
        case(KEY_POP) :
            viz->PopStack();
            break;
        case(KEY_LEAF) :
            viz->DrawLeaf();
            break;
        case(KEY_ROTATE) :
            viz->Rotate();
            break;
        case(KEY_CUSTOM) :
            viz->Custom();
            break;
        }
    }

private:
    LSystemDrawInfo* info_;

    octet::dynarray<LSystemState*> stateVec_;
    octet::hash_map<char, SymbolRef> referenceMap_;
    octet::dynarray<char> axiom_;
    octet::dynarray<char> alphabet_;
};

#include <fstream>
//the importer kept seperate from the LSystem for reduction in function count and use
//only one public function, to read into an LSystem
class LSystemImporter
{
public:
    LSystemImporter(){}
    ~LSystemImporter(){}

    bool Load(LSystem* lSys, const char* filename)
    {
        std::fstream file;

        file.open(filename, std::ios::in);
        if (!file.is_open())
        {
            assert(0);
        }

        file.seekg(0, file.end);
        int length = file.tellg();
        file.seekg(0, file.beg);

        read_.resize(length);
        file.read(read_.data(), length);
        CleanWhiteSpace();

        file.close();

        octet::string str(read_.data(), read_.size());
        bool check = false;
        check = LoadKeyDeclerations(lSys, str);
        if (!check)return false;
        check = LoadAlphabet(lSys, str);
        if (!check)return false;
        check = LoadAxiom(lSys, str);
        if (!check)return false;
        check = LoadRules(lSys, str);
        if (!check)return false;
        check = LoadDrawInfo(lSys, str);
        if (!check) return false;

        read_.resize(0);
        tempAlphabet_.resize(0);

        return true;
    }
private:
    //load the symbol alphabet from the text file containing all non-inbuilt symbols in the L system
    //a local version is stored for error checking in the Axiom and Rule sections
    bool LoadAlphabet(LSystem* lSys, octet::string& str)
    {
        int startLoc = str.find("Alphabet");

        if (startLoc == -1)
        {
            printf("%s", "Could not find the Alphabet subsection.\n");
            assert(false);
            return false;
        }
        startLoc += 8;//size of "Alphabet"
        int endLoc = FindSymbolAfter('}', startLoc);
        if (endLoc == -1)
        {
            printf("%s", "No close brackets after Alphabet section.\n");
            assert(false);
            return false;
        }
        int counter = 0;

        for (int i = startLoc; i < endLoc; ++i)
        {
            if (IsNotGrammer(read_[i]))
            {
                lSys->AddAlphabetSymbol(read_[i]);
                tempAlphabet_.push_back(read_[i]);
                ++counter;
            }
        }
        return true;
    }

    //load the axiom, fairly simple, find the first string and load all of it
    //grammer inside the axiom will be ignored, but it will complain
    bool LoadAxiom(LSystem* lSys, octet::string& str)
    {
        int startLoc = str.find("Axiom");

        if (startLoc == -1)
        {
            printf("%s\n", "Could not find the Axiom subsection.\n");
            assert(false);
            return false;
        }
        startLoc += 5;//size of "Axiom"
        int endLoc = FindSymbolAfter('}', startLoc);
        if (endLoc == -1)
        {
            printf("%s\n", "No close brackets after Axiom section.\n");
            assert(false);
            return false;
        }
        octet::dynarray<char> arr;
        for (int i = startLoc; i < endLoc; ++i)
        {
            if (IsNotGrammer(read_[i]))
            {
                if (IsInAlphabet(read_[i]))
                {
                    arr.push_back(read_[i]);
                }
                else
                {
                    printf("%s%c%s\n", "Symbol ", read_[i], " is not in the Alphabet, but is in the Axiom");
                }
            }
            else if (read_[i] == ';')//if we find semicolon, be consistant and exit early
            {
                break;
            }
            else //otherwise complain about grammer inside Axiom, we dont care but they should
            {
                if (arr.size() > 0)
                {
                    printf("%s\n", "Grammer found in Axiom will be ignored, consider removing it.");
                }
            }
        }//END OF FOR
        if (arr.size() == 0)
        {
            printf("%s\n", "No axiom was found");
            assert(false);
            return false;
        }
        lSys->SetAxiom(arr.data(), arr.size());
        return true;
    }

    //loads the simple symbol replacement rules
    //will only read single chracter replacements ie(F=FF is fine, FF=FF not ok(read as F=FF))
    bool LoadRules(LSystem* lSys, octet::string& str)
    {
        int startLoc = str.find("Rules");
        if (startLoc == -1)
        {
            printf("%s", "Could not find the Rules subsection.\n");
            assert(false);
            return false;
        }
        startLoc += 6;//size of "Rules"
        int endLoc = FindSymbolAfter('}', startLoc);
        if (endLoc == -1)
        {
            printf("%s", "No close brackets after Rules section.\n");
            assert(false);
            return false;
        }

        for (int i = startLoc; i < endLoc; ++i)
        {
            if (read_[i] == '=')
            {
                char symbol(read_[i - 1]);
                if (IsNotGrammer(symbol))
                {
                    if (IsInAlphabet(symbol))
                    {
                        int ruleEndLoc = FindSymbolAfter(';', i);
                        if (ruleEndLoc>endLoc || ruleEndLoc == -1)
                        {
                            printf("%s%c%s\n", "No semicolon after ", symbol, "'s rule");
                            assert(false);
                            return false;
                        }
                        lSys->AddBasicRules(symbol, octet::string(&read_[i + 1], ruleEndLoc - 1 - i));// take the chracters till the ;, excluding the ;
                        i = ruleEndLoc + 1;//skip the rule we just read for effiency
                    }
                    else
                    {
                        printf("%s%c%s\n", "Symbol ", symbol, " is not in Alphabet, but is in Rules");
                    }
                }
            }//END OF GRAMMER
        }//END OF FOR
        return true;
    }

    //loads any special key replacements, by default +,-,[,],F are reserved for special use
    //Any of these can be overloaded with their own custom symbol
    bool LoadKeyDeclerations(LSystem* lSys, octet::string& str)
    {

        int startLoc = str.find("KeyDecl");
        lSys->SetKeyDecl('F', LSystem::KEY_DRAW);
        lSys->SetKeyDecl('[', LSystem::KEY_PUSH);
        lSys->SetKeyDecl(']', LSystem::KEY_POP);
        lSys->SetKeyDecl('+', LSystem::KEY_PLUS_ROTATE);
        lSys->SetKeyDecl('-', LSystem::KEY_MINUS_ROTATE);

        if (startLoc == -1)
        {
            return true;
        }
        startLoc += 7; //size of the string KeyDecl
        int endLoc = FindSymbolAfter('}', startLoc);
        if (endLoc == -1)
        {
            printf("%s", "No close brackets after KeyDecl section.\n");
            assert(false);
            return false;
        }

        for (int i = startLoc; i < endLoc; ++i)
        {
            if (read_[i] == '=')
            {
                char newSymbol;
                if (IsNotGrammer(read_[i - 1]))
                {
                    newSymbol = read_[i - 1];
                    int end = FindSymbolAfter(';', i);
                    octet::string key(&read_[i + 1], end - 1 - i);
                    LSystem::KEY_SYMBOLS keyNum = KeyFromString(key);
                    if (keyNum != LSystem::KEY_NULL)
                    {
                        lSys->SetKeyDecl(newSymbol, keyNum);
                        i += key.size();
                    }
                    else
                    {
                        printf("%s%s%s\n", "Malformed KeyDecl assignment, ", key.c_str(), " not recognised");
                    }
                }
                else
                {
                    printf("%s%c%s\n", "Malformed KeyDecl assignment, ", newSymbol, " is grammer");
                   
                }
            }//END OF READ IF
        }//END OF FOR
        return true;
    }

    bool LoadDrawInfo(LSystem* lSys, octet::string& str)
    {
        int startPos = str.find("DrawInfo");
        if (startPos == -1)
        {
            return true;
        }
        startPos += 9; //size of "DrawInfo{"
        int endPos = FindSymbolAfter('}', startPos);

        if (endPos == -1)
        {
            printf("%s", "No close brackets after DrawInfo section");
            assert(false);
            return false;
        }

        float arr[sizeof(LSystemDrawInfo) / sizeof(float)] = { 0 };
        int counter = 0;
        for (int i = startPos; i < endPos; ++i)
        {
            int equals = FindSymbolAfter('=', i);
            int eol = FindSymbolAfter(';',equals);
            if (eol>endPos)
            {
                printf("%s", "No semicolon after statement");
                assert(false);
                return false;
            }
            octet::string value(&read_[i], equals - i);
            octet::string key(&read_[equals + 1], (eol - equals)-1);
            arr[InfoLocFromString(key)] = atof(value);
            i = eol;
            counter++;
        }
        if (counter > 0)
        {
            LSystemDrawInfo* info = new LSystemDrawInfo();
            memcpy(info, arr, sizeof(info)*sizeof(float));
            lSys->SetDrawInfo(info);
        }
    
    }
private:
    //lineraly searches for the next instance of a symbol after a current point
    int FindSymbolAfter(char symbol, int loc)
    {
        for (int i = loc; i < read_.size(); ++i)
        {
            if (read_[i] == symbol)
            {
                return i;
            }
        }
        return -1;
    }
    // simple fix for built in types, conversion from string to enum
    static LSystem::KEY_SYMBOLS KeyFromString(const octet::string& c)
    {
        if (c == "KEY_DRAW")
        {
            return LSystem::KEY_DRAW;
        }
        if (c == "KEY_PLUS_ROTATE")
        {
            return LSystem::KEY_PLUS_ROTATE;
        }
        if (c == "KEY_MINUS_ROTATE")
        {
            return LSystem::KEY_MINUS_ROTATE;
        }
        if (c == "KEY_PUSH")
        {
            return LSystem::KEY_PUSH;
        }
        if (c == "KEY_POP")
        {
            return LSystem::KEY_POP;
        }

        //Possible extension of features
        if (c == "KEY_LEAF")
        {
            return LSystem::KEY_LEAF;
        }
        if (c == "KEY_ROTATE")
        {
            return LSystem::KEY_ROTATE;
        }
        if (c == "KEY_CUSTOM")
        {
            return LSystem::KEY_CUSTOM;
        }

        return LSystem::KEY_NULL;
    }

    static int InfoLocFromString(const octet::string& c)
    {
        if (c == "LENGTH")
        {
            return 0;
        }
        if (c == "LENGTH_REDUCTION")
        {
            return 1;
        }
        if (c == "WIDTH")
        {
            return 2;
        }
        if (c == "WIDTH_REDUCTION")
        {
            return 3;
        }
        if (c == "MIN_ROT_X")
        {
            return 4;
        }
        if (c == "MAX_ROT_X")
        {
            return 7;
        }
        if (c == "MIN_ROT_Y")
        {
            return 5;
        }
        if (c== "MAX_ROT_Y")
        {
            return 8;
        }
        if (c == "MIN_ROT_Z")
        {  
            return 6;
        }
        if (c == "MAX_ROT_Z")
        {
            return 9;
        }
    }

    //cheks to see if that char is grammer
    //grammer being defined as the characters in quote marks ||| , { } =  ; |||
    inline bool IsNotGrammer(char c)
    {
        return (c != ','&&c != '{'&&c != '}'&&c != '='&&c != ';');
    }

    //checks to see if that char was seen in our alphabet
    bool IsInAlphabet(char c)
    {
        //instead, make sure that the tempAlphabet vec is sorted,
        //or have 255 sized array containing bools if the symbol had been added
        //this will work for now, as it is unlikely the alphabet will be too long
        for (int i = 0; i < tempAlphabet_.size(); ++i)
        {
            if (tempAlphabet_[i] == c)
            {
                return true;
            }
        }
        return false;
    }

    //clean out whitespace from the data
    void CleanWhiteSpace()
    {
        //looks for new line, space and tabs and deletes them
        char* readPtr = read_.data();
        char* writePtr = read_.data();
        char* endPtr = read_.data() + read_.size();
        int counter = 0;
        while (readPtr != endPtr)
        {
            if (*readPtr != ' ' && *readPtr != '\n' && *readPtr != '\t')//if its not whitespace
            {
                ++counter; //we found a character, so increment eg (A,B,c,d etc...)
                if (*writePtr == ' ' || *writePtr == '\n' || *writePtr == '\t')// if we can write into whitespace
                {
                    *writePtr = *readPtr; //do it
                    *readPtr = ' ';// and replace our reading character with whitespace
                }
                ++readPtr;
                ++writePtr;
            }
            else
            {

                ++readPtr;//we are going over whitespace, skip it
            }
        }
        read_.resize(counter);//keep the size conistant
    }
private:
    octet::dynarray<char> tempAlphabet_;
    octet::dynarray<char> read_;
};




class DrawHelper2D: public LSystemVisualizer
{
public:

    DrawHelper2D():dir_(0,1,0){
        maxRot_ = 0;
        minRot_ = 0;
    lineLength_=0.1f;
    meshy_ = new octet::mesh();
    }

    void Init(LSystemDrawInfo* info)override
    {
        if (info)
        {
            if(info->sectionLength)lineLength_ = info->sectionLength;
            if (info->minZRot)minRot_ = info->minZRot;
            if (info->maxZRot)maxRot_ = info->maxZRot;
        }
    }
    void DrawLine() override
    {
        verticies_.push_back(myVertex(matrixStack_.back()[3].xyz(), 0xff + 255));
        matrixStack_.back().translate((dir_*lineLength_).x(),
            (dir_*lineLength_).y(),
            (dir_*lineLength_).z());
        verticies_.push_back(myVertex(matrixStack_.back()[3].xyz(), 0xff + 255));

    }
   void RotatePositive()override
    {
       matrixStack_.back().rotateZ(minRot_);
    }
    void RotateNegative()override
    {
        matrixStack_.back().rotateZ(-minRot_);
    }
    void PushStack()override
    {
        matrixStack_.push_back(matrixStack_.back());
    }
    void PopStack()override
    {
        matrixStack_.pop_back();
    }
    void Custom()override
    {
       
    }
    void SetState(LSystemState* state)override
    {
        matrixStack_.resize(0);
        matrixStack_.push_back(octet::mat4t());
    }

    void Finished()override
    {
        
        meshy_->allocate(sizeof(myVertex)* verticies_.size(), 0);
        meshy_->set_params(sizeof(myVertex), 0, verticies_.size(), GL_LINES, 0);

        meshy_->clear_attributes();
        meshy_->add_attribute(octet::attribute_pos, 3, GL_FLOAT, 0);
        meshy_->add_attribute(octet::attribute_color, 4, GL_UNSIGNED_BYTE, 12, TRUE);

        octet::gl_resource::wolock vl(meshy_->get_vertices());
        myVertex* vtx = (myVertex*)vl.u8();

        memcpy(vtx, verticies_.data(), sizeof(myVertex)*verticies_.size());
        verticies_.resize(0);
    }


    octet::mesh* GetMesh()
    {
        return meshy_;
    }
private:
    struct myVertex
    {
        myVertex(octet::vec3 v, uint32_t col)
        {
            pos = v; color = col;
        }
        myVertex(){}
        octet::vec3p pos;
        uint32_t color;
    };

    octet::dynarray<myVertex> verticies_;

    float lineLength_;
    float minRot_;
    float maxRot_;
    octet::vec3 dir_;

    int numVerticies_;

    LSystemState* state_;

    octet::dynarray<octet::mat4t> matrixStack_;
    octet::vec3 direction_;

    octet::ref<octet::mesh> meshy_;
};

#include "AngleConvert.h"

#include<time.h>
#include <random>
class DrawHelper3D : public LSystemVisualizer
{
public:
    DrawHelper3D(int vertexNum) : minRot_(20,20,20),maxRot_(0,0,0), dir_(0, 1, 0),
    randomize_(true){
        thickness_ = 0.5f;
        thicknessReduction_ = 0;
        sectionLength_ = 0.5f;
        meshy_ = new octet::mesh();
        cylinderBase_.resize(vertexNum);
        srand(time(NULL));
    }
    void Init(LSystemDrawInfo* info)  override
    {
        float oldThick = thickness_;
        if (info)
        {
            if (info->sectionWidth) thickness_ = info->sectionWidth;
            if (info->sectionWidthReduction) thicknessReduction_ = info->sectionWidthReduction;
            if (info->sectionLength)sectionLength_ = info->sectionLength;
            if (info->minXRot || info->minYRot || info->minZRot)
            {
                minRot_ = octet::vec3(info->minXRot, info->minYRot, info->minZRot);
            }
            if (info->maxXRot || info->maxYRot || info->maxZRot) maxRot_ = octet::vec3(info->maxXRot, info->maxYRot, info->maxZRot);
            randomize_ = info->randomize;
        }
        if (startPos_.size() == 0)
        {
            float angle = M_PI * 2 / cylinderBase_.size();
            for (int i = 0; i < cylinderBase_.size(); ++i)
            {
                cylinderBase_[i] = myVertex(octet::vec3(
                    sinf(angle*i)*thickness_,
                    0,
                    cosf(angle*i)*thickness_));
                verticies_.push_back(cylinderBase_.back());
            }
            startPos_.push_back(0);
        }
        else if (thickness_ != oldThick)
        {
            for (int i = 0; i < cylinderBase_.size(); ++i)
            {
                cylinderBase_[i].pos = cylinderBase_[i].pos.normalize();
                cylinderBase_[i].pos *= thickness_;
            }
        }
    }
    void DrawLine() override
    {
        octet::vec3 v = dir_*sectionLength_;
        matrixStack_.back().translate(v.x(), v.y(), v.z());
        for (int i = 0; i < cylinderBase_.size(); ++i)
        {
            verticies_.push_back(myVertex(matrixStack_.back()[3].xyz()+(cylinderBase_[i].pos*matrixStack_.back())));
        }
        startPos_.back() = MakeIndecies(startPos_.back());
    }
    void RotatePositive()override
    {
        
        if (randomize_)
        {
            float r = (float)rand() / RAND_MAX;
            switch (rand() % 3)
            {
            case 0:
                matrixStack_.back().rotateZ(minRot_.z() + max(maxRot_.z() - minRot_.z()*r,
                    0));
                break;
            case 1:
                matrixStack_.back().rotateX(minRot_.x() + max(maxRot_.x() - minRot_.x()*r,
                    0));
                break;
            case 2:
                matrixStack_.back().rotateY(minRot_.y() + max(maxRot_.y() - minRot_.y()*r,
                    0));
                break;
            }
        }
        else
        {
            
            matrixStack_.back().rotateZ(minRot_.z());
            matrixStack_.back().rotateX(minRot_.x());
            matrixStack_.back().rotateY(minRot_.y());
        }
    }
    void RotateNegative()override
    {
        if (randomize_)
        {
            float r = (float)rand() / RAND_MAX;
            switch (rand() % 3)
            {
            case 0:
                matrixStack_.back().rotateZ(-(minRot_.z() + max(maxRot_.z() - minRot_.z()*r,
                    0)));
                break;
            case 1:
                matrixStack_.back().rotateX(-(minRot_.x() + max(maxRot_.x() - minRot_.x()*r,
                    0)));
                break;
            case 2:
                matrixStack_.back().rotateY(-(minRot_.y() + max(maxRot_.y() - minRot_.y()*r,
                    0)));
                break;
            }
        }
        else
        {
            matrixStack_.back().rotateZ(-minRot_.z());
            matrixStack_.back().rotateX(-minRot_.x());
            matrixStack_.back().rotateY(-minRot_.y());
        }
    }
    void PushStack()override
    {

        startPos_.push_back(startPos_.back());
        matrixStack_.push_back(matrixStack_.back());
    }
    void PopStack()override
    {
        startPos_.pop_back();
        matrixStack_.pop_back();
    }
    void Custom()override
    {

    }
    void SetState(LSystemState* state)override
    {
        matrixStack_.resize(0);
        matrixStack_.push_back(octet::mat4t());
    }

    void Finished()override
    {
        meshy_->allocate(sizeof(myVertex)* verticies_.size(), sizeof(unsigned int)*indicies_.size());
        meshy_->set_params(sizeof(myVertex), indicies_.size(), verticies_.size(),GL_TRIANGLES, GL_UNSIGNED_INT);

        meshy_->clear_attributes();
        meshy_->add_attribute(octet::attribute_pos, 3, GL_FLOAT, 0);
        meshy_->add_attribute(octet::attribute_normal, 3, GL_FLOAT, 12);
        meshy_->add_attribute(octet::attribute_uv, 2, GL_FLOAT, 24);

        octet::gl_resource::wolock vl(meshy_->get_vertices());
        octet::gl_resource::wolock il(meshy_->get_indices());
        myVertex* vtx = (myVertex*)vl.u8();
        unsigned int * indx = (unsigned int*)il.u8();

        memcpy(vtx, verticies_.data(), sizeof(myVertex)*verticies_.size());
        memcpy(indx, indicies_.data(), sizeof(unsigned int)*indicies_.size());
        verticies_.resize(0);
        indicies_.resize(0);
        startPos_.back() = 0;
    }


    octet::mesh* GetMesh()
    {
        return meshy_;
    }
private:

    float max(float a, float b){return a > b ? a : b; }

    int MakeIndecies(int startSpace)
    {
        int objectSize = cylinderBase_.size();
        int targetSpace = verticies_.size()-objectSize;
        for (int i = 0; i < objectSize; ++i)
        {
            /*
                (ti)------(ti+1)
                  |  \      |
                  |    \    |
                  |      \  |
                  |        \|
                 (si)=-----(si+1)
            */

            indicies_.push_back(i+startSpace);

            indicies_.push_back(((i+1)%objectSize)+startSpace);

            indicies_.push_back(i + targetSpace);

            indicies_.push_back(i + targetSpace);

            indicies_.push_back(((i + 1) % objectSize) + startSpace);

            indicies_.push_back(((i + 1) % objectSize) + targetSpace);

        }
        return targetSpace;
    }
    struct myVertex
    {
        myVertex(octet::vec3 v)
        {
            pos = v; normal = v;
        }
        myVertex(){}
        octet::vec3 pos;
        octet::vec3 normal;
        octet::vec2 uv;
    };

    octet::dynarray<myVertex> verticies_;
    octet::dynarray<unsigned int> indicies_;
    octet::dynarray<int> startPos_;


    octet::dynarray<myVertex> cylinderBase_;

    float sectionLength_;
    float thickness_;

    float thicknessReduction_;

    octet::vec3 minRot_;
    octet::vec3 maxRot_;
    octet::vec3 dir_;

    bool randomize_;

    octet::dynarray<octet::mat4t> matrixStack_;

    octet::ref<octet::mesh> meshy_;
};






namespace octet {
    /// Scene containing a box with octet.
    class LSystems : public app {
        // scene for drawing box
        ref<visual_scene> app_scene;


        LSystemImporter import_;
        dynarray<LSystem> lSys_;
        DrawHelper2D draw2D_;
        DrawHelper3D draw3D_;
        const LSystemState* s_;
        LSystemDrawInfo drawInfo_;


        TwBar* bar_;
        mouse_ball camera;
        float speed_;

        dynarray<std::string> files_;

        int oldFile_;
        int fileChoice_;
        
        int numIterations_;

        bool lmbPressed_;
        
        bool is3D_;
        static bool regenerate_;
        static bool reload_;
    public:
        /// this is called when we construct the class before everything is initialised.
        LSystems(int argc, char **argv) : app(argc, argv), draw3D_(5), is3D_(true),fileChoice_(0),oldFile_(0),numIterations_(6) {
            lmbPressed_ = false;
            speed_ = 4;
        }


        static void TW_CALL Generate(void * c)
        {
            regenerate_ = true;
        }

        static void TW_CALL ChangeFile(void* c)
        {
            reload_ = true;
        }
        /// this is called once OpenGL is initialized
        void app_init() {
            app_scene = new visual_scene();
            app_scene->create_default_camera_and_lights();
            app_scene->get_camera_instance(0)->set_far_plane(1000000000000);

            drawInfo_.sectionLength = 0.5f;
            drawInfo_.sectionWidth = 0.5f;

            drawInfo_.minZRot = 20.0f;

            TwInit(TW_OPENGL, NULL);
            TwWindowSize(768, 768 - 35);

            bar_ = TwNewBar("TweakBar");

            TwAddVarRW(bar_, "Section length", TW_TYPE_FLOAT, &drawInfo_.sectionLength,
                "Min=0.00001 Max=8000 Step='0.02' Help='Length of lines drawn'");

            TwAddVarRW(bar_, "Section width", TW_TYPE_FLOAT, &drawInfo_.sectionWidth,
                "Min=0.01 Max=50 Step='0.02' Help='Thickness of 3D objects, only works on 3D'");


            TwAddVarRW(bar_, "Number of Iterations", TW_TYPE_INT16, &numIterations_, "Help='Number of iterations note that when this becomes to high loading times may be slow'");

            TwAddVarRW(bar_, "3D", TW_TYPE_BOOLCPP, &is3D_, "Help='Switches between 2D and 3D drawing'");

            TwAddSeparator(bar_, "Rotation", "");

            TwAddVarRW(bar_, "Min X rotation", TW_TYPE_FLOAT, &drawInfo_.minXRot,
                "Step=0.1f Help='Helps for random variation of X rotation'");

            TwAddVarRW(bar_, "Min Y rotation", TW_TYPE_FLOAT, &drawInfo_.minYRot,
                "Step=0.1f Help='Helps for random variation of Y rotation'");

            TwAddVarRW(bar_, "Min Z rotation", TW_TYPE_FLOAT, &drawInfo_.minZRot,
                "Step=0.1f Help='Helps for random variation of Z rotation'");


            TwAddVarRW(bar_, "Max X rotation", TW_TYPE_FLOAT, &drawInfo_.maxXRot,
                "Step=0.1f Help='If this is lower or the same then the min, then only the min is used'");

            TwAddVarRW(bar_, "Max Y rotation", TW_TYPE_FLOAT, &drawInfo_.maxYRot,
                "Step=0.1f Help='If this is lower or the same then the min, then only the min is used'");

            TwAddVarRW(bar_, "Max Z rotation", TW_TYPE_FLOAT, &drawInfo_.maxZRot,
                "Step=0.1f Help='If this is lower or the same then the min, then only the min is used'");

            TwAddVarRW(bar_, "Randomize Drawing", TW_TYPE_BOOLCPP, &drawInfo_.randomize,
                "Help='Enables or disables the randomization of angles, without the minimum angle will always be chosen'");

            TwAddSeparator(bar_, "Buttons", "");

            TwAddButton(bar_, "Generate", Generate, NULL, "");

            const int num = 8;
            TwEnumVal presets[num] =
            {
                { 0, "Tree1" },
                { 1, "Tree2" },
                { 2, "Tree3" },
                { 3, "Tree4" },
                { 4, "Tree5" },
                { 5, "Tree6" },
                //put in the two extra

                { 6, "Custom1" },
                { 7, "Custom2" },
            };

            files_.resize(num);
            lSys_.resize(num);

            TwType eNum = TwDefineEnum("Presets", presets, num);

            TwAddVarRW(bar_, "Tree presets", eNum, &fileChoice_,
                "Help='A switch for some preset LSystems use custom and the filename field to load your own'");           

            app_scene->get_light_instance(0)->get_light()->set_attenuation(0, 0.01f, 0);


            files_[0]="Tree1.txt";
            files_[1]="Tree2.txt";
            files_[2]="Tree3.txt";
            files_[3] = "Tree4.txt";
            files_[4] = "Tree5.txt";
            files_[5] = "Tree6.txt";

            files_[6] = "Triangle.txt";
            files_[7] = "Dragon.txt";


            for (int i = 0; i < files_.size(); ++i)
            {
                import_.Load(&lSys_[i], files_[i].c_str());
                if (lSys_[i].GetDrawInfo())
                {
                    drawInfo_.Combine(lSys_[i].GetDrawInfo());
                    *lSys_[i].GetDrawInfo() = drawInfo_;
                }
                else
                {
                    lSys_[i].SetDrawInfo(new LSystemDrawInfo());
                    memcpy(lSys_[i].GetDrawInfo(), &drawInfo_, sizeof(drawInfo_));
                }
                lSys_[i].Iterate(numIterations_);
            }
            
            //visi.Visualize(&draw3D);
            mesh_instance *inst;
            ref<param_shader> sh = new param_shader("shaders/default.vs", "shaders/gradient.fs");
            if (is3D_)
            {
                lSys_[0].Visualize(&draw3D_);
                inst = new mesh_instance(new scene_node(), draw3D_.GetMesh(), new material(vec4(1, 0, 0, 1), sh));
                drawInfo_ = *lSys_[0].GetDrawInfo();
            }
            else
            {
                lSys_[0].Visualize(&draw2D_);
                inst = new mesh_instance(new scene_node(), draw2D_.GetMesh(), new material(vec4(1, 0, 0, 1), sh));
                drawInfo_ = *lSys_[0].GetDrawInfo();
            }

            camera.init(this, 1000, 100.0f);
            
            glLineWidth(1.0f);
            
            app_scene->add_mesh_instance(inst);
        }

        /// this is called to draw the world
        void draw_world(int x, int y, int w, int h) {
            int vx = 0, vy = 0;
            get_viewport_size(vx, vy);
            app_scene->begin_render(vx, vy);

            if (reload_)
            {
                reload_ = false;
                
                regenerate_ = true;
            }

            if (regenerate_)
            {
                if (oldFile_ != fileChoice_)
                {
                    drawInfo_ = *lSys_[fileChoice_].GetDrawInfo();
                    oldFile_ = fileChoice_;
                }
                else
                {
                   
                    *lSys_[fileChoice_].GetDrawInfo() = drawInfo_;
                }
                s_ = lSys_[fileChoice_].GetCurrentState();
                if (s_->level > numIterations_)
                {
                    lSys_[fileChoice_].Decrement(s_->level - numIterations_);
                }
                if (s_->level < numIterations_)
                {
                    lSys_[fileChoice_].Iterate(numIterations_ - s_->level);
                }
                regenerate_ = false;
                if (is3D_)
                {
                    lSys_[fileChoice_].Visualize(&draw3D_);
                    app_scene->get_mesh_instance(0)->set_mesh(draw3D_.GetMesh());
                }
                else
                {
                    lSys_[fileChoice_].Visualize(&draw2D_);
                    app_scene->get_mesh_instance(0)->set_mesh(draw2D_.GetMesh());
                }
            }
            camera.update(app_scene->get_camera_instance(0)->get_node()->access_nodeToParent());
            // update matrices. assume 30 fps.
            app_scene->update(1.0f / 30);
            int mX=  0;
            int mY = 0;
            get_mouse_pos(mX, mY);
            TwMouseMotion(mX, mY);

            if (is_key_down(key_lmb)&&!lmbPressed_)
            {
                TwMouseButton(TW_MOUSE_PRESSED,TW_MOUSE_LEFT);
                lmbPressed_ = true;
            }
            else
            {
                lmbPressed_ = false;
                TwMouseButton(TW_MOUSE_RELEASED, TW_MOUSE_LEFT);
            }

         
            // draw the scene
            app_scene->render((float)vx / vy);


            if (is_key_down(key_shift))
            {
                speed_ = 200;
            }
            else
            {
                speed_ = 4;
            }

            if (is_key_down('W'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(0, speed_, 0));
            }
            if (is_key_down('D'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(speed_, 0, 0));
            }
            if (is_key_down('A'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(-speed_, 0, 0));
            }
            if (is_key_down('S'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(0, -speed_, 0));
            }
            if (is_key_down('Q'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(0, 0,speed_ ));
            }
            if (is_key_down('E'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(0, 0, -speed_));
            }
    
            if (is_key_down(' '))
            {
                FILE* f = fopen("NEWFILE.txt", "w");
                if (f)
                {
                    draw3D_.GetMesh()->dump(f);
                }
                fclose(f);
            }
            TwDraw();
        }
    };
    bool LSystems::regenerate_ = false;
    bool LSystems::reload_ = false;
}
