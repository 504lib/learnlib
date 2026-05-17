#include "Finite_State_Machine.h"
#include <array>
#include <new>
#include "../Log/Log.h"
#undef LOG_ASSERT
#define NO_LOG_ASSERT
#include "./static_queue/static_queue.h"

DECLARE_STATIC_QUEUE(DataPackageQueue,DataPackage,16);
DECLARE_STATIC_QUEUE(NextStateQueue,uint8_t,1);

constexpr uint8_t Invalid_State_ID = 0xFFu;                 // 无效状态ID
constexpr uint8_t MAX_State_Number = MAX_STATE_NUM;                      // 最大状态数量

enum class Current_action 
{ 
    Action,
    Entry,
    Exit
};



struct FSM_Node
{
    uint8_t        state_id = Invalid_State_ID;                               // 状态ID
    FSM_Function    function;                                                   // 状态转
};


class FSM_Structure
{
private:
    uint8_t                 current_state_id;                                     // 当前状态ID
    uint8_t                 previous_state_id;                                    // 上一个状态ID
    uint8_t                 current_state_index;                                  // 当前状态索引
    Current_action          current_action;                                 // 当前动作类型
    bool                    initialized;                                          // 是否初始化
    bool                    isEnable;                                            // 是否使能
    DataPackageQueue_t      data_package_queue;                                   // 数据包队列    
    NextStateQueue_t        next_state_queue;                                         // 状态队列
    bool                    isExistStateID(uint8_t state_id);                    // 检查状态ID是否存在
    uint8_t                 get_State_Index(uint8_t state_id);                    // 获取状态ID对应的索引
    bool                    isinitialized(void);                                  // 检查是否初始化       
    uint8_t                 Search_IDLE_State_Block(void);                        // 搜索空闲状态块                     
    void                    Transimit_action(Current_action action);                                  // 处理状态转换的动作函数
    std::array<FSM_Node, MAX_State_Number>      state_table;                      // 状态表
public:
    FSM_Structure();
    ~FSM_Structure();
    bool FSM_State_Transition(uint8_t state_id);
    bool FSM_Add_State(uint8_t state_id, FSM_Function function);
    bool FSM_Push_Data_Package(DataPackage data_package);
    bool FSM_Get_Data_Package(DataPackage* data_package);
    bool FSM_Start(uint8_t first_state_id);
    void FSM_Set_Enable(bool enable);
    void FSM_Process(void);
};

FSM_Structure::FSM_Structure()
{
    this->current_state_id = Invalid_State_ID;   // 初始化当前状态ID为无效状态
    this->previous_state_id = Invalid_State_ID;  // 初始化上一个状态ID为无效状态
    this->current_state_index = Invalid_State_ID;  // 初始化当前状态索引为无效状态
    this->current_action = Current_action::Entry;              // 初始化当前动作类型为Entry
    this->isEnable = true;                   // 初始化FSM为使能状态
    this->data_package_queue = {0};            // 初始化数据包队列
    this->next_state_queue = {0};                     // 初始化状态队列
    this->initialized = true;                   // 标记为已初始化
    NextStateQueue_INIT(&this->next_state_queue);                     // 初始化状态队列
    DataPackageQueue_INIT(&this->data_package_queue);            // 初始化数据包队列

}

FSM_Structure::~FSM_Structure()
{
    this->initialized = false;                  // 标记为未初始化
}



bool FSM_Structure::FSM_Start(uint8_t first_state_id)
{
    if (!this->isinitialized())
    {
        LOG_WARN("FSM is not initialized.");
        return false;
    }
    if (!this->isExistStateID(first_state_id))
    {
        LOG_WARN("First state ID %d does not exist.", first_state_id);
        return false;
    }
    this->current_state_id = first_state_id;   // 设置初始状态ID
    this->current_state_index = get_State_Index(this->current_state_id);  // 获取初始状态ID对应的索引
    this->current_action = Current_action::Entry;              // 切换到Entry状态，准备进入初始状态
    return true;
}


bool FSM_Structure::isinitialized(void)
{
    return this->initialized;
}

bool FSM_Structure::isExistStateID(uint8_t state_id)
{
    for (const auto& node : state_table)
    {
        if (node.state_id == state_id)
        {
            return true;                        // 找到匹配的状态ID
        }
    }
    return false;                               // 没有找到匹配的状态ID
}



