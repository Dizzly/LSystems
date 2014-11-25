////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//


class LSystemVisualizer
{ //make these variables private
public:
    enum KEY_SYMBOLS{
        KEY_DRAW = 1,
        KEY_PLUS_ROTATE,
        KEY_MINUS_ROTATE,
        KEY_PUSH,
        KEY_POP,
        KEY_ROTATE,
        KEY_LEAF,
        KEY_CUSTOM,

        KEY_MAX
    };

    class LSystemState
    {
    public:
        LSystemState(LSystemState* prev = NULL) : readIndex(0), prevState(prev)
        {
            prevState = prev;
        }
        ~LSystemState(){}

        int readIndex;
        const LSystemState *prevState;
        octet::dynarray<char> state_;
    };
    typedef bool(*VarFunc)(LSystemState*);
public:
    LSystemVisualizer(){}
    ~LSystemVisualizer(){}
    void Iterate(int n)
    {
        for (int i = 0; i < n; ++i)
        {
            Iterate();
        }
    }

    void Iterate()
    {
        assert(states_.size()>0);
        states_.push_back(LSystemState(&states_.back()));
        const LSystemState* prevState = states_.back().prevState;
        assert(prevState->state_.size() > 0);
        for (int i = 0; i < prevState->state_.size(); ++i)
        {
            states_.back().readIndex = i;
            ProcessSymbol(prevState->state_[i]);
        }
    }

    void AddAlphabetSymbol(char symb)
    {
        alphabet_.push_back(symb);
    }
    void SetAxiom(const char* c, int size)
    {
        axiom_.resize(size);
        memcpy(axiom_.data(), c, size);
        states_.push_back(LSystemState());
        states_.back().state_ = axiom_;
    }

    void AddBasicRules(char c, octet::string& str)
    {
        simpleRules_[c] = str;
    }
    void AddRuleFunctions(char c, VarFunc func)
    {
        varFuncs_[c] = func;
    }

    const LSystemState& GetCurrentState()
    {
        return states_.back();
    }
private:
    void ProcessSymbol(const char c)
    {
        LSystemState* state = &states_.back();
        if (simpleRules_.get_index(c) !=-1)
        {
            const octet::string& str = simpleRules_.get_value(
                simpleRules_.get_index(c));

            for (int i = 0; i < str.size(); ++i)
            {
                states_.back().state_.push_back(str[i]);
            }
        }
        else if(varFuncs_.get_index(c)!=-1)
        {
            VarFunc v = varFuncs_.get_value(
                varFuncs_.get_index(c));
            v(&states_.back());
        }
        else
        {
            state->state_.push_back(c);
        }
        
    }
private:
    octet::dynarray<LSystemState> states_;

    octet::hash_map<char,octet::string> simpleRules_;
    octet::hash_map<char,VarFunc> varFuncs_;
    octet::dynarray<char> axiom_;
    octet::dynarray<char> alphabet_;
};




#include <fstream>
class LSystemImporter
{
public:
    LSystemImporter(){}
    ~LSystemImporter(){}

