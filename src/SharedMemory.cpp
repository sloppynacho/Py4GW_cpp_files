#include "Headers.h"
#include "SharedMemory.h"

SharedMemoryManager::~SharedMemoryManager() {
    Destroy();
}

bool SharedMemoryManager::Create(const std::wstring& name, size_t total_size) {
    Destroy();

    if (name.empty() || total_size < sizeof(SharedMemoryHeader)) {
        return false;
    }

    const DWORD high = static_cast<DWORD>((static_cast<unsigned long long>(total_size) >> 32) & 0xFFFFFFFFull);
    const DWORD low = static_cast<DWORD>(static_cast<unsigned long long>(total_size) & 0xFFFFFFFFull);

    HANDLE mapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        high,
        low,
        name.c_str()
    );

    if (!mapping) {
        return false;
    }

    void* view = MapViewOfFile(
        mapping,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        total_size
    );

    if (!view) {
        CloseHandle(mapping);
        return false;
    }

    mapping_handle_ = mapping;
    view_ = view;
    total_size_ = total_size;
    name_ = name;

    ZeroMemory(view_, total_size_);

    auto* header = Header();
    header->version = 1;
    header->total_size = static_cast<uint32_t>(total_size_);
    header->sequence = 0;
    header->process_id = GetCurrentProcessId();
    header->window_handle = reinterpret_cast<uint64_t>(GW::MemoryMgr::GetGWWindowHandle());

    return true;
}

void SharedMemoryManager::Destroy() {
    if (view_) {
        UnmapViewOfFile(view_);
        view_ = nullptr;
    }

    if (mapping_handle_) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
    }

    total_size_ = 0;
    name_.clear();
}

bool SharedMemoryManager::IsValid() const {
    return mapping_handle_ != nullptr && view_ != nullptr;
}

void* SharedMemoryManager::Data() const {
    return view_;
}

size_t SharedMemoryManager::Size() const {
    return total_size_;
}

const std::wstring& SharedMemoryManager::Name() const {
    return name_;
}

SharedMemoryHeader* SharedMemoryManager::Header() const {
    if (!view_) {
        return nullptr;
    }
    return static_cast<SharedMemoryHeader*>(view_);
}

void SharedMemoryManager::BeginWrite() const {
    auto* header = Header();
    if (!header) {
        return;
    }
    // Sequence-lock pattern:
    // 1. Writer increments to an odd value before touching payload bytes.
    // 2. Readers treat odd values as "write in progress" and retry.
    // 3. EndWrite() increments again, making the sequence even.
    // 4. Readers accept the snapshot only if the sequence is the same
    //    before and after reading and the final value is even.
    ++header->sequence;
}

void SharedMemoryManager::EndWrite() const {
    auto* header = Header();
    if (!header) {
        return;
    }
    ++header->sequence;
}

void* SharedMemoryManager::PayloadData(size_t offset) const {
    if (!view_ || offset >= total_size_) {
        return nullptr;
    }
    return static_cast<uint8_t*>(view_) + offset;
}

std::wstring SharedMemoryManager::BuildName(const wchar_t* prefix, DWORD process_id, HWND window_handle) {
    wchar_t buffer[160] = { 0 };
    swprintf_s(
        buffer,
        L"%ls_%lu_%p",
        prefix ? prefix : L"Py4GW_SharedMemory_PID_",
        static_cast<unsigned long>(process_id),
        window_handle
    );
    return std::wstring(buffer);
}


bool SharedMemoryManager::CreateAgentArrayRegion(const std::wstring& name) {
    return Create(name, AgentArraySMStructSize());
}

