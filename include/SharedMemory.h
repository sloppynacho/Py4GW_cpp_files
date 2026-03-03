#pragma once

#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <string>

static constexpr uint32_t agent_array_max_size = 300;

struct SharedMemoryHeader {
    uint32_t version = 1;
    uint32_t total_size = 0;
    uint32_t sequence = 0;
    uint32_t process_id = 0;
    uint64_t window_handle = 0;
};

#pragma pack(push, 1)
struct TimeSharedMemoryExample {
    uint64_t tick_count = 0;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Agent_SHMemStruct {
    uintptr_t ptr = 0;
	GW::GamePos Position = GW::GamePos(0, 0, 0);
	float z = 0.0f;

    float rotation_angle = 0.0f;
	GW::Vec2f velocity = GW::Vec2f(0, 0);

    //Agent Type
	uint32_t agent_type = 0; // 0=living, 1=item, 2=gadget
    uint32_t agent_id = 0;
    uint32_t item_id = 0;
    uint32_t owner_id = 0;
    uint32_t player_number = 0;
	uint32_t profession[2] = { 0, 0 };
    uint32_t level = 0;
	float EnergyValues[3] = { 0.0f, 0.0f, 0.0f }; // energy, max_energy, energy_regen
	float HPValues[3] = { 0.0f, 0.0f, 0.0f }; // hp, max_hp, hp_regen
    uint32_t login_number = 0;
    uint32_t allegiance = 0;
    uint32_t effects = 0;
    uint32_t type_map = 0;
    uint32_t model_state = 0;
    uint32_t casting_skill_id = 0;

};

struct AgentRef_SHMemStruct {
    uint32_t agent_id = 0;
    uint32_t index = 0;
};

struct AgentRefArray_SHMemStruct {
    uint32_t count = 0;
    AgentRef_SHMemStruct entries[agent_array_max_size] = {};
};

struct AgentArray_SHMemStruct {
	uint32_t max_size = agent_array_max_size;
	uint32_t AgentArrayCount = 0;
    Agent_SHMemStruct AgentArray[agent_array_max_size] = {};
    AgentRefArray_SHMemStruct AllArray = {};
    AgentRefArray_SHMemStruct AllyArray = {};
    AgentRefArray_SHMemStruct NeutralArray = {};
    AgentRefArray_SHMemStruct EnemyArray = {};
    AgentRefArray_SHMemStruct SpiritPetArray = {};
    AgentRefArray_SHMemStruct MinionArray = {};
    AgentRefArray_SHMemStruct NPCMinipetArray = {};
    AgentRefArray_SHMemStruct LivingArray = {};
    AgentRefArray_SHMemStruct ItemArray = {};
    AgentRefArray_SHMemStruct OwnedItemArray = {};
    AgentRefArray_SHMemStruct GadgetArray = {};
    AgentRefArray_SHMemStruct DeadAllyArray = {};
    AgentRefArray_SHMemStruct DeadEnemyArray = {};
};
#pragma pack(pop)


class SharedMemoryManager {
public:
    SharedMemoryManager() = default;
    ~SharedMemoryManager();

    SharedMemoryManager(const SharedMemoryManager&) = delete;
    SharedMemoryManager& operator=(const SharedMemoryManager&) = delete;

    bool Create(const std::wstring& name, size_t total_size);
    void Destroy();

    bool IsValid() const;
    void* Data() const;
    size_t Size() const;
    const std::wstring& Name() const;

    SharedMemoryHeader* Header() const;
    // Writer-side sync helpers for the sequence-lock in SharedMemoryHeader.
    // Readers should only trust snapshots where:
    // 1. the first observed sequence is even
    // 2. the sequence did not change after reading
    void BeginWrite() const;
    void EndWrite() const;

    // Pointer to the bytes immediately after the header. This is where
    // payload structs should normally live.
    void* PayloadData(size_t offset = sizeof(SharedMemoryHeader)) const;

    template <typename T>
    T* PayloadAs(size_t offset = sizeof(SharedMemoryHeader)) const {
        return static_cast<T*>(PayloadData(offset));
    }

	//AgentArray
    bool CreateAgentArrayRegion(const std::wstring& name);
    bool UpdateAgentArrayRegion();
    AgentArray_SHMemStruct* AgentArraySMStruct() const;

    static constexpr size_t AgentArraySMStructSize() {
        return sizeof(SharedMemoryHeader) + sizeof(AgentArray_SHMemStruct);
    }


    static std::wstring BuildName(const wchar_t* prefix, DWORD process_id, HWND window_handle = nullptr);

private:
    HANDLE mapping_handle_ = nullptr;
    void* view_ = nullptr;
    size_t total_size_ = 0;
    std::wstring name_;
};
