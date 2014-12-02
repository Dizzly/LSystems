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

class LSystemDrawInfo{}

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
            LSystemState* state=stateVec_.back();
            viz->SetState(state);
            for (int i = 0; i < state->state_.size(); ++i)
            {
                CallKey(referenceMap_[state->state_[i]].key,viz);
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




class DrawHelper2D: public LSystemVisualizer
{
public:

    DrawHelper2D(): minRot_(0,0,20),maxRot_(0,0,20),dir_(0,1,0){
    lineLength_=0.1f;
    meshy_ = new octet::mesh();
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
       matrixStack_.back().rotateZ(minRot_.z());
    }
    void RotateNegative()override
    {
        matrixStack_.back().rotateZ(-minRot_.z());
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

        meshy_->add_attribute(octet::attribute_pos, 3, GL_FLOAT, 0);
        meshy_->add_attribute(octet::attribute_color, 4, GL_UNSIGNED_BYTE, 12, TRUE);

        octet::gl_resource::wolock vl(meshy_->get_vertices());
        myVertex* vtx = (myVertex*)vl.u8();

        memcpy(vtx, verticies_.data(), sizeof(myVertex)*verticies_.size());
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
    octet::vec3 minRot_;
    octet::vec3 maxRot_;
    octet::vec3 dir_;

    int numVerticies_;

    LSystemState* state_;

    octet::dynarray<octet::mat4t> matrixStack_;
    octet::vec3 direction_;

    octet::mesh* meshy_;
};

#include "AngleConvert.h"

#include<time.h>
#include <random>
class DrawHelper3D : public LSystemVisualizer
{
public:

    DrawHelper3D() : minRot_(0, 0, 20), maxRot_(0, 0, 20), dir_(0, 1, 0){
        sectionLength_ = 0.5f;
        meshy_ = new octet::mesh();

        srand(time(NULL));

        float thickness = 0.4f;
        int numVertexInBase = 8;
        float angle = M_PI*2 / numVertexInBase;
        cylinderBase_.reserve(numVertexInBase);
        for (int i = 0; i < numVertexInBase; ++i)
        {
            cylinderBase_.push_back(myVertex(octet::vec3(
                sin(angle*i)*thickness,
                0,
                cos(angle*i)*thickness)));
            verticies_.push_back(cylinderBase_.back());
        }
        startPos_.push_back(0);

    }
    void DrawLine() override
    {
        octet::vec3 v = dir_*sectionLength_;
        matrixStack_.back().translate(v.x(),v.y(),v.z());
        octet::quat q = matrixStack_.back().toQuaternion();
        for (int i = 0; i < cylinderBase_.size(); ++i)
        {
            verticies_.push_back(myVertex(matrixStack_.back()[3].xyz()+(cylinderBase_[i].pos*q)));
        }
        startPos_.back() = MakeIndecies(startPos_.back());
    }
    void RotatePositive()override
    {
        switch (rand()%3)
        {
        case 0:
            matrixStack_.back().rotateZ(minRot_.z());
            break;
        case 1:
            matrixStack_.back().rotateX(minRot_.z());
            break;
        case 2:
            matrixStack_.back().rotateY(minRot_.z());
            break;
        }
    }
    void RotateNegative()override
    {
        switch (rand()%3)
        {
        case 0:
            matrixStack_.back().rotateZ(-minRot_.z());
            break;
        case 1:
            matrixStack_.back().rotateX(-minRot_.z());
            break;
        case 2:
            matrixStack_.back().rotateY(-minRot_.z());
            break;
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

        meshy_->add_attribute(octet::attribute_pos, 3, GL_FLOAT, 0);
        meshy_->add_attribute(octet::attribute_normal, 3, GL_FLOAT, 12);
        meshy_->add_attribute(octet::attribute_uv, 2, GL_FLOAT, 24);

        octet::gl_resource::wolock vl(meshy_->get_vertices());
        octet::gl_resource::wolock il(meshy_->get_indices());
        myVertex* vtx = (myVertex*)vl.u8();
        unsigned int * indx = (unsigned int*)il.u8();

        memcpy(vtx, verticies_.data(), sizeof(myVertex)*verticies_.size());
        memcpy(indx, indicies_.data(), sizeof(unsigned int)*indicies_.size());
    }


    octet::mesh* GetMesh()
    {
        return meshy_;
    }
private:

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
    octet::vec3 minRot_;
    octet::vec3 maxRot_;
    octet::vec3 dir_;

    int numVerticies_;

    LSystemState* state_;

    octet::dynarray<octet::mat4t> matrixStack_;
    octet::vec3 direction_;

    octet::mesh* meshy_;
};









namespace octet {
    /// Scene containing a box with octet.
    class LSystems : public app {
        // scene for drawing box
        ref<visual_scene> app_scene;


        LSystemImporter import;
        LSystem visi;
        DrawHelper2D d;
        DrawHelper3D d2;
        const LSystemState* s;


        mouse_ball camera;
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
            
            visi.Iterate(9);
            s = visi.GetCurrentState();
            //visi.Visualize(&d);
            visi.Visualize(&d2);

            app_scene->get_light_instance(0)->get_light()->set_attenuation(0, 0.01f, 0);

            camera.init(this, 100, 100.0f);
            
            glLineWidth(0.1f);
            ref<param_shader> sh = new param_shader("shaders/default.vs", "shaders/gradient.fs");
            mesh_instance *inst = new mesh_instance(new scene_node(), d2.GetMesh(), new material(vec4(1, 0, 0, 1),sh));
            app_scene->add_mesh_instance(inst);
        }

        /// this is called to draw the world
        void draw_world(int x, int y, int w, int h) {
            int vx = 0, vy = 0;
            get_viewport_size(vx, vy);
            app_scene->begin_render(vx, vy);


            camera.update(app_scene->get_camera_instance(0)->get_node()->access_nodeToParent());
            // update matrices. assume 30 fps.
            app_scene->update(1.0f / 30);


            
            // draw the scene
            app_scene->render((float)vx / vy);




            if (is_key_down('W'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(0, 4, 0));
            }
            if (is_key_down('D'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(4, 0, 0));
            }
            if (is_key_down('A'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(-4, 0, 0));
            }
            if (is_key_down('S'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(0, -4, 0));
            }
            if (is_key_down('Q'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(0, 0,4 ));
            }
            if (is_key_down('E'))
            {
                app_scene->get_camera_instance(0)->get_node()->translate(vec3(0, 0, -4));
            }


        }
    };
}
