// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>

#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/intrusive_red_black_tree.h"
#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/k_light_lock.h"

namespace Kernel {

class KernelCore;
class KProcess;

class KAutoObjectWithListContainer {
    NON_COPYABLE(KAutoObjectWithListContainer);
    NON_MOVEABLE(KAutoObjectWithListContainer);

public:
    using ListType = Common::IntrusiveRedBlackTreeMemberTraits<
        &KAutoObjectWithList::list_node>::TreeType<KAutoObjectWithList>;

public:
    class ListAccessor : public KScopedLightLock {
    private:
        ListType& m_list;

    public:
        explicit ListAccessor(KAutoObjectWithListContainer* container)
            : KScopedLightLock(container->m_lock), m_list(container->m_object_list) { /* ... */
        }
        explicit ListAccessor(KAutoObjectWithListContainer& container)
            : KScopedLightLock(container.m_lock), m_list(container.m_object_list) { /* ... */
        }

        typename ListType::iterator begin() const {
            return m_list.begin();
        }

        typename ListType::iterator end() const {
            return m_list.end();
        }

        typename ListType::iterator find(typename ListType::const_reference ref) const {
            return m_list.find(ref);
        }
    };

    friend class ListAccessor;

private:
    KLightLock m_lock;
    ListType m_object_list;

public:
    KAutoObjectWithListContainer(KernelCore& kernel) : m_lock(kernel), m_object_list() {}

    void Initialize() {}
    void Finalize() {}

    void Register(KAutoObjectWithList* obj);
    void Unregister(KAutoObjectWithList* obj);
    size_t GetOwnedCount(KProcess* owner);
};

} // namespace Kernel
