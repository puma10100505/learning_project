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

BlueprintNodePin::BlueprintNodePin(const std::string& InName, const ENodePinType InType, const ENodePinDataType InDataType)
    : PinName(InName), PinType(InType), PinDataType(InDataType)
{
    PinId = BlueprintNodeManager::GetNextSequenceId();
}

BlueprintNodePin::BlueprintNodePin(const std::string& InName, const ENodePinType InType, const ENodePinDataType InDataType, BlueprintNode* InOwner)
    : PinName(InName), PinType(InType), PinDataType(InDataType), Owner(InOwner)
{
    PinId = BlueprintNodeManager::GetNextSequenceId();
}

void BlueprintNodePin::Draw()
{
    if (PinType == ENodePinType::INPUT_PIN)
    {
        //LOG_F(INFO, "pin type: %d, datatype: %d", PinType, PinDataType);
        // ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
        // ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkCreationOnSnap);
        if (PinFunc == ENodePinFunc::EXEC)
        {
            ImNodes::BeginInputAttribute(GetId(), ImNodesPinShape_Triangle);
            ImGui::SetNextItemWidth(120);
            ImNodes::EndInputAttribute();
        }
        else
        {
            switch (PinDataType)
            {
                case ENodePinDataType::TEXT:
                {
                    ImNodes::BeginInputAttribute(GetId(), ImNodesPinShape_Circle);                
                    ImGui::SetNextItemWidth(120);
                    ImGui::InputInt(PinName.c_str(), (int*)&(IntVal));
                    ImNodes::EndInputAttribute();
                    break;
                }

                case ENodePinDataType::INT_VAL:
                {
                    ImNodes::BeginInputAttribute(GetId(), ImNodesPinShape_Circle);
                    ImGui::SetNextItemWidth(120);
                    ImGui::InputInt(PinName.c_str(), (int*)&(IntVal));
                    ImNodes::EndInputAttribute();
                    break;
                }

                case ENodePinDataType::FLOAT_VAL:
                {
                    ImNodes::BeginInputAttribute(GetId(), ImNodesPinShape_Circle);
                    ImGui::SetNextItemWidth(120);
                    ImGui::InputFloat(PinName.c_str(), (float*)&(FloatVal));
                    ImNodes::EndInputAttribute();
                    break;
                }

                case ENodePinDataType::VEC3_VAL:
                {
                    ImNodes::BeginInputAttribute(GetId(), ImNodesPinShape_Circle);
                    ImGui::SetNextItemWidth(120);
                    ImGui::InputFloat3(PinName.c_str(), (float*)&(Vec3Val));
                    ImNodes::EndInputAttribute();
                    break;
                }

                case ENodePinDataType::LABEL:
                default:
                {
                    ImNodes::BeginInputAttribute(GetId(), ImNodesPinShape_Circle);
                    ImGui::TextUnformatted(PinName.c_str());
                    ImNodes::EndInputAttribute();
                }
            }
        }
    }
    else if (PinType == ENodePinType::OUTPUT_PIN)
    {
        if (PinFunc == ENodePinFunc::EXEC)
        {
            ImNodes::BeginOutputAttribute(GetId(), ImNodesPinShape_Triangle);
            ImGui::SetNextItemWidth(120);
            ImNodes::EndOutputAttribute();
        }
        else
        {
            switch (PinDataType)
            {
                case ENodePinDataType::LABEL:
                default:
                {
                    ImNodes::BeginOutputAttribute(GetId(), ImNodesPinShape_Circle);
                    ImGui::TextUnformatted(PinName.c_str());
                    ImNodes::EndOutputAttribute();
                }
            }
        }   
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

BlueprintNode::BlueprintNode(const std::string& InTitle, BlueprintNodeManager* InOwner)
    : NodeTitle(InTitle), Owner(InOwner)
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
    Pins.push_back({InName, InType, ENodePinDataType::LABEL});    
    BlueprintNodePin& LastPin = Pins[Pins.size() - 1];
    LastPin.SetPinFunc(ENodePinFunc::EXEC);
    LastPin.SetOwner(this);
    GetOwner()->AddToAllPinList(&LastPin);

    LOG_F(INFO, "Pin num: %d, Last Pin.Id: %d, Name: %s, Type: %d", Pins.size(), 
        LastPin.GetId(), LastPin.GetName().c_str(), LastPin.GetPinType());
    return this;
}

BlueprintNode* BlueprintNode::AddExecPin(const std::string& InName, const ENodePinType InType)
{
    Pins.push_back({InName, InType, ENodePinDataType::NONE});
    BlueprintNodePin& LastPin = Pins[Pins.size() - 1];
    LastPin.SetOwner(this);
    LastPin.SetPinFunc(ENodePinFunc::EXEC);
    GetOwner()->AddToAllPinList(&LastPin);
    LOG_F(INFO, "AddExecPin num: %d, Last Pin.Id: %d, Name: %s, Type: %d", Pins.size(), 
        LastPin.GetId(), LastPin.GetName().c_str(), LastPin.GetPinType());
    return this;
}

BlueprintNode* BlueprintNode::AddTextPin(const std::string& InName, const ENodePinType InType)
{
    Pins.push_back({InName, InType, ENodePinDataType::TEXT});
    BlueprintNodePin& LastPin = Pins[Pins.size() - 1];
    LastPin.SetOwner(this);
    LastPin.SetPinFunc(ENodePinFunc::DATA);
    GetOwner()->AddToAllPinList(&LastPin);
    LOG_F(INFO, "AddTextPin Pin num: %d, Last Pin.Id: %d, Name: %s, Type: %d", Pins.size(), 
        LastPin.GetId(), LastPin.GetName().c_str(), LastPin.GetPinType());
    return this;
}

BlueprintNode* BlueprintNode::AddIntPin(const std::string& InName, const ENodePinType InType)
{
    Pins.push_back({InName, InType, ENodePinDataType::INT_VAL});
    BlueprintNodePin& LastPin = Pins[Pins.size() - 1];
    LastPin.SetOwner(this);
    LastPin.SetPinFunc(ENodePinFunc::DATA);
    GetOwner()->AddToAllPinList(&LastPin);
    LOG_F(INFO, "AddIntPin Pin num: %d, Last Pin.Id: %d, Name: %s, Type: %d", Pins.size(), 
        LastPin.GetId(), LastPin.GetName().c_str(), LastPin.GetPinType());
    return this;
}

BlueprintNode* BlueprintNode::AddFloatPin(const std::string& InName, const ENodePinType InType)
{
    Pins.push_back({InName, InType, ENodePinDataType::FLOAT_VAL});
    BlueprintNodePin& LastPin = Pins[Pins.size() - 1];
    LastPin.SetOwner(this);
    LastPin.SetPinFunc(ENodePinFunc::DATA);
    GetOwner()->AddToAllPinList(&LastPin);
    LOG_F(INFO, "AddFloatPin Pin num: %d, Last Pin.Id: %d, Name: %s, Type: %d", Pins.size(), 
        LastPin.GetId(), LastPin.GetName().c_str(), LastPin.GetPinType());
    return this;
}

BlueprintNode* BlueprintNode::AddVector3Pin(const std::string& InName, const ENodePinType InType)
{
    Pins.push_back({InName, InType, ENodePinDataType::VEC3_VAL});
    BlueprintNodePin& LastPin = Pins[Pins.size() - 1];
    LastPin.SetOwner(this);  
    LastPin.SetPinFunc(ENodePinFunc::DATA);  
    GetOwner()->AddToAllPinList(&LastPin);
    LOG_F(INFO, "AddVector3Pin Pin num: %d, Last Pin.Id: %d, Name: %s, Type: %d", Pins.size(), 
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

void BlueprintNodeManager::RemoveLinkById(int LinkId)
{
    for (auto Itr = Links.begin(); Itr != Links.end(); ++Itr)
    {
        if (Itr->LinkId == LinkId)
        {
            Links.erase(Itr);
            break;
        }
    }
}

void BlueprintNodeManager::UpdateLinks()
{
    // TODO: 触发LINK变更事件
    // 当前帧是否有新增加的LINK
    do 
    {
        int StartAttrId, StopAttrId;
        if (ImNodes::IsLinkCreated(&StartAttrId, &StopAttrId))
        {
            // 执行流与执行流连接，数据流与数据流连接
            if (AllPinsList[StartAttrId]->GetPinFunc() != AllPinsList[StopAttrId]->GetPinFunc())
            {
                break;
            }

            // 如果是执行流则保证流程唯一性
            if (AllPinsList[StartAttrId]->GetPinFunc() == ENodePinFunc::EXEC || AllPinsList[StopAttrId]->GetPinFunc() == ENodePinFunc::EXEC)
            {
                for (const TBPLink& LinkItem: Links)
                {
                    if (LinkItem.StartAttrId == StartAttrId || LinkItem.StopAttrId == StopAttrId)
                    {
                        RemoveLinkById(LinkItem.LinkId);
                    }
                }
            }

            Links.push_back({BlueprintNodeManager::GetNextSequenceId(), StartAttrId, StopAttrId});
        }
    } while (false);

    // TODO: 触发LINK变更事件
    // 当前帧是否有删除的LINK
    int DetachedLinkId;
    if (ImNodes::IsLinkDestroyed(&DetachedLinkId))
    {
        RemoveLinkById(DetachedLinkId);
    }

    LOG_F(INFO, "selected nodes.num: %d, selected links.num: %d", ImNodes::NumSelectedNodes(), ImNodes::NumSelectedLinks());

    // 
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
    Nodes.push_back({InTitle, this});

    LOG_F(INFO, "Total node num: %d", Nodes.size());
    return &Nodes[Nodes.size() - 1];
}

void BlueprintNode::SetNodeData(void* InData)
{
    Data = InData;
}

void BlueprintNodeManager::AddToAllPinList(BlueprintNodePin* Pin)
{
    if (Pin == nullptr)
    {
        return;
    }

    AllPinsList.insert({Pin->GetId(), Pin});
}