    bool Load(LSystemVisualizer* lSys,const octet::string& filename)
    {
        std::fstream file;

        std::string;
        file.open(filename.c_str(),std::ios::in);
        if (!file.is_open())
        {
            assert(0);
        }

        file.seekg(0,file.end);
        int length = file.tellg();
        file.seekg(0,file.beg);
        
        read_.resize(length);
        file.read(read_.data(), length);
        CleanWhiteSpace();

        octet::string str(read_.data(), read_.size());
        bool check = false;
        check=LoadKeyDeclerations(lSys, str);
        if (!check)return false;
        check=LoadAlphabet(lSys,str);
        if (!check)return false;
        check=LoadAxiom(lSys, str);
        if (!check)return false;
        check=LoadRules(lSys, str);
        if (!check)return false;


        return true;
    }
    //load the symbol alphabet from the text file containing all non-inbuilt symbols in the L system
    //a local version is stored for error checking in the Axiom and Rule sections
    bool LoadAlphabet(LSystemVisualizer* lSys,octet::string& str)
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
    bool LoadAxiom(LSystemVisualizer* lSys, octet::string& str)
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
        if (arr.size()==0)
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
    bool LoadRules(LSystemVisualizer* lSys, octet::string& str)
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
            if (read_[i]=='=')
            {
                char symbol(read_[i - 1]);
                if (IsNotGrammer(symbol))
                {
                    if (IsInAlphabet(symbol))
                    {
                        int ruleEndLoc = FindSymbolAfter(';', i);
                        if (ruleEndLoc>endLoc||ruleEndLoc==-1)
                        {
                            printf("%s%c%s\n", "No semicolon after ", symbol, "'s rule");
                            assert(false);
                            return false;
                        }
                        lSys->AddBasicRules(symbol, octet::string(&read_[i + 1], ruleEndLoc - 1 - i));// take the chracters till the ;, excluding the ;
                        i = ruleEndLoc+1;//skip the rule we just read for effiency
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
    bool LoadKeyDeclerations(LSystemVisualizer* lSys, octet::string& str)
    {

        int startLoc = str.find("KeyDecl");
      
        keys_[LSystemVisualizer::KEY_DRAW] = 'F';
        keys_[LSystemVisualizer::KEY_PUSH] = '[';
        keys_[LSystemVisualizer::KEY_POP] = ']';
        keys_[LSystemVisualizer::KEY_PLUS_ROTATE] = '+';
        keys_[LSystemVisualizer::KEY_MINUS_ROTATE] = '-';

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
                    octet::string key(&read_[i+1], end - 1 - i);
                    int keyNum = KeyFromString(key);
                    if (keyNum != -1)
                    {
                        keys_[keyNum] = newSymbol;
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
        return false;
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
    static int KeyFromString(const octet::string& c)
    {
        if (c == "KEY_DRAW")
        {
            return LSystemVisualizer::KEY_DRAW;
        }
        if (c == "KEY_PLUS_ROTATE")
        {
            return LSystemVisualizer::KEY_PLUS_ROTATE;
        }
        if (c == "KEY_MINUS_ROTATE")
        {
            return LSystemVisualizer::KEY_MINUS_ROTATE;
        }
        if (c == "KEY_PUSH")
        {
            return LSystemVisualizer::KEY_PUSH;
        }
        if (c == "KEY_POP")
        {
            return LSystemVisualizer::KEY_POP;
        }
        /*
        //Possible extension of features
        if (c == "KEY_LEAF")
        {
            return LSystemVisualizer::KEY_LEAF;
        }
        if (c == "KEY_ROTATE")
        {
            return LSystemVisualizer::KEY_ROTATE;
        }
        */
        return -1;
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
    octet::hash_map<int, char> keys_;
    octet::dynarray<char> tempAlphabet_;
    octet::dynarray<char> read_;
};







namespace octet {
    /// Scene containing a box with octet.
    class LSystems : public app {
        // scene for drawing box
        ref<visual_scene> app_scene;


        LSystemImporter import;
        LSystemVisualizer visi;
        const LSystemVisualizer::LSystemState* s;
    public:
        /// this is called when we construct the class before everything is initialised.
        LSystems(int argc, char **argv) : app(argc, argv) {
        }

        /// this is called once OpenGL is initialized
        void app_init() {
            app_scene = new visual_scene();
            app_scene->create_default_camera_and_lights();
            
            import.Load(&visi, "LSys.txt");
            struct myVertex{
                myVertex(vec3 p, uint32_t c) :pos(p),color(c){}
                vec3p pos;
                uint32_t color;
            };

            mesh* meshy = new mesh();
            meshy->allocate(sizeof(myVertex)* 16000, 0);
            meshy->set_params(sizeof(myVertex), 0, 16000, GL_LINES, 0);

            meshy->add_attribute(attribute_pos, 3, GL_FLOAT, 0);
            meshy->add_attribute(attribute_color, 4, GL_UNSIGNED_BYTE, 12, TRUE);

            visi.Iterate();
            s = &visi.GetCurrentState();
            bool breakpoint = true;
            visi.Iterate(4);
            s = &visi.GetCurrentState();
            breakpoint = true;

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
                    *vtx = myVertex(vec3(0, 0, -10) * stack.back(), 0xff + 255<< 0);
                    ++vtx;
                    stack.back().translate(0, 0.1,0);
                    *vtx = myVertex(vec3(0,0,-10) * stack.back(),0xff+255<<0);
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
            glLineWidth(1);

            param_shader* shader = new param_shader("shaders/default.vs", "shaders/simple_color.fs");
            mesh_instance *inst = new mesh_instance(new scene_node(), meshy, new material(vec4(1,0,0,1),
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
            if (is_key_down('S'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(0, 0, 1));
            }

            // tumble the box  (there is only one mesh instance)

        }
    };
}