bool SharedMemoryManager::UpdateAgentArrayRegion() {
    AgentArray_SHMemStruct* payload = AgentArraySMStruct();
    if (!payload) {
        return false;
    }

    auto instance_type = GW::Map::GetInstanceType();
    bool is_map_ready = (GW::Map::GetIsMapLoaded()) && (!GW::Map::GetIsObserving()) && (instance_type != GW::Constants::InstanceType::Loading);

    const auto agents = GW::Agents::GetAgentArray();
    const auto ac = GW::GetAgentContext();
    
    BeginWrite();

    ZeroMemory(payload, sizeof(*payload));
    payload->max_size = agent_array_max_size;

    if (!agents || !ac) {
        EndWrite();
        return false;
    }

    if (!is_map_ready) {
        EndWrite();
        return false;
    }

    auto push_ref = [](AgentRefArray_SHMemStruct& arr, uint32_t agent_id, uint32_t index) {
        if (arr.count >= agent_array_max_size) {
            return;
        }
        AgentRef_SHMemStruct& entry = arr.entries[arr.count++];
        entry.agent_id = agent_id;
        entry.index = index;
    };

    for (const GW::Agent* agent : *agents) {
        if (!agent) {
            continue;
        }


        const uint32_t id = agent->agent_id;
        if (!id) {
            continue;
        }

        // Match the existing stale-pointer gate used by FilterAgents.
        if (!(ac->agent_movement.size() > id && ac->agent_movement[id])) {
            continue;
        }

        if (payload->AgentArrayCount >= agent_array_max_size) {
            break;
        }

        const uint32_t slot = payload->AgentArrayCount++;
        Agent_SHMemStruct& out = payload->AgentArray[slot];

		out.ptr = reinterpret_cast<uintptr_t>(agent);

        out.Position = agent->pos;
		out.z = agent->z;

        out.rotation_angle = agent->rotation_angle;
		out.velocity = agent->velocity;
        out.agent_id = id;
        push_ref(payload->AllArray, id, slot);

        if (agent->GetIsGadgetType()) {
            out.agent_type = 2;
            push_ref(payload->GadgetArray, id, slot);
            continue;
        }

        if (agent->GetIsItemType()) {
            out.agent_type = 1;

            const auto item = agent->GetAsAgentItem();
            if (!item) {
                continue;
            }

            out.item_id = item->item_id;
            out.owner_id = item->owner;
            push_ref(payload->ItemArray, id, slot);

            if (item->owner != 0) {
                push_ref(payload->OwnedItemArray, id, slot);
            }
            continue;
        }

        if (!agent->GetIsLivingType()) {
            continue;
        }

        out.agent_type = 0;

        const auto living = agent->GetAsAgentLiving();
        if (!living) {
            continue;
        }

        push_ref(payload->LivingArray, id, slot);

        out.owner_id = static_cast<uint32_t>(living->owner);
        out.player_number = living->player_number;
        out.profession[0] = static_cast<uint32_t>(living->primary);
        out.profession[1] = static_cast<uint32_t>(living->secondary);
        out.level = living->level;
        out.EnergyValues[0] = living->energy;
        out.EnergyValues[1] = living->max_energy;
        out.EnergyValues[2] = living->energy_regen;
        out.HPValues[0] = living->hp;
        out.HPValues[1] = living->max_hp;
        out.HPValues[2] = static_cast<float>(living->hp_pips);
        out.login_number = living->login_number;
        out.allegiance = static_cast<uint32_t>(living->allegiance);
        out.effects = living->effects;
        out.type_map = living->type_map;
        out.model_state = living->model_state;
        out.casting_skill_id = living->skill;

        switch (living->allegiance) {
        case GW::Constants::Allegiance::Ally_NonAttackable:
            push_ref(payload->AllyArray, id, slot);
            if (living->GetIsDead()) {
                push_ref(payload->DeadAllyArray, id, slot);
            }
            break;
        case GW::Constants::Allegiance::Neutral:
            push_ref(payload->NeutralArray, id, slot);
            break;
        case GW::Constants::Allegiance::Enemy:
            push_ref(payload->EnemyArray, id, slot);
            if (living->GetIsDead()) {
                push_ref(payload->DeadEnemyArray, id, slot);
            }
            break;
        case GW::Constants::Allegiance::Spirit_Pet:
            push_ref(payload->SpiritPetArray, id, slot);
            break;
        case GW::Constants::Allegiance::Minion:
            push_ref(payload->MinionArray, id, slot);
            break;
        case GW::Constants::Allegiance::Npc_Minipet:
            push_ref(payload->NPCMinipetArray, id, slot);
            break;
        default:
            break;
        }
    }

    EndWrite();

    return true;
}

AgentArray_SHMemStruct* SharedMemoryManager::AgentArraySMStruct() const {
    return PayloadAs<AgentArray_SHMemStruct>();
}
