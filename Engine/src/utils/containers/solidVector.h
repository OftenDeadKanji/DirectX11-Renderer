#pragma once
#include "../assert.h"
#include <cstdint>
#include <vector>

template<typename T>
class SolidVector
{
public:
    using ID = uint32_t;
    using Index = uint32_t;

    bool occupied(ID id) const 
    { 
        DEV_ASSERT(id < m_occupied.size()); 
        
        return m_occupied[id]; 
    }

    Index size() const
    { 
        return Index(m_data.size());
    }

    const T* data() const 
    { 
        return m_data.data(); 
    }
    T* data() 
    { 
        return m_data.data(); 
    }

    const T& at(Index index) const
    { 
        DEV_ASSERT(index < m_data.size());  
        
        return m_data[index]; 
    }

    T& at(Index index)
    { 
        DEV_ASSERT(index < m_data.size());  
        
        return m_data[index]; 
    }

    const T& operator[](ID id) const 
    { 
        assertId(id); 
        
        return m_data[m_forwardMap[id]]; 
    }

    T& operator[](ID id) 
    { 
        assertId(id); 
        
        return m_data[m_forwardMap[id]]; 
    }

    ID insert(const T& value)
    {
        ID id = m_nextUnused;
        DEV_ASSERT(id <= m_forwardMap.size() && m_forwardMap.size() == m_occupied.size());

        if (id == m_forwardMap.size())
        {
            m_forwardMap.push_back(Index(m_forwardMap.size() + 1));
            m_occupied.push_back(false);
        }

        DEV_ASSERT(!m_occupied[id]);

        m_nextUnused = m_forwardMap[id];
        m_forwardMap[id] = Index(m_data.size());
        m_occupied[id] = true;

        m_data.emplace_back(value);
        m_backwardMap.emplace_back(id);

        return id;
    }

    void erase(ID id)
    {
        DEV_ASSERT(id < m_forwardMap.size() && m_forwardMap.size() == m_occupied.size());

        Index& forwardIndex = m_forwardMap[id];
        DEV_ASSERT(m_occupied[id]);

        m_data[forwardIndex] = std::move(m_data.back());
        m_data.pop_back();

        ID backwardIndex = m_backwardMap.back();

        m_backwardMap[forwardIndex] = backwardIndex;
        m_backwardMap.pop_back();

        m_forwardMap[backwardIndex] = forwardIndex;

        forwardIndex = m_nextUnused;
        m_occupied[id] = false;
        m_nextUnused = id;
    }

    void clear()
    {
        m_forwardMap.clear();
        m_backwardMap.clear();
        m_occupied.clear();
        m_data.clear();
        m_nextUnused = 0;
    }

    void reserve(Index count)
    {
        m_data.reserve(count);
        m_forwardMap.reserve(count);
        m_backwardMap.reserve(count);
        m_occupied.reserve(count);
    }
protected:
    void assertId(ID id) const
    {
        DEV_ASSERT(id < m_occupied.size());
        DEV_ASSERT(m_occupied[id]);
    }

    std::vector<T> m_data;
    std::vector<Index> m_forwardMap;
    std::vector<ID> m_backwardMap;
    std::vector<bool> m_occupied;

    ID m_nextUnused = 0;
};
