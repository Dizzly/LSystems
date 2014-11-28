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

//an abstract base class for creation of a intergrated graphics tool, not necessary
class LSystemVisualizer
{
public:
    virtual void DrawLine() = 0{};

    virtual void RotatePositive() = 0{};
    virtual void RotateNegative() = 0{};

    virtual void PushStack() = 0{};
    virtual void PopStack() = 0{};

    virtual void DrawLeaf(){};
    virtual void Rotate(){};
    virtual void Custom(){};

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
    LSystem(){}
    ~LSystem(){}

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
        stateVec_.back()->level = stateVec_.size();
        const LSystemState* prevState = stateVec_.back()->prevState;
        assert(prevState->state_.size() > 0);
        if (visualizer_)
        {
            visualizer_->SetState(stateVec_.back());
        }
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

    void AddAlphabetSymbol(char symb)
    {
        alphabet_.push_back(symb);
    }

    void SetAxiom(const char* c, int size)
    {
        assert(stateVec_.size() == 0);
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

    void SetKeyDecl(char c,KEY_SYMBOLS k)
    {
        referenceMap_[c].key = k;
    }

    void SetVisualizer(LSystemVisualizer* viz)
    {
        visualizer_ = viz;
    }

    void AddRuleFunction(char c, VarFunc func)
    {
        referenceMap_[c].func = func;
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
        if (r.key != KEY_NULL&&visualizer_)
        {
            CallKey(r.key);
        }
    }

    void CallKey(int key)
    {
        switch (key)
        {
        case(KEY_DRAW) :
            visualizer_->DrawLine();
            break;
        case(KEY_PLUS_ROTATE) :
            visualizer_->RotatePositive();
            break;
        case(KEY_MINUS_ROTATE) :
            visualizer_->RotateNegative();
            break;
        case(KEY_PUSH) :
            visualizer_->PushStack();
            break;
        case(KEY_POP) :
            visualizer_->PopStack();
            break;
        case(KEY_LEAF) :
            visualizer_->DrawLeaf();
            break;
        case(KEY_ROTATE) :
            visualizer_->Rotate();
            break;
        case(KEY_CUSTOM) :
            visualizer_->Custom();
            break;
        }
    }

private:
    LSystemVisualizer* visualizer_;

    octet::dynarray<LSystemState*> stateVec_;
    octet::hash_map<char, SymbolRef> referenceMap_;
    octet::dynarray<char> axiom_;
    octet::dynarray<char> alphabet_;
};




class DrawHelper: public LSystemVisualizer
{
public:
    void DrawLine() override
    {

    }
   void RotatePositive()override
    {

    }
    void RotateNegative()override
    {

    }
    void PushStack()override
    {

    }
    void PopStack()override
    {

    }
    void Custom()override
    {
       
    }

    void SetState(LSystemState* state)override
    { }
private:
    float lineLength_;
    octet::vec3 minRot_;
    octet::vec3 maxRot_;
};



#include <fstream>
//the importer kept seperate from the LSystem for reduction in function count and use
//only one public function, to read into an LSystem
class LSystemImporter
{
public:
    LSystemImporter(){}
    ~LSystemImporter(){}

    bool Load(LSystem* lSys, const octet::string& filename)
    {
        std::fstream file;

        file.open(filename.c_str(), std::ios::in);
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
                        lSys->SetKeyDecl(newSymbol,keyNum);
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







namespace octet {
    /// Scene containing a box with octet.
    class LSystems : public app {
        // scene for drawing box
        ref<visual_scene> app_scene;


        LSystemImporter import;
        LSystem visi;
        DrawHelper d;
        const LSystemState* s;
    public:
        /// this is called when we construct the class before everything is initialised.
        LSystems(int argc, char **argv) : app(argc, argv) {
        }

        /// this is called once OpenGL is initialized
        void app_init() {
            app_scene = new visual_scene();
            app_scene->create_default_camera_and_lights();
            app_scene->get_camera_instance(0)->set_far_plane(1000000000000);
            import.Load(&visi, "LSys.txt");
            visi.SetVisualizer(&d);

            struct myVertex{
                myVertex(vec3 p, uint32_t c) :pos(p), color(c){}
                vec3p pos;
                uint32_t color;
            };

           

            bool breakpoint = true;
            visi.Iterate(9);
            s = visi.GetCurrentState();
            breakpoint = true;



            mesh* meshy = new mesh();
            meshy->allocate(sizeof(myVertex)* 1600000, 0);
            meshy->set_params(sizeof(myVertex), 0, 1600000, GL_LINES, 0);

            meshy->add_attribute(attribute_pos, 3, GL_FLOAT, 0);
            meshy->add_attribute(attribute_color, 4, GL_UNSIGNED_BYTE, 12, TRUE);

            gl_resource::wolock vl(meshy->get_vertices());
            myVertex* vtx = (myVertex*)vl.u8();

            vec4 dir(0, 0.1, 0, 0);
            dynarray<mat4t> stack;
            stack.push_back(mat4t());

            for (int i = 0; i < s->state_.size(); ++i)
            {
                const char c = s->state_[i];
                if (c == 'F')
                {
                    *vtx = myVertex(vec3(0, 0, -10) * stack.back(), 0xff + 255 << 0);//creates a vertex
                    ++vtx;
                    stack.back().translate(0, 0.1, 0);
                    *vtx = myVertex(vec3(0, 0, -10) * stack.back(), 0xff + 255 << 0);
                    ++vtx;
                }
                if (c == '+')
                {
                    stack.back().rotateZ(20);
                }
                if (c == '-')
                {
                    stack.back().rotateZ(-20);
                }
                if (c == '[')
                {
                    stack.push_back(stack.back());
                }
                if (c == ']')
                {
                    stack.pop_back();
                }
            }
            glLineWidth(0.1);

            param_shader* shader = new param_shader("shaders/default.vs", "shaders/simple_color.fs");
            mesh_instance *inst = new mesh_instance(new scene_node(), meshy, new material(vec4(1, 0, 0, 1),
                shader));
            app_scene->add_mesh_instance(inst);
        }

        /// this is called to draw the world
        void draw_world(int x, int y, int w, int h) {
            int vx = 0, vy = 0;
            get_viewport_size(vx, vy);
            app_scene->begin_render(vx, vy);

            // update matrices. assume 30 fps.
            app_scene->update(1.0f / 30);

            // draw the scene
            app_scene->render((float)vx / vy);

            if (is_key_down('W'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(0, 0, -1));
            }
            if (is_key_down('D'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(1, 0, 0));
            }
            if (is_key_down('A'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(-1, 0, 0));
            }
            if (is_key_down('S'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(0, 0, 1));
            }
            if (is_key_down('E'))
            {
                app_scene->get_camera_instance(0)->get_node()->rotate(0.4f, vec3(1, 0, 0));
            }
            if (is_key_down('Q'))
            {
                app_scene->get_camera_instance(0)->get_node()->rotate(-0.4f, vec3(1, 0, 0));
            }

        }
    };
}