bool FSM_Structure::FSM_State_Transition(uint8_t state_id)
{
    if (!this->isinitialized())
    {
        LOG_WARN("FSM is not initialized.");
        return false;
    }
    if (!this->isExistStateID(state_id))
    {
        LOG_WARN("State ID %d does not exist.", state_id);
        return false;
    }
    bool success = NextStateQueue_PUSH(&next_state_queue, state_id);  // 将目标状态ID推入状态队列
    if (!success)
    {
        LOG_WARN("Failed to push state ID %d to next state queue. Queue may be full.", state_id);
        return false;                           // 推入状态队列失败，可能是队列已满
    }
    this->current_action = Current_action::Exit;              // 切换到Exit状态，准备进行状态转换
    return true;
}

uint8_t FSM_Structure::Search_IDLE_State_Block()
{
    uint8_t index = 0;
    for (const auto& node : state_table)
    {
        if (node.state_id == Invalid_State_ID)
        {
            return index;                        // 找到空闲状态块，返回其索引
        }
        index++;
    }
    return Invalid_State_ID;                   // 没有找到空闲状态块，返回无效状态ID
}

bool FSM_Structure::FSM_Add_State(uint8_t state_id, FSM_Function function)
{
    if (!this->isinitialized())
    {
        LOG_WARN("FSM is not initialized.");
        return false;
    }
    if (state_id == Invalid_State_ID)
    {
        LOG_FATAL("Invalid state ID: %d. State ID cannot be %d.", state_id, Invalid_State_ID);
        return false;
    }
    if (!function.action)
    {
        LOG_WARN("Action function pointer is null for state ID %d. Action function is required.", state_id);
        return false;
    }
    if (!this->isExistStateID(state_id) && Search_IDLE_State_Block() != Invalid_State_ID)
    {
        uint8_t index = Search_IDLE_State_Block();  // 搜索空闲状态块
        state_table[index].state_id = state_id;     // 设置状态ID
        state_table[index].function = function;     // 设置状态函数
        return true;
    }
    LOG_WARN("State ID %d already exists or no idle state block available.", state_id);
    return false;                               // 状态ID已存在或没有空闲状态块
}



bool FSM_Structure::FSM_Push_Data_Package(DataPackage data_package)
{
    if (!this->isinitialized())
    {
        LOG_WARN("FSM is not initialized.");
        return false;
    }
    return DataPackageQueue_PUSH(&data_package_queue, data_package);  // 将数据包推入数据包队列
}


bool FSM_Structure::FSM_Get_Data_Package(DataPackage* data_package)
{
    if (!this->isinitialized())
    {
        LOG_WARN("FSM is not initialized.");
        return false;
    }
    return DataPackageQueue_POP(&data_package_queue, data_package);  // 从数据包队列中弹出一个数据包
}

void FSM_Structure::FSM_Set_Enable(bool enable)
{
    if (!this->isinitialized())
    {
        LOG_WARN("FSM is not initialized.");
        return;
    }
    this->isEnable = enable;                    // 设置FSM的使能状态
}


uint8_t FSM_Structure::get_State_Index(uint8_t state_id)
{
    uint8_t index = 0;
    if (state_id == Invalid_State_ID)
    {
        LOG_WARN("Invalid state ID: %d. State ID cannot be %d.", state_id, Invalid_State_ID);
        return Invalid_State_ID;                   // 无效状态ID，返回无效状态ID
    }
    for (const auto& node : state_table)
    {
        if (node.state_id == state_id)
        {
            return index;                        // 找到匹配的状态ID，返回其索引
        }
        index++;
    }
    return Invalid_State_ID;                   // 没有找到匹配的状态ID，返回无效状态ID
}


void FSM_Structure::Transimit_action(Current_action action)
{
    this->current_action = action;              // 更新当前动作类型    
}

