#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "imgui.h"
#include "imnodes/imnodes.h"

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

class BlueprintNodePin
{
public:
    BlueprintNodePin(const std::string& InName, const ENodePinType InType);

    ~BlueprintNodePin() {}

    virtual void Draw();

    inline const int GetId() { return PinId; }
    inline const std::string& GetName() { return PinName; }
    inline const ENodePinType GetPinType()  { return PinType; }

private:
    std::string PinName;
    int PinId;
    ENodePinType PinType;  
};

class BlueprintNode
{
public:
    BlueprintNode(const std::string& InTitle);

    ~BlueprintNode(){}

    virtual void Draw();
    const int GetId() const;

    BlueprintNode* AddPin(const std::string& InName, const ENodePinType InType);

private:
    std::string NodeTitle;
    int NodeId;
    std::vector<BlueprintNodePin> Pins;
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

    static int GetNextSequenceId();
    static int GetCurrentSequenceId();

protected:
    bool IsNodeValid(const BlueprintNode& NewNode) const;

private:
    std::vector<BlueprintNode> Nodes;
    std::vector<TBPLink> Links;

    static int NodeSequenceId;
};
