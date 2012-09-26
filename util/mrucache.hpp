#ifndef __UTIL_MRUCACHE_HPP
#define __UTIL_MRUCACHE_HPP

#include <cassert>
#include <iterator>
#include <boost/cstdint.hpp>
#include <boost/unordered_map.hpp>

namespace util
{

template <typename KeyType, typename ValueType>
class MRUCache;

template <typename KeyType, typename ValueType>
class MRUCache
{
  struct Entry;
  
  typedef boost::unordered_map<KeyType, Entry*> EntriesMap;

  EntriesMap entries;
  Entry* first;
  Entry* last;
  uint16_t capacity;
  
  struct Entry
  {
    MRUCache& cache;
    std::pair<KeyType, ValueType> pair;
    Entry* next;
    Entry* prev;
    
    Entry(MRUCache& cache, const KeyType& key, const ValueType& value) :
      cache(cache), pair(std::make_pair(key, value)), next(0), prev(0)
    {
      Entry* temp = cache.first;
      cache.first = this;
      if (temp) temp->prev = cache.first;
      cache.first->next = temp;
      if (!cache.first->next) cache.last = cache.first;
    }
    
    ~Entry()
    {
      if (next) next->prev = prev;
      if (prev) prev->next = next;
      if (this == cache.first) cache.first = next;
      if (this == cache.last) cache.last = prev;
    }
  };
  
  void EraseOldest()
  {
    typename EntriesMap::iterator it = entries.find(last->pair.first);
    assert(it != entries.end());
    delete it->second;
    entries.erase(it);
  }
  
public:
  typedef typename boost::unordered_map<KeyType, ValueType>::size_type size_type;

  class const_iterator;
  
  class iterator : public std::iterator<std::forward_iterator_tag, ValueType>
  {
    Entry* entry;

  public:
    iterator(Entry* entry) : entry(entry) { }
    iterator(const iterator& iter) : entry(iter.entry) { }
   ~iterator() { }

    iterator& operator=(const iterator& rhs)
    {
       entry = rhs.entry;
       return *this ;
    }
    
    bool operator==(const iterator& rhs) { return entry == rhs.entry; }
    bool operator!=(const iterator& rhs) { return entry != rhs.entry; }

    iterator& operator++()
    {
      if (entry) entry = entry->next;
      return *this;
    }

    iterator operator++(int)
    {
       iterator temp(*this);
       ++(*this);
       return temp;
    }

    std::pair<KeyType, ValueType>& operator*() { return entry->pair; }
    std::pair<KeyType, ValueType>* operator->() { return &entry->pair; }
    friend class const_iterator;
  };
  
  class const_iterator : public std::iterator<std::forward_iterator_tag, ValueType>
  {
    Entry* entry;

  public:
    const_iterator(Entry* entry) : entry(entry) { }
    const_iterator(const const_iterator& iter) : entry(iter.entry) { }
    const_iterator(const iterator& iter) : entry(iter.entry) { }
   ~const_iterator() { }

    const_iterator& operator=(const const_iterator& rhs)
    {
       entry = rhs.entry;
       return *this ;
    }

    bool operator==(const const_iterator& rhs) { return entry == rhs.entry; }
    bool operator!=(const const_iterator& rhs) { return entry != rhs.entry; }

    const_iterator& operator++()
    {
      if (entry) entry = entry->next;
      return *this;
    }

    const_iterator operator++(int)
    {
       const_iterator temp(*this);
       ++(*this);
       return temp;
    }

    const std::pair<KeyType, ValueType>& operator*() const { return entry->pair; }
    const std::pair<KeyType, ValueType>* operator->() const { return &entry->pair; }
  };

  MRUCache(uint16_t capacity) :
    first(0), last(0), capacity(capacity)
  {
    if (!capacity) throw std::logic_error("Capacity must be larger than zero");
  }
  
  ~MRUCache()
  {
    while (!entries.empty())
    {
      delete entries.begin()->second;
      entries.erase(entries.begin());
    }
  }
  
  const ValueType& Lookup(const KeyType& key) const
  {
    typename EntriesMap::iterator it = entries.find(key);
    if (it == entries.end()) throw std::out_of_range("Key not in cache");
    return it->second->pair.second;
  }
  
  ValueType& Lookup(const KeyType& key)
  {
    typename EntriesMap::iterator it = entries.find(key);
    if (it == entries.end()) throw std::out_of_range("Key not in cache");
    return it->second->pair.second;
  }
  
  void Insert(const KeyType& key, const ValueType& value)
  {
    while (entries.size() >= capacity) EraseOldest();
    entries.insert(std::make_pair(key, new Entry(*this, key, value)));
  }
  
  iterator begin() { return iterator(first); }
  iterator end() { return iterator(0); }
  const_iterator begin() const { return const_iterator(first); }
  const_iterator end() const { return const_iterator(0); }  
  
  friend struct Entry;
};

} /* util namespace */

#endif