void FSM_Structure::FSM_Process(void)
{
    if (!this->isinitialized())
    {
        LOG_WARN("FSM is not initialized.");
        return;
    }
    if (!this->isEnable)
    {
        LOG_INFO("FSM is disabled. Skipping processing.");
        return;
    }
    if (this->current_state_id == Invalid_State_ID)
    {
        LOG_WARN("Current state ID is invalid. Cannot process FSM.");
        return;
    }
    if (this->current_state_index == Invalid_State_ID)
    {
        this->current_state_index = get_State_Index(this->current_state_id);  // 获取当前状态ID对应的索引
        if (this->current_state_index == Invalid_State_ID)
        {
            LOG_WARN("Current state ID %d does not exist in state table. Cannot process FSM.", this->current_state_id);
            return;
        }
    }
    switch (this->current_action)
    {
    case Current_action::Action:
        if (state_table[this->current_state_index].function.action != nullptr)
        {
            state_table[this->current_state_index].function.action();  // 执行状态转换的动作函数
        }    
        break;
    case Current_action::Entry:
        if (state_table[this->current_state_index].function.entry != nullptr)
        {
            state_table[this->current_state_index].function.entry();   // 执行进入状态的动作函数
        }
        Transimit_action(Current_action::Action);                      // 切换到Action状态
        break;
    case Current_action::Exit:
        if (state_table[this->current_state_index].function.exit != nullptr)
        {
            state_table[this->current_state_index].function.exit();    // 执行退出状态
        }
        uint8_t next_state_id;
        if (NextStateQueue_POP(&this->next_state_queue,&next_state_id))
        {
            this->previous_state_id = this->current_state_id;   // 更新上一个状态ID
            this->current_state_id = next_state_id;           // 更新当前状态ID
            this->current_state_index = get_State_Index(this->current_state_id);  // 获取新状态ID对应的索引
            this->current_action = Current_action::Entry;              // 切换到Entry状态，准备进入新状态
            if (this->current_state_index == Invalid_State_ID)
            {
                LOG_WARN("Next state ID %d does not exist in state table. Cannot transition to next state.", next_state_id);
                return;
            }
        }
        else
        {
            this->current_action = Current_action::Action;                      // 没有下一个状态，保持在当前状态的Action状态
        }
    default:
        break;
    }
}

extern "C" {
    FSM_Structure* FSM_Create(FMS_MEMORY* memory)
    {
        if (memory == nullptr)
        {
            return nullptr;
        }
        static_assert(sizeof(FSM_Structure) <= FSM_STORAGE_SIZE, "FSM_Structure size exceeds FSM_STORAGE_SIZE. Please increase FSM_STORAGE_SIZE or reduce FSM_Structure size.");
        static_assert(alignof(FSM_Structure) <= alignof(FMS_MEMORY), "FSM_Structure alignment exceeds FMS_MEMORY alignment. Please adjust alignments accordingly.");
        return new(memory->storage) FSM_Structure();
    }

    void FSM_Destroy(FSM_Structure* fsm)
    {
        if (fsm == nullptr)
        {
            return;
        }
        fsm->~FSM_Structure(); // 显式调用析构函数
    }
    bool FSM_Add_State(FSM_Structure* fsm, uint8_t state_id, FSM_Function function)
    {
        if (fsm == nullptr)
        {
            return false;
        }
        return fsm->FSM_Add_State(state_id, function);
    }
    bool FSM_Push_Data_Package(FSM_Structure* fsm, DataPackage data_package)
    {
        if (fsm == nullptr)
        {
            return false;
        }
        return fsm->FSM_Push_Data_Package(data_package);
    }
    bool FSM_Get_Data_Package(FSM_Structure* fsm, DataPackage* data_package)
    {
        if (fsm == nullptr)
        {
            return false;
        }
        return fsm->FSM_Get_Data_Package(data_package);
    }
    bool FSM_Start(FSM_Structure* fsm, uint8_t first_state_id)
    {
        if (fsm == nullptr)
        {
            return false;
        }
        return fsm->FSM_Start(first_state_id);
    }
    void FSM_Set_Enable(FSM_Structure* fsm, bool enable)
    {
        if (fsm == nullptr)
        {
            return;
        }
        fsm->FSM_Set_Enable(enable);
    }
    void FSM_Process(FSM_Structure* fsm)
    {
        if (fsm == nullptr)
        {
            return;
        }
        fsm->FSM_Process();
    }
    bool FSM_State_Transition(FSM_Structure* fsm, uint8_t state_id)
    {
        if (fsm == nullptr)
        {
            return false;
        }
        return fsm->FSM_State_Transition(state_id);
    }
}