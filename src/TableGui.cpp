#include "TableGUI.h"
#include <exception>
typedef struct RECT_INT
{
    INT32 SpriteID;
    INT32 CX;
    INT32 CY;
} RECT_INT;

namespace
{
    ImGuiTree* g_pInstance = nullptr;
}
bool g_editopen = true;
ImGuiTree::ImGuiTree()
{
    if (g_pInstance)
        throw std::exception("ImGuiLog is a singleton!");
    isPlayAnimation = false;
    ActionElements = 0;
    g_pInstance = this;

    Clear();
}

ImGuiTree& ImGuiTree::Get()
{
    if (!g_pInstance)
        throw std::exception("ImGuiLog needs an instance!");
    return *g_pInstance;
}

bool ImGuiTree::HasInstance()
{
    return g_pInstance != nullptr;
}


void ImGuiTree::Clear()
{


}

void ImGuiTree::ShowCFPKEditTable(const char* title, ACTION_FRAME_ARRAY& cfpk, bool* p_open)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    if (!ImGui::Begin(title, p_open, window_flags))
    {
        ImGui::End();
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(430, 650), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    if (ImGui::BeginTable("##split", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Object");
        ImGui::TableSetupColumn("Contents",ImGuiTableColumnFlags_WidthFixed, 300.0f);
        ImGui::TableHeadersRow();
        int action_size = cfpk.GetSize();

        for (int obj_i = 0; obj_i < action_size; obj_i++)
        {
            int obj_id = obj_i + 100;
            ShowObject("Object", cfpk[obj_i], obj_id);
        }


        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
    ImGui::End();

}
void ImGuiTree::ShowObject(const char* prefix, FRAME_ARRAY& cfpk, int uid)
{
    static CAnimationManager& m_animationManager = CAnimationManager::Get();
    static std::shared_ptr<CAnimation> m_pAnimation = m_animationManager.GetAnimation("empty");
    static std::shared_ptr<CFrame> temporaryFrame = m_pAnimation->GetFrame(0);
    ImGui::PushID(uid);
    // Text and Tree nodes are less high than framed widgets, using AlignTextToFramePadding() we add vertical spacing to make the tree lines equal high.
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    bool node_open = ImGui::TreeNode("FRAME", "%s_%u", prefix, uid);

    if (node_open)
    {

        static ImVector<RECT_INT> items;
        if (items.Size != cfpk.GetSize())
        {
            items.resize(cfpk.GetSize());//
            for (int n = 0; n < cfpk.GetSize(); n++)
            {
                const int temp_n = n % cfpk.GetSize();
                items[temp_n].SpriteID = cfpk[n].GetSpriteID();
                items[temp_n].CX= cfpk[n].GetCX();
                items[temp_n].CY= cfpk[n].GetCY();
                
            }
        }
        for (int i = 0; i < cfpk.GetSize(); ++i)
        {
            RECT_INT* item = &items[i];
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
            ImGui::TreeNodeEx("Sprite", flags, "Sprite_%d", i);

            ImGui::TableSetColumnIndex(1);
            //ImGui::SetNextItemWidth(-FLT_MIN);
            int* p = (int*)item;
            ImGui::InputInt3("",  p);
            ImGui::SameLine();

            if (ImGui::Button("Show")) 
            {

                isPlayAnimation = false;
                
                std::shared_ptr<CFrame>m_pFrame = m_pAnimation->GetFrame(0);
                m_pFrame->Set(item->SpriteID, item->CX, item->CY);

            }
            ImGui::SameLine();
            if (ImGui::Button("Editing"))
            {

                isPlayAnimation = false;
                
                
                std::shared_ptr<CFrame>m_pFrame = m_pAAnimation->GetFrame(i);
                p[1] = temporaryFrame->GetCX();
                p[2] = temporaryFrame->GetCY();
                m_pFrame->Set(p[0], temporaryFrame->GetCX(), temporaryFrame->GetCY());

            }
            ImGui::SameLine();
            if (ImGui::Button("COPY"))
            {
                std::shared_ptr<CFrame> emptyFrame = std::make_shared<CFrame>(p[0], temporaryFrame->GetCX(), temporaryFrame->GetCY());

                m_pAAnimation->AddFrame(emptyFrame);
            }
  
            ImGui::NextColumn();


            ImGui::PopID();
         }
            // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
        ImGui::TreePop();
     }
    ImGui::PopID();
}
void ImGuiTree::ShowObject(const char* prefix, DIRECTION_FRAME_ARRAY& cfpk, int uid)
{
    // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
    ImGui::PushID(uid);
    
    // Text and Tree nodes are less high than framed widgets, using AlignTextToFramePadding() we add vertical spacing to make the tree lines equal high.
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    bool node_open = ImGui::TreeNode("Object", "%s_%u", prefix, uid);
    if (cfpk[0].GetSize() == 0)
    {
        ImGui::TableSetColumnIndex(1);

        ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "invalid Data");
        if (node_open)
        {
            ImGui::TreePop();
        }
    }
    else
    {
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("Normal Data");
        ImGui::SameLine();
        if (ImGui::Button("PlayFarme"))
        {
            static CAnimationManager& m_animationManager = CAnimationManager::Get();
            FRAME_ARRAY& PlayFrame = cfpk[0];
            std::shared_ptr<CAnimation> emptyAnimat = std::make_shared<CAnimation>();
            for (int i = 0; i < PlayFrame.GetSize(); ++i)
            {
                

                std::shared_ptr<CFrame> emptyFrame = std::make_shared<CFrame>(PlayFrame[i].GetSpriteID(), PlayFrame[i].GetCX(), PlayFrame[i].GetCY());
                emptyAnimat->AddFrame(emptyFrame);
                

         
            }
            m_animationManager.AddAnimation("Frame", emptyAnimat);
            isPlayAnimation = true;
            ActionElements = uid - 100;
        }
        if (node_open)
        {

            for (int i = 0; i < 8; i++)
            {
                ImGui::PushID(i); // Use field index as identifier.


                ShowObject("Child", cfpk[i], i);


                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
   
    ImGui::PopID();







}
void ImGuiTree::ShowEditgui(int dir, bool& p_open)
{
   
    static std::vector<std::shared_ptr<CAnimation>> DIR_animation_vector;
    static std::shared_ptr<CFrame> temporaryFrame;
    static  FRAME_VECTOR_3U tempFrame;
    static ACTION_FRAME_ARRAY temp_cfpk;
    static DIRECTION_FRAME_ARRAY temp_DF;
    static FRAME_ARRAY temp_FA;
    static CCreatureFramePack NewCFPK;
    static bool openeditor = false;
    static CAnimationManager& m_animationManager = CAnimationManager::Get();
    static std::shared_ptr<CAnimation> m_pAnimation = m_animationManager.GetAnimation("empty");

    static std::shared_ptr<CAnimation> m_pAnimation1 = std::make_shared<CAnimation>();//1
    static std::shared_ptr<CAnimation> m_pAnimation2 = std::make_shared<CAnimation>();//2
    static std::shared_ptr<CAnimation> m_pAnimation3 = std::make_shared<CAnimation>();//3
    static std::shared_ptr<CAnimation> m_pAnimation4 = std::make_shared<CAnimation>();//4
    static std::shared_ptr<CAnimation> m_pAnimation5 = std::make_shared<CAnimation>();//5
    static std::shared_ptr<CAnimation> m_pAnimation6 = std::make_shared<CAnimation>();//6
    static std::shared_ptr<CAnimation> m_pAnimation7 = std::make_shared<CAnimation>();//7
    static std::shared_ptr<CAnimation> m_pAnimation8 = std::make_shared<CAnimation>();//8


    if (!ImGui::Begin("NewAdditioFrameworkPackage"))
    {

        return;
    }


    static int selected = 0;
    {
        ImGui::BeginChild("DirectionChoice", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);

        for (int i = 0; i < 8; i++)
        {

            // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
            char label[128];
            sprintf(label, "Direction OBJECT %d", i);
            if (ImGui::Selectable(label, selected == i)) {
                selected = i;
            }
         
           


        }




        ImGui::EndChild();
    }

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::BeginChild("CFPK Editor", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
    ImGui::Separator();
    if (ImGui::Button("ClearDatabase"))
    {


        for (const auto& element : DIR_animation_vector) {
            element->ClearAnimations();
        }
        openeditor = false;
        m_pAnimation->ClearAnimations();
    }
    ImGui::Separator();
    if (ImGui::Button("InitResource"))
    {
        openeditor = false;
 
        NewCFPK.Init(1);
        temp_cfpk.Init(1);
        temp_cfpk[0].Init(8);

        DIR_animation_vector.clear();

        DIR_animation_vector.push_back(m_pAnimation1);
        DIR_animation_vector.push_back(m_pAnimation2);
        DIR_animation_vector.push_back(m_pAnimation3);
        DIR_animation_vector.push_back(m_pAnimation4);
        DIR_animation_vector.push_back(m_pAnimation5);
        DIR_animation_vector.push_back(m_pAnimation6);
        DIR_animation_vector.push_back(m_pAnimation7);
        DIR_animation_vector.push_back(m_pAnimation8);

        

    }

    ImGui::InputInt("New Element Direction", &dir);
    ImGui::InputInt("New Element SpriteID", &tempFrame.spriteID);
    ImGui::InputInt("New Element CX", &tempFrame.x);
    ImGui::InputInt("New Element CY", &tempFrame.y);

    if (ImGui::Button("Add CFPK"))
    {
        m_pAAnimation = DIR_animation_vector[selected];


        std::shared_ptr<CFrame> emptyFrame = std::make_shared<CFrame>(tempFrame.spriteID, tempFrame.x, tempFrame.y);
        
        m_pAAnimation->AddFrame(emptyFrame);
    }

    if (ImGui::Button("PRINT_CFPK"))
    {
        for (int dir_i = 0; dir_i < 8; dir_i++)
        {
            m_pAnimation = DIR_animation_vector[dir_i];
            int size_vector = m_pAnimation->getSize();
            temp_FA.Init(size_vector);
            for (int i = 0; i < size_vector; ++i) {
                temporaryFrame = m_pAnimation->GetFrame(i);
                temp_FA[i] = *temporaryFrame;
            }


            temp_cfpk[0][dir_i] = temp_FA;





        }
       

        NewCFPK[0] = temp_cfpk;
        openeditor = true;




        

    }

    if (ImGui::Button("Save File"))
    {
        ofstream wcfpk_file("New.cfpk", ofstream::binary);
        ofstream wicfpk_file("New.cfpki", ofstream::binary);
        NewCFPK.SaveToFile(wcfpk_file, wicfpk_file);
    }
    if (ImGui::Button("Show object by IMgui"))
    {
        isPlayAnimation = false;

        std::shared_ptr<CFrame>m_pFrame = m_pAnimation->GetFrame(0);
        m_pFrame->Set(tempFrame.spriteID, tempFrame.x, tempFrame.y);
    }

    ImGui::EndChild();
    ImGui::EndGroup();

    ImGui::End();


    if (openeditor)
    {
        ShowCFPKEditTable("NewSSPK", NewCFPK[0], &p_open);
    }
}
