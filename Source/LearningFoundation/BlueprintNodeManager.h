#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "imgui.h"
#include "imnodes/imnodes.h"
#include "glm/glm.hpp"

class BlueprintNodeManager;

struct TBPLink
{
    int LinkId;
    int StartAttrId;
    int StopAttrId;
};

enum ENodePinType
{
    INPUT_PIN,
    OUTPUT_PIN
};

enum ENodePinFunc
{
    EXEC,
    DATA
};

enum ENodePinDataType
{
    NONE,
    LABEL,
    TEXT,
    FLOAT_VAL,
    VEC3_VAL,
    INT_VAL    
};

class BlueprintNode;

class BlueprintNodePin
{
public:
    BlueprintNodePin(const std::string& InName, const ENodePinType InType);
    BlueprintNodePin(const std::string& InName, const ENodePinType InType, const ENodePinDataType InDataType);
    BlueprintNodePin(const std::string& InName, const ENodePinType InType, const ENodePinDataType InDataType, BlueprintNode* InOwner);

    ~BlueprintNodePin() {}

    virtual void Draw();

    inline const int GetId() { return PinId; }
    inline const std::string& GetName() { return PinName; }
    inline const ENodePinType GetPinType()  { return PinType; }
    inline const ENodePinDataType GetPinDataType() { return PinDataType; }

    inline void SetOwner(BlueprintNode* InOwner) { Owner = InOwner; }
    inline BlueprintNode* GetOwner() { return Owner; }

    inline const ENodePinFunc GetPinFunc() { return PinFunc; }
    inline void SetPinFunc(const ENodePinFunc Func) { PinFunc = Func; }

private:
    std::string PinName;
    int PinId;
    ENodePinType PinType;  
    ENodePinDataType PinDataType; 
    ENodePinFunc PinFunc;

    std::string StrVal = "";
    int IntVal = 0;
    float FloatVal = 0.f;
    glm::vec3 Vec3Val = {0.f, 0.f, 0.f};

    BlueprintNode* Owner = nullptr;
};

class BlueprintNode
{
public:
    BlueprintNode(const std::string& InTitle);
    BlueprintNode(const std::string& InTitle, BlueprintNodeManager* InOwner);

    ~BlueprintNode(){}

    virtual void Draw();
    const int GetId() const;

    BlueprintNode* AddPin(const std::string& InName, const ENodePinType InType);
    BlueprintNode* AddTextPin(const std::string& InName, const ENodePinType InType);
    BlueprintNode* AddVector3Pin(const std::string& InName, const ENodePinType InType);
    BlueprintNode* AddFloatPin(const std::string& InName, const ENodePinType InType);
    BlueprintNode* AddIntPin(const std::string& InName, const ENodePinType InType);
    BlueprintNode* AddExecPin(const std::string& InName, const ENodePinType InType);

    void SetNodeData(void* InData);
    inline void* GetNodeData() { return Data; }
    inline void SetOwner(BlueprintNodeManager* InOwner) { Owner = InOwner; }
    inline BlueprintNodeManager* GetOwner() { return Owner; }

private:
    std::string NodeTitle;
    int NodeId;
    std::vector<BlueprintNodePin> Pins;
    void* Data = nullptr;

    BlueprintNodeManager* Owner = nullptr;
};

class BlueprintNodeManager
{
public:
    BlueprintNodeManager() {}
    ~BlueprintNodeManager() {}

    void Draw();
    void UpdateLinks();
    void AddBPNode(const BlueprintNode& NewNode);
    BlueprintNode* AddBPNode(const std::string& InTitle);
    void RemoveLinkById(int LinkId);

    void AddToAllPinList(BlueprintNodePin* Pin);

    static int GetNextSequenceId();
    static int GetCurrentSequenceId();

protected:
    bool IsNodeValid(const BlueprintNode& NewNode) const;

private:
    std::vector<BlueprintNode> Nodes;
    std::vector<TBPLink> Links;
    std::unordered_map<int /* Pin Id */, BlueprintNodePin*> AllPinsList;

    static int NodeSequenceId;
};
