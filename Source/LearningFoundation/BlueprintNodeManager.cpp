#include "BlueprintNodeManager.h"
#include "loguru.hpp"

int BlueprintNodeManager::NodeSequenceId = 0;

int BlueprintNodeManager::GetNextSequenceId()
{
    NodeSequenceId++;

    return NodeSequenceId;
}

int BlueprintNodeManager::GetCurrentSequenceId() 
{
    return NodeSequenceId;
}

BlueprintNodePin::BlueprintNodePin(const std::string& InName, const ENodePinType InType)
    : PinName(InName), PinType(InType) 
{
    PinId = BlueprintNodeManager::GetNextSequenceId();
}

void BlueprintNodePin::Draw()
{
    if (PinType == ENodePinType::INPUT_PIN)
    {
        ImNodes::BeginInputAttribute(GetId(), ImNodesPinShape_Circle);
        ImGui::TextUnformatted(PinName.c_str());
        ImNodes::EndInputAttribute();
    }
    else if (PinType == ENodePinType::OUTPUT_PIN)
    {
        ImNodes::BeginOutputAttribute(GetId(), ImNodesPinShape_CircleFilled);
        ImGui::TextUnformatted(PinName.c_str());
        ImNodes::EndOutputAttribute();
    }
    else 
    {
        LOG_F(WARNING, "illegal node pin type, pin_type: %d", PinType);
    }
}

BlueprintNode::BlueprintNode(const std::string& InTitle)
    : NodeTitle(InTitle)
{
    NodeId = BlueprintNodeManager::GetNextSequenceId();
}

const int BlueprintNode::GetId() const
{
    return NodeId;
}

void BlueprintNode::Draw()
{
    ImNodes::BeginNode(GetId());
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(NodeTitle.c_str());
    ImNodes::EndNodeTitleBar();

    for (BlueprintNodePin& Pin: Pins)
    {
        Pin.Draw();
    }

    ImNodes::EndNode();
}

BlueprintNode* BlueprintNode::AddPin(const std::string& InName, const ENodePinType InType)
{
    Pins.push_back({InName, InType});
    BlueprintNodePin& LastPin = Pins[Pins.size() - 1];
    LOG_F(INFO, "Pin num: %d, Last Pin.Id: %d, Name: %s, Type: %d", Pins.size(), 
        LastPin.GetId(), LastPin.GetName().c_str(), LastPin.GetPinType());
    return this;
}

void BlueprintNodeManager::Draw()
{
    ImNodes::BeginNodeEditor();
    for (BlueprintNode& Node: Nodes)
    {
        Node.Draw();
    }

    for (const TBPLink& Link: Links)
    {
        ImNodes::Link(Link.LinkId, Link.StartAttrId, Link.StopAttrId);
    }

    ImNodes::EndNodeEditor();
}

void BlueprintNodeManager::UpdateLinks()
{
    int StartAttrId, StopAttrId;
    if (ImNodes::IsLinkCreated(&StartAttrId, &StopAttrId))
    {
        Links.push_back({BlueprintNodeManager::GetNextSequenceId(), StartAttrId, StopAttrId});
    }
}

bool BlueprintNodeManager::IsNodeValid(const BlueprintNode& NewNode) const 
{
    return NewNode.GetId() > BlueprintNodeManager::GetCurrentSequenceId();
}

void BlueprintNodeManager::AddBPNode(const BlueprintNode& NewNode)
{
    if (IsNodeValid(NewNode) == false)
    {
        LOG_F(WARNING, "NewNode is not valid, id: %d", NewNode.GetId());
        return;
    }

    Nodes.push_back(NewNode);
}

BlueprintNode* BlueprintNodeManager::AddBPNode(const std::string& InTitle)
{
    Nodes.push_back({InTitle});
    LOG_F(INFO, "Total node num: %d", Nodes.size());
    return &Nodes[Nodes.size() - 1];
}