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

/**
 * @brief    当前动作类型枚举
 *
 * Action - 状态执行动作，每周期循环执行
 * Entry  - 进入状态时执行一次
 * Exit   - 退出状态时执行一次
 */
enum class Current_action
{
    Action,
    Entry,
    Exit
};



/**
 * @brief    状态机节点，存储状态ID及其对应的动作函数集
 */
struct FSM_Node
{
    uint8_t        state_id = Invalid_State_ID;                               // 状态ID
    FSM_Function   function;                                                   // 状态转换函数集（action/entry/exit）
};


/**
 * @brief    有限状态机结构，管理状态表、状态切换、数据包队列及主循环处理
 */
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


/**
 * @brief    Construct a new fsm structure::fsm structure object
 * 
 */
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

/**
 * @brief    Destroy the fsm structure::fsm structure object
 * 
 */
FSM_Structure::~FSM_Structure()
{
    this->initialized = false;                  // 标记为未初始化
}


/**
 * @brief    启动状态机,设置初始状态并准备进入初始状态
 * 
 * @param    first_state_id  初始状态ID
 * @return   true      成功启动状态机，设置初始状态并准备进入初始状态
 * @return   false     状态机未初始化，或初始状态ID不存在，启动失败
 */
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

/**
 * @brief    返回是否初始化
 * 
 * @return   true      对象已经初始化 
 * @return   false     对象未初始化
 */
bool FSM_Structure::isinitialized(void)
{
    return this->initialized;
}


/**
 * @brief    是否存在对应状态ID
 * 
 * @param    state_id  状态ID
 * @return   true      存在
 * @return   false     不存在
 */
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



/**
 * @brief    转换状态
 * 
 * @param    state_id  唯一状态ID,即目标状态
 * @return   true      待切换成功,已经推到队列
 * @return   false     待切换失败,可能是状态机未初始化,状态ID不存在,或状态队列已满
 */
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

/**
 * @brief    内部函数::搜索空闲状态块
 * 
 * @return   uint8_t   返回对应空闲块索引,若没有返回无效状态ID
 */
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


/**
 * @brief    添加状态到状态表
 * 
 * @param    state_id  唯一状态ID,即目标状态
 * @param    function  状态函数，包括状态转换的动作函数、进入状态的动作函数和退出状态的动作函数
 * @return   true      成功添加状态到状态表
 * @return   false     状态机未初始化，或状态ID无效，或状态ID已存在，或没有空闲状态块可用，添加失败
 */
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


/**
 * @brief    向数据包队列中推送数据包，供状态机的action函数使用
 *
 * @param    data_package  待推送的数据包
 * @return   true          推送成功
 * @return   false         状态机未初始化，或队列已满
 */
bool FSM_Structure::FSM_Push_Data_Package(DataPackage data_package)
{
    if (!this->isinitialized())
    {
        LOG_WARN("FSM is not initialized.");
        return false;
    }
    return DataPackageQueue_PUSH(&data_package_queue, data_package);  // 将数据包推入数据包队列
}


/**
 * @brief    从数据包队列中取出一个数据包（FIFO顺序）
 *
 * @param    data_package  数据包指针，用于接收弹出的数据包
 * @return   true          获取成功
 * @return   false         状态机未初始化，或队列为空
 */
bool FSM_Structure::FSM_Get_Data_Package(DataPackage* data_package)
{
    if (!this->isinitialized())
    {
        LOG_WARN("FSM is not initialized.");
        return false;
    }
    return DataPackageQueue_POP(&data_package_queue, data_package);  // 从数据包队列中弹出一个数据包
}

/**
 * @brief    设置状态机的使能状态，禁用后FSM_Process将跳过处理
 *
 * @param    enable  true为使能，false为禁用
 */
void FSM_Structure::FSM_Set_Enable(bool enable)
{
    if (!this->isinitialized())
    {
        LOG_WARN("FSM is not initialized.");
        return;
    }
    this->isEnable = enable;                    // 设置FSM的使能状态
}


/**
 * @brief    内部函数::根据状态ID查找其在状态表中的索引
 *
 * @param    state_id  状态ID
 * @return   uint8_t   状态表中对应的索引，若不存在则返回Invalid_State_ID
 */
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


/**
 * @brief    内部函数::切换当前动作类型
 *
 * @param    action  目标动作类型（Action / Entry / Exit）
 */
void FSM_Structure::Transimit_action(Current_action action)
{
    this->current_action = action;              // 更新当前动作类型    
}

/**
 * @brief    状态机主处理函数，每个周期调用一次，根据当前动作类型执行相应操作
 *
 * 处理流程：
 * - Action：循环执行当前状态的action函数
 * - Entry ：执行当前状态的entry函数，完成后自动切换到Action
 * - Exit  ：执行当前状态的exit函数，从状态队列弹出下一个状态ID并切换到Entry；
 *           若队列为空则保持在当前状态的Action
 */
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

/**
 * @brief    C接口::在预分配的内存上构造状态机实例
 *
 * @param    memory           调用者提供的对齐内存块，大小至少为FSM_STORAGE_SIZE
 * @return   FSM_Structure*   成功返回状态机指针，失败（memory为nullptr）返回nullptr
 */
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

/**
 * @brief    C接口::销毁状态机实例，调用析构函数后释放资源
 *
 * @param    fsm  状态机指针，为nullptr时直接返回
 */
    void FSM_Destroy(FSM_Structure* fsm)
    {
        if (fsm == nullptr)
        {
            return;
        }
        fsm->~FSM_Structure(); // 显式调用析构函数
    }
/**
 * @brief    C接口::添加状态到状态表
 */
    bool FSM_Add_State(FSM_Structure* fsm, uint8_t state_id, FSM_Function function)
    {
        if (fsm == nullptr)
        {
            return false;
        }
        return fsm->FSM_Add_State(state_id, function);
    }
/**
 * @brief    C接口::向数据包队列推送数据包
 */
    bool FSM_Push_Data_Package(FSM_Structure* fsm, DataPackage data_package)
    {
        if (fsm == nullptr)
        {
            return false;
        }
        return fsm->FSM_Push_Data_Package(data_package);
    }
/**
 * @brief    C接口::从数据包队列取出数据包
 */
    bool FSM_Get_Data_Package(FSM_Structure* fsm, DataPackage* data_package)
    {
        if (fsm == nullptr)
        {
            return false;
        }
        return fsm->FSM_Get_Data_Package(data_package);
    }
/**
 * @brief    C接口::启动状态机，设置初始状态并进入Entry
 */
    bool FSM_Start(FSM_Structure* fsm, uint8_t first_state_id)
    {
        if (fsm == nullptr)
        {
            return false;
        }
        return fsm->FSM_Start(first_state_id);
    }
/**
 * @brief    C接口::设置状态机使能状态
 */
    void FSM_Set_Enable(FSM_Structure* fsm, bool enable)
    {
        if (fsm == nullptr)
        {
            return;
        }
        fsm->FSM_Set_Enable(enable);
    }
/**
 * @brief    C接口::状态机主处理函数，每个周期调用一次
 */
    void FSM_Process(FSM_Structure* fsm)
    {
        if (fsm == nullptr)
        {
            return;
        }
        fsm->FSM_Process();
    }
/**
 * @brief    C接口::触发状态转换，将目标状态ID推入队列
 */
    bool FSM_State_Transition(FSM_Structure* fsm, uint8_t state_id)
    {
        if (fsm == nullptr)
        {
            return false;
        }
        return fsm->FSM_State_Transition(state_id);
    }